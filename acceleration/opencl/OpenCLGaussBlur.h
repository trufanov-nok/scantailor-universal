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

#ifndef OPENCL_GAUSS_BLUR_H_
#define OPENCL_GAUSS_BLUR_H_

#include "OpenCLGrid.h"
#include "VecNT.h"
#include <CL/cl2.hpp>
#include <vector>

namespace opencl
{

/**
 * @brief Applies an axis-aligned 2D gaussian filter to a float-valued grid.
 *
 * @param command_queue The command queue to use.
 * @param program The pre-built OpenCL code.
 * @param src_grid The grid to apply gaussian filtering to.
 * @param h_sigma The standard deviation in horizontal direction.
 * @param v_sigma The standard deviation in vertical direction.
 * @param dependencies If provided, the kernels enqueued by this function will be
 *        made to depend on the events provided.
 * @param completion_set If provided, used to return a set of events indicating
 *        the completion of all asynchronous operations initiated by this function.
 * @return The filtered grid.
 */
OpenCLGrid<float> gaussBlur(
	cl::CommandQueue const& command_queue,
	cl::Program const& program,
	OpenCLGrid<float> const& src_grid,
	float h_sigma, float v_sigma,
	std::vector<cl::Event> const* dependencies = nullptr,
	std::vector<cl::Event>* completion_set = nullptr);

/**
 * @brief Applies an oriented 2D gaussian filter to a float-valued grid.
 *
 * @param command_queue The command queue to use.
 * @param program The pre-built OpenCL code.
 * @param src_grid The grid to apply gaussian filtering to.
 * @param dir_x (dir_x, dir_y) vector is a principal direction of a gaussian.
 *        The other principal direction is the one orthogonal to (dir_x, dir_y).
 *        The (dir_x, dir_y) vector doesn't have to be normalized, yet it can't
 *        be a zero vector.
 * @param dir_y @see dir_x
 * @param dir_sigma The standard deviation in (dir_x, dir_y) direction.
 * @param ortho_dir_sigma The standard deviation in a direction orthogonal
 *        to (dir_x, dir_y).
 * @param dependencies If provided, the kernels enqueued by this function will be
 *        made to depend on the events provided.
 * @param completion_set If provided, used to return a set of events indicating
 *        the completion of all asynchronous operations initiated by this function.
 * @return The filtered grid.
 */
OpenCLGrid<float> anisotropicGaussBlur(
	cl::CommandQueue const& command_queue,
	cl::Program const& program,
	OpenCLGrid<float> const& src_grid,
	float dir_x, float dir_y,
	float dir_sigma, float ortho_dir_sigma,
	std::vector<cl::Event> const* dependencies = nullptr,
	std::vector<cl::Event>* completion_set = nullptr);

} // namespace opencl

#endif
