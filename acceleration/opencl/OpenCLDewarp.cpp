/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "OpenCLDewarp.h"
#include "Utils.h"
#include "imageproc/BadAllocIfNull.h"
#include "imageproc/Grayscale.h"
#include <QSysInfo>
#include <QDebug>
#include <CL/cl.hpp>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

using namespace imageproc;
using namespace dewarping;

namespace opencl
{

namespace
{

std::pair<QImage, cl::ImageFormat> convertImage(QImage const& src, QColor const& bg_color)
{
	auto is_opaque_gray = [](QRgb rgba) {
		return qAlpha(rgba) == 0xff && qRed(rgba) == qBlue(rgba) && qRed(rgba) == qGreen(rgba);
	};

	switch (src.format()) {
		case QImage::Format_Invalid:
			throw std::runtime_error("Null image passed to opencl::dewarp()");
		case QImage::Format_Indexed8:
		case QImage::Format_Mono:
		case QImage::Format_MonoLSB:
			if (src.allGray() && is_opaque_gray(bg_color.rgba())) {
				return std::make_pair(
					toGrayscale(src), cl::ImageFormat(CL_LUMINANCE, CL_UNORM_INT8)
				);
			}
			// fall through
		default:
			cl_channel_order const rgba_order =
					QSysInfo::ByteOrder == QSysInfo::BigEndian ? CL_ARGB : CL_BGRA;
			if (!src.hasAlphaChannel() && qAlpha(bg_color.rgba()) == 0xff) {
				return std::make_pair(
					badAllocIfNull(src.convertToFormat(QImage::Format_RGB32)),
					cl::ImageFormat(rgba_order, CL_UNORM_INT8)
				);
			} else {
				return std::make_pair(
					badAllocIfNull(src.convertToFormat(QImage::Format_ARGB32)),
					cl::ImageFormat(rgba_order, CL_UNORM_INT8)
				);
			}
	}
}

struct Generatrix
{
	cl_float2 origin;
	cl_float2 vector;
	cl_float2 homog_m1;
	cl_float2 homog_m2;

	Generatrix() = default;

	Generatrix(CylindricalSurfaceDewarper::Generatrix const& gen) {
		origin.s[0] = gen.imgLine.p1().x();
		origin.s[1] = gen.imgLine.p1().y();
		vector.s[0] = gen.imgLine.p2().x() - gen.imgLine.p1().x();
		vector.s[1] = gen.imgLine.p2().y() - gen.imgLine.p1().y();
		homog_m1.s[0] = gen.pln2img.mat()(0, 0);
		homog_m1.s[1] = gen.pln2img.mat()(1, 0);
		homog_m2.s[0] = gen.pln2img.mat()(0, 1);
		homog_m2.s[1] = gen.pln2img.mat()(1, 1);
	}
};

} // anonymous namespace


QImage dewarp(
	cl::CommandQueue const& command_queue,
	cl::Program const& program,
	QImage const& src, QSize const& dst_size,
	dewarping::CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& background_color,
	QSizeF const& min_mapping_area)
{
	cl::Context const context = command_queue.getInfo<CL_QUEUE_CONTEXT>();
	cl::Device const device = command_queue.getInfo<CL_QUEUE_DEVICE>();
	size_t const max_const_mem = device.getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>();
	size_t const max_wg_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
	size_t const v_wg_size = std::sqrt((double)max_wg_size);
	size_t const h_wg_size = max_wg_size / v_wg_size;

	auto const converted_image = convertImage(src, background_color);

	// Create source and destination images on the device.
	cl::Image2D src_image(
		context, CL_MEM_READ_ONLY,
		converted_image.second, src.width(), src.height()
	);
	cl::Image2D dst_image(
		context, CL_MEM_WRITE_ONLY,
		converted_image.second, dst_size.width(), dst_size.height()
	);

	// Writhe the source image to device memory.
	cl::size_t<3> const origin;
	cl::size_t<3> region;
	region[0] = src.width();
	region[1] = src.height();
	region[2] = 1;
	command_queue.enqueueWriteImage(
		src_image, CL_TRUE, origin, region, converted_image.first.bytesPerLine(), 0,
		(void*)converted_image.first.bits()
	);

	// Create host and device buffers for storing Generatrix structures.
	// Because of constant memory size limitations, we may need several passes.
	// Across passes we need a total of dst_width + 1 Generatrix instances,
	// indexed from [0] to [dst_width].
	int const range_buffer_elements = std::min<int>(
		max_const_mem / sizeof(Generatrix), dst_size.width() + 1
	);
	size_t const range_buffer_size = range_buffer_elements * sizeof(Generatrix);
	cl::Buffer const range_device_buffer(context, CL_MEM_READ_ONLY, range_buffer_size);
	std::vector<Generatrix> range_host_buffer(range_buffer_elements);

	float const model_domain_left = model_domain.left();
	float const model_x_scale = 1.f / (model_domain.right() - model_domain.left());

	float const model_domain_top = model_domain.top();
	float const model_y_scale = 1.f / (model_domain.bottom() - model_domain.top());

	CylindricalSurfaceDewarper::State state;

	int range_begin = 0;
	int range_end = 0;
	while (range_end < dst_size.width()) {
		// Note that range boundaries are inclusive.
		range_begin = range_end;
		range_end = std::min<int>(
			dst_size.width(), range_begin + range_buffer_elements - 1
		);

		for (int dst_x = range_begin; dst_x <= range_end; ++dst_x) {
			double const model_x = (dst_x - model_domain_left) * model_x_scale;
			CylindricalSurfaceDewarper::Generatrix const generatrix(
				distortion_model.mapGeneratrix(model_x, state)
			);
			range_host_buffer[dst_x - range_begin] = generatrix;
		}

		// Copy generatrix_host_buffer to generatrix_device_buffer.
		command_queue.enqueueWriteBuffer(
			range_device_buffer, CL_TRUE, 0, (range_end + 1 - range_begin)*sizeof(Generatrix),
			range_host_buffer.data()
		);

		cl::Kernel kernel(program, "dewarp");
		int idx = 0;
		kernel.setArg(idx++, src_image);
		kernel.setArg(idx++, dst_image);
		kernel.setArg(idx++, range_device_buffer);
		kernel.setArg(idx++, cl_int(range_end));
		kernel.setArg(idx++, cl_float(model_domain_top));
		kernel.setArg(idx++, cl_float(model_y_scale));
		kernel.setArg(idx++, cl_float4{
			(float)background_color.redF(), (float)background_color.greenF(),
			(float)background_color.blueF(), (float)background_color.alphaF()
		}),
		kernel.setArg(idx++, cl_float2{
			(float)min_mapping_area.width(), (float)min_mapping_area.height()
		});

		cl::Event evt;

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NDRange(range_begin, 0),
			cl::NDRange(
				thisOrNextMultipleOf(range_end - range_begin, h_wg_size),
				thisOrNextMultipleOf(dst_size.height(), v_wg_size)
			),
			cl::NDRange(h_wg_size, v_wg_size), nullptr, &evt
		);

		evt.wait();
	}

	QImage dst(dst_size, converted_image.first.format());
	badAllocIfNull(dst);
	region[0] = dst.width();
	region[1] = dst.height();
	region[2] = 1;
	command_queue.enqueueReadImage(
		dst_image, CL_TRUE, origin, region, dst.bytesPerLine(), 0, dst.bits()
	);

	return dst;
}

} // namespace opencl
