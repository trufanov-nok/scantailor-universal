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

#ifndef OPENCL_HIT_MISS_REPLACE_H_
#define OPENCL_HIT_MISS_REPLACE_H_

#include "OpenCLGrid.h"
#include "imageproc/BWColor.h"
#include <QPoint>
#include <CL/cl2.hpp>
#include <cstdint>
#include <vector>

namespace opencl
{

/**
 * @brief Performs a hit-miss matching operation.
 *
 * @see imageproc::hitMissMatch()
 *
 * @param command_queue
 * @param program
 * @param src The source image.
 * @param src_pixel_width The width of @p src in pixels. src.width() returns width in words.
 * @param src_surroundings The color that is assumed to be outside of @p src boundaries.
 * @param dst The destination image.
 * @param hits Offsets to hit positions relative to the origin point.
 * @param misses Offsets to miss positions relative to the origin point.
 * @param dependencies If provided, OpenCL operations enqueued by this function become
 *        dependent on this set of events to complete.
 * @param completion_set If provided, a set of events associated with completion of this
 *        operation is returned through this variable. It's allowed to pass a pointer to
 *        the same vector of events as both @p dependencies and @p completion_set.
 */
void hitMissMatch(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<uint32_t> const& src, int const src_pixel_width,
	imageproc::BWColor src_surroundings, OpenCLGrid<uint32_t> const& dst,
	std::vector<QPoint> const& hits, std::vector<QPoint> const& misses,
	std::vector<cl::Event> const* dependencies = nullptr,
	std::vector<cl::Event>* completion_set = nullptr);

/**
 * @brief Switches some pixels between black and white, but only when a specified
 *        pattern of pixels is encountered.
 *
 * @see imageproc::hitMissReplaceInPlace()
 *
 * @param command_queue
 * @param program
 * @param image The image to be modified.
 * @param image_pixel_width The width of @p image in pixels. image.width() returns width in words.
 * @param image_surroundings The color that is assumed to be outside of @p image boundaries.
 * @param tmp A temporary image of identical or higher dimensions with respect to @p image.
 * @param pattern
 * @param pattern_width
 * @param pattern_height
 * @param dependencies If provided, OpenCL operations enqueued by this function become
 *        dependent on this set of events to complete.
 * @param completion_set If provided, a set of events associated with completion of this
 *        operation is returned through this variable. It's allowed to pass a pointer to
 *        the same vector of events as both @p dependencies and @p completion_set.
 */
void hitMissReplaceInPlace(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<uint32_t> const& image, int image_pixel_width,
	imageproc::BWColor image_surroundings, OpenCLGrid<uint32_t> const& tmp,
	char const* pattern, int pattern_width, int pattern_height,
	std::vector<cl::Event> const* dependencies = nullptr,
	std::vector<cl::Event>* completion_set = nullptr);

} // namespace opencl

#endif
