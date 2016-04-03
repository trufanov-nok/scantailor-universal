/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015-2016  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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
#include <CL/cl2.hpp>
#include <array>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <vector>

using namespace imageproc;
using namespace dewarping;

namespace opencl
{

namespace
{

struct AdaptedImage
{
	QImage image;
	cl::ImageFormat clFormat;
};

AdaptedImage adaptImage(QImage const& src, QColor const& bg_color)
{
	AdaptedImage adapted;

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
				adapted.image = toGrayscale(src);
				adapted.clFormat = cl::ImageFormat(CL_R, CL_UNORM_INT8);
				break;
			}
			// fall through
		default:
			if (!src.hasAlphaChannel() && qAlpha(bg_color.rgba()) == 0xff) {
				// Note that Qt stores RGB32 as 0xffRRGGBB, so it can be interpreted
				// as ARGB by OpenCL.
				adapted.image = badAllocIfNull(src.convertToFormat(QImage::Format_RGB32));
			} else {
				adapted.image = badAllocIfNull(src.convertToFormat(QImage::Format_ARGB32));
			}
			cl_channel_order const order = QSysInfo::BigEndian ? CL_ARGB : CL_BGRA;
			adapted.clFormat = cl::ImageFormat(order, CL_UNORM_INT8);
			break;
	}

	return adapted;
}

struct Generatrix
{
	cl_float2 origin;
	cl_float2 vector;
	cl_float2 homog_m1;
	cl_float2 homog_m2;
	cl_int2 dst_y_range;

	void set(CylindricalSurfaceDewarper::Generatrix const& gen, std::pair<int, int> dst_y_range) {
		origin.s[0] = gen.imgLine.p1().x();
		origin.s[1] = gen.imgLine.p1().y();
		vector.s[0] = gen.imgLine.p2().x() - gen.imgLine.p1().x();
		vector.s[1] = gen.imgLine.p2().y() - gen.imgLine.p1().y();
		homog_m1.s[0] = gen.pln2img.mat()(0, 0);
		homog_m1.s[1] = gen.pln2img.mat()(1, 0);
		homog_m2.s[0] = gen.pln2img.mat()(0, 1);
		homog_m2.s[1] = gen.pln2img.mat()(1, 1);
		this->dst_y_range.s[0] = dst_y_range.first;
		this->dst_y_range.s[1] = dst_y_range.second;
	}
};

} // anonymous namespace


QImage dewarp(
	cl::CommandQueue const& command_queue,
	cl::Program const& program,
	QImage const& src, QSize const& dst_size,
	dewarping::CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& background_color,
	float min_density, float max_density,
	QSizeF const& min_mapping_area)
{
	cl::Context const context = command_queue.getInfo<CL_QUEUE_CONTEXT>();
	cl::Device const device = command_queue.getInfo<CL_QUEUE_DEVICE>();
	size_t const max_const_mem = device.getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>();
	size_t const max_wg_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
	size_t const v_wg_size = std::sqrt((double)max_wg_size);
	size_t const h_wg_size = max_wg_size / v_wg_size;

	auto const adapted = adaptImage(src, background_color);

	// Create source and destination images on the device.
	cl::Image2D src_image(
		context, CL_MEM_READ_ONLY,
		adapted.clFormat, src.width(), src.height()
	);
	cl::Image2D dst_image(
		context, CL_MEM_WRITE_ONLY,
		adapted.clFormat, dst_size.width(), dst_size.height()
	);

	std::vector<cl::Event> events;
	cl::Event evt;

	// Write the source image to device memory.
	std::array<size_t, 3> const origin{0, 0, 0};
	std::array<size_t, 3> region{(size_t)src.width(), (size_t)src.height(), 1};
	command_queue.enqueueWriteImage(
		src_image, CL_FALSE, origin, region, adapted.image.bytesPerLine(), 0,
		(void*)adapted.image.bits(), &events, &evt
	);
	indicateCompletion(&events, evt);

	// Create host and device buffers for storing Generatrix structures.
	// Because of constant memory size limitations, we may need several passes.
	// Across passes we need a total of dst_width + 1 Generatrix instances,
	// indexed from [0] to [dst_width].
	int const range_buffer_elements = std::min<int>(
		max_const_mem / sizeof(Generatrix), dst_size.width() + 1
	);
	size_t const range_buffer_size = range_buffer_elements * sizeof(Generatrix);
	cl::Buffer const range_device_buffer(context, CL_MEM_READ_ONLY, range_buffer_size);
	std::vector<Generatrix> range_host_buffer(dst_size.width() + 1);

	float const model_domain_left = model_domain.left();
	float const model_x_scale = 1.f / model_domain.width();

	float const model_domain_top = model_domain.top();
	float const model_domain_height = model_domain.height();
	float const model_y_scale = 1.f / model_domain_height;

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

			std::pair<int, int> dst_y_range(0, dst_size.height() - 1); // Inclusive.

			// Called for points where pixel density reaches the lower or upper threshold.
			auto const processCriticalPoint =
					[&generatrix, &dst_y_range, model_domain_top, model_domain_height]
					(double model_y, bool upper_threshold) {

				if (!generatrix.pln2img.mirrorSide(model_y)) {
					double const dst_y = model_domain_top + model_y * model_domain_height;
					double const second_deriv = generatrix.pln2img.secondDerivativeAt(model_y);
					if (std::signbit(second_deriv) == upper_threshold) {
						if (dst_y > dst_y_range.first) {
							dst_y_range.first = std::min((int)std::ceil(dst_y), dst_y_range.second);
						}
					} else {
						if (dst_y < dst_y_range.second) {
							dst_y_range.second = std::max((int)std::floor(dst_y), dst_y_range.first);
						}
					}
				}
			};

			double const recip_len = 1.0 / generatrix.imgLine.length();

			generatrix.pln2img.solveForDeriv(
				min_density * recip_len,
				[processCriticalPoint](double model_y) {
					processCriticalPoint(model_y, /*upper_threshold=*/false);
				}
			);

			generatrix.pln2img.solveForDeriv(
				max_density * recip_len,
				[processCriticalPoint](double model_y) {
					processCriticalPoint(model_y, /*upper_threshold=*/true);
				}
			);

			range_host_buffer[dst_x].set(generatrix, dst_y_range);
		}

		// Copy generatrix_host_buffer to generatrix_device_buffer.
		command_queue.enqueueWriteBuffer(
			range_device_buffer, CL_FALSE, 0, (range_end + 1 - range_begin)*sizeof(Generatrix),
			range_host_buffer.data() + range_begin, &events, &evt
		);
		indicateCompletion(&events, evt);

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
		});
		kernel.setArg(idx++, cl_float2{
			(float)min_mapping_area.width(), (float)min_mapping_area.height()
		});

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NDRange(range_begin, 0),
			cl::NDRange(
				thisOrNextMultipleOf(range_end - range_begin, h_wg_size),
				thisOrNextMultipleOf(dst_size.height(), v_wg_size)
			),
			cl::NDRange(h_wg_size, v_wg_size), &events, &evt
		);
		indicateCompletion(&events, evt);
	}

	QImage dst(dst_size, adapted.image.format());
	if (dst.format() == QImage::Format_Indexed8) {
		dst.setColorTable(createGrayscalePalette());
	}
	badAllocIfNull(dst);
	region[0] = dst.width();
	region[1] = dst.height();
	region[2] = 1;
	command_queue.enqueueReadImage(
		dst_image, CL_FALSE, origin, region, dst.bytesPerLine(), 0, dst.bits(), &events, &evt
	);
	indicateCompletion(&events, evt);

	cl::WaitForEvents(events);

	return dst;
}

} // namespace opencl
