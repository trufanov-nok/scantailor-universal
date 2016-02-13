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

#ifndef OPENCL_BINARY_RASTER_OP_H_
#define OPENCL_BINARY_RASTER_OP_H_

#include "OpenCLGrid.h"
#include <QRect>
#include <CL/cl2.hpp>
#include <cstdint>
#include <vector>
#include <string>

namespace opencl
{

/**
 * @brief Performs a bitwise operation on rectangular regions of two images.
 *
 * One of those images (the @p dst_grid) is modified as a result.
 *
 * @note The @p src_grid and @p dst_grid dimensions are in 32-bit words while @p src_pixel_rect
 * and @p dst_pixel_rect dimensions are in pixels.
 *
 * @see binary_raster_op.cl
 */
void binaryRasterOp(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	std::string const& kernel_name_base,
	OpenCLGrid<uint32_t> const& src_grid, QRect const& src_pixel_rect,
	OpenCLGrid<uint32_t> const& dst_grid, QRect const& dst_pixel_rect,
	std::vector<cl::Event> const* dependencies = nullptr,
	std::vector<cl::Event>* completion_set = nullptr);

} // namespace opencl

#endif
