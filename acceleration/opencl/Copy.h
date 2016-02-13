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

#ifndef OPENCL_TRANSPOSE_H_
#define OPENCL_TRANSPOSE_H_

#include "OpenCLGrid.h"
#include <CL/cl2.hpp>

namespace opencl
{

/**
 * @brief Copy the grid of float values but not the padding.
 *
 * @param command_queue The command queue to use.
 * @param program Pre-built kernel-space code.
 * @param src The source grid.
 * @param dst_padding The padding the destination grid will have.
 *        The padding will be left uninitialized.
 * @param dependencies If provided, the kernels enqueued by this function will be
 *        made to depend on the events provided.
 * @param completion_set If provided, used to return a set of events indicating
 *        the completion of all asynchronous operations initiated by this function.
 * @return The copy.
 */
OpenCLGrid<float> copy(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<float> const& src, int dst_padding,
	std::vector<cl::Event> const* dependencies = nullptr,
	std::vector<cl::Event>* completion_set = nullptr);

} // namespace opencl

#endif
