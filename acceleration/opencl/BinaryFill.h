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

#ifndef OPENCL_BINARY_FILL_H_
#define OPENCL_BINARY_FILL_H_

#include "OpenCLGrid.h"
#include "imageproc/BWColor.h"
#include <QRect>
#include <CL/cl2.hpp>
#include <cstdint>
#include <vector>

namespace opencl
{

/**
 * @brief Fills a rectanglular area of a bit-packed binary image with black or white.
 *
 * @note The @p grid dimensions are in words while @p pixel_rect dimensions are in pixels.
 */
void binaryFillRect(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<uint32_t> const& grid, QRect const& pixel_rect, imageproc::BWColor fill_color,
	std::vector<cl::Event> const* dependencies = nullptr,
	std::vector<cl::Event>* completion_set = nullptr);

/**
 * @brief Fills the area inside outer_pixel_rect but not inside inner_pixel_rect.
 *
 * The rectangles are allowed to overlap. Areas of the inner rectangle that are outside
 * of the outer rectangle won't be filled. On the other hand, the outer rectangle is not
 * allowed to reach outside of the grid.
 *
 * @note The @p grid dimensions are in words while @p outer_pixel_rect and @p inner_pixel_rect
 * dimensions are in pixels.
 */
void binaryFillFrame(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<uint32_t> const& grid, QRect const& outer_pixel_rect,
	QRect const& inner_pixel_rect, imageproc::BWColor fill_color,
	std::vector<cl::Event> const* dependencies = nullptr,
	std::vector<cl::Event>* completion_set = nullptr);

} // namespace opencl

#endif
