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

#ifndef OPENCL_AFFINE_TRANSFORM_H_
#define OPENCL_AFFINE_TRANSFORM_H_

#include <QImage>
#include <QSizeF>
#include <QRect>
#include <QRectF>
#include <QTransform>
#include <QColor>
#include <CL/cl2.hpp>

namespace imageproc
{
	class OutsidePixels;
}

namespace opencl
{

/** @see imageproc::affineTransform() */
QImage affineTransform(
	cl::CommandQueue const& command_queue,
	cl::Program const& program,
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, imageproc::OutsidePixels const& outside_pixels,
	QSizeF const& min_mapping_area = QSizeF(0.9, 0.9));

} // namespace opencl

#endif
