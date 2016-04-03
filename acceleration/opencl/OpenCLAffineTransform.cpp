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

#include "OpenCLAffineTransform.h"
#include "Utils.h"
#include "imageproc/AffineTransform.h"
#include "imageproc/BadAllocIfNull.h"
#include "imageproc/Grayscale.h"
#include <QPolygonF>
#include <QColor>
#include <QSysInfo>
#include <QDebug>
#include <CL/cl2.hpp>
#include <utility>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <vector>

using namespace imageproc;

namespace opencl
{

namespace
{

struct AdaptedImage
{
	QImage image;
	cl::ImageFormat clFormat;
};

AdaptedImage adaptImage(QImage const& src, imageproc::OutsidePixels const& outside_pixels)
{
	AdaptedImage adapted;

	bool const have_bg_color = bool(outside_pixels.flags() & OutsidePixels::COLOR);

	auto is_opaque_gray = [](QRgb rgba) {
		return qAlpha(rgba) == 0xff && qRed(rgba) == qBlue(rgba) && qRed(rgba) == qGreen(rgba);
	};

	switch (src.format()) {
		case QImage::Format_Invalid:
			throw std::runtime_error("Null image passed to opencl::dewarp()");
		case QImage::Format_Indexed8:
		case QImage::Format_Mono:
		case QImage::Format_MonoLSB:
			if (src.allGray() && (!have_bg_color || is_opaque_gray(outside_pixels.rgba()))) {
				adapted.image = toGrayscale(src);
				adapted.clFormat = cl::ImageFormat(CL_R, CL_UNORM_INT8);
				break;
			}
			// fall through
		default:
			if (!src.hasAlphaChannel() && (!have_bg_color || qAlpha(outside_pixels.rgba()) == 0xff)) {
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

QSizeF calcSrcUnitSize(QTransform const& xform, QSizeF const& min)
{
	// Imagine a rectangle of (0, 0, 1, 1), except we take
	// centers of its edges instead of its vertices.
	QPolygonF dst_poly;
	dst_poly.push_back(QPointF(0.5, 0.0));
	dst_poly.push_back(QPointF(1.0, 0.5));
	dst_poly.push_back(QPointF(0.5, 1.0));
	dst_poly.push_back(QPointF(0.0, 0.5));

	QPolygonF const src_poly(xform.map(dst_poly));

	qreal left = src_poly[0].x();
	qreal right = left;
	qreal top = src_poly[0].y();
	qreal bottom = top;
	for (int i = 1; i < 4; ++i) {
		left = std::min<qreal>(left, src_poly[i].x());
		right = std::max<qreal>(right, src_poly[i].x());
		top = std::min<qreal>(top, src_poly[i].y());
		bottom = std::max<qreal>(bottom, src_poly[i].y());
	}

	return QSizeF(
		std::max<qreal>(min.width(), right - left),
		std::max<qreal>(min.height(), bottom - top)
	);
}

} // anonymous namespace


QImage affineTransform(
	cl::CommandQueue const& command_queue,
	cl::Program const& program,
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, imageproc::OutsidePixels const& outside_pixels,
	QSizeF const& min_mapping_area)
{
	cl::Context const context = command_queue.getInfo<CL_QUEUE_CONTEXT>();
	cl::Device const device = command_queue.getInfo<CL_QUEUE_DEVICE>();
	size_t const max_wg_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
	size_t const v_wg_size = std::sqrt((double)max_wg_size);
	size_t const h_wg_size = max_wg_size / v_wg_size;

	QTransform inv_xform;
	inv_xform.translate(dst_rect.x(), dst_rect.y());
	inv_xform *= xform.inverted();

	auto const adapted = adaptImage(src, outside_pixels);
	QSizeF const unit_size(calcSrcUnitSize(inv_xform, min_mapping_area));

	std::vector<cl::Event> events;
	cl::Event evt;

	// Create source and destination images on the device.
	cl::Image2D src_image(
		context, CL_MEM_READ_ONLY,
		adapted.clFormat, src.width(), src.height()
	);
	cl::Image2D dst_image(
		context, CL_MEM_WRITE_ONLY,
		adapted.clFormat, dst_rect.width(), dst_rect.height()
	);

	// Write the source image to device memory.
	std::array<size_t, 3> const origin{0, 0, 0};
	std::array<size_t, 3> region{(size_t)src.width(), (size_t)src.height(), 1};
	command_queue.enqueueWriteImage(
		src_image, CL_FALSE, origin, region, adapted.image.bytesPerLine(), 0,
		(void*)adapted.image.bits(), &events, &evt
	);
	indicateCompletion(&events, evt);

	cl::Kernel kernel(program, "affine_transform");
	int idx = 0;
	kernel.setArg(idx++, src_image);
	kernel.setArg(idx++, dst_image);
	kernel.setArg(idx++, cl_float2{(float)inv_xform.m11(), (float)inv_xform.m12()});
	kernel.setArg(idx++, cl_float2{(float)inv_xform.m21(), (float)inv_xform.m22()});
	kernel.setArg(idx++, cl_float2{(float)inv_xform.dx(), (float)inv_xform.dy()});
	kernel.setArg(idx++, cl_float2{(float)unit_size.width(), (float)unit_size.height()});
	kernel.setArg(idx++, cl_int(outside_pixels.flags()));
	kernel.setArg(idx++, cl_float4{
		(float)qRed(outside_pixels.rgba()) / 255.f,
		(float)qGreen(outside_pixels.rgba()) / 255.f,
		(float)qBlue(outside_pixels.rgba()) / 255.f,
		(float)qAlpha(outside_pixels.rgba()) / 255.f
	});

	command_queue.enqueueNDRangeKernel(
		kernel,
		cl::NullRange,
		cl::NDRange(
			thisOrNextMultipleOf(dst_rect.width(), h_wg_size),
			thisOrNextMultipleOf(dst_rect.height(), v_wg_size)
		),
		cl::NDRange(h_wg_size, v_wg_size),
		&events, &evt
	);
	indicateCompletion(&events, evt);

	QImage dst(dst_rect.size(), adapted.image.format());
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
