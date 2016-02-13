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

#ifndef OPENCL_DEWARP_H_
#define OPENCL_DEWARP_H_

#include "dewarping/CylindricalSurfaceDewarper.h"
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QRectF>
#include <QColor>
#include <CL/cl2.hpp>

namespace opencl
{

/** @see AcceleratedOperations::dewarp() */
QImage dewarp(
	cl::CommandQueue const& command_queue,
	cl::Program const& program,
	QImage const& src, QSize const& dst_size,
	dewarping::CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& background_color,
	float min_density, float max_density,
	QSizeF const& min_mapping_area);

} // namespace opencl

#endif
