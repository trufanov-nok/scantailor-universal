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

#ifndef OPENCL_RENDER_POLYNOMIAL_SURFACE_H_
#define OPENCL_RENDER_POLYNOMIAL_SURFACE_H_

#include "imageproc/GrayImage.h"
#include <Eigen/Core>
#include <CL/cl2.hpp>

namespace opencl
{

/**
 * @brief Render a polynomial sorface into a grayscale image.
 *
 * @param command_queue The command queue to use.
 * @param program Pre-built kernel-space code.
 * @param width The width of the destination image.
 * @param height The height of the destination image.
 * @param coeffs The polynomial coefficients. @see PolynomialSurface::coeffs().
 * @return The rendered surface.
 */
imageproc::GrayImage renderPolynomialSurface(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	int width, int height, Eigen::MatrixXd const& coeffs);

} // namespace opencl

#endif
