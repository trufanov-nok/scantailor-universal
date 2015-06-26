/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef ACCELERATABLE_OPERATIONS_H_
#define ACCELERATABLE_OPERATIONS_H_

#include "Grid.h"
#include "VecNT.h"
#include <vector>

/**
 * @brief A collection of heavy-weight operations that may be accelerated by
 *        something like OpenCL.
 *
 * @note Methods of this class may be called from multiple threads concurrently.
 */
class AcceleratableOperations
{
public:
	virtual ~AcceleratableOperations() {}

	/**
	 * @brief Applies an oriented 2D gaussian filter to a grid of float values.
	 *
	 * @param src The grid to apply gaussian filtering to.
	 * @param dir_x (dir_x, dir_y) vector is a principal direction of a gaussian.
	 *        The other principal direction is the one orthogonal to (dir_x, dir_y).
	 *        The (dir_x, dir_y) vector doesn't have to be normalized, yet it can't
	 *        be a zero vector.
	 * @param dir_y @see dir_x
	 * @param dir_sigma The standard deviation in (dir_x, dir_y) direction.
	 * @param ortho_dir_sigma The standard deviation in a direction orthogonal
	 * @return The blurred grid.
	 */
	virtual Grid<float> anisotropicGaussBlur(
		Grid<float> const& src, float dir_x, float dir_y,
		float dir_sigma, float ortho_dir_sigma) const = 0;

	/**
	 * @brief Perform anisotropic gaussian filtering at multiple orientations and scales,
	 *        capturing the maximum response across all combinations of those.
	 *
	 * @param src The image to apply gaussian filtering to.
	 * @param directions The list of blurring directions to try. Directions have to be
	 *        normalized, that is directions[i].norm() has to be 1.
	 * @param sigmas The list of blur intensities to try. The first element in a Vec2f
	 *        is the standard deviation along the specified direction. The second element
	 *        is the standard deviation in the orthogonal direction.
	 * @param shoulder_length When combining gaussians with different directions and
	 *        standard deviations, we do something a bit more compex than taking per-pixel
	 *        minimum or maximum across different scales and orientations. Instead, we take
	 *        the per-pixel maximum of the following quantity:
	 *        @code
	 *        0.5 * (above_px + below_px) - origin_px
	 *        @endcode
	 *        Where above_px and below_px are pixels sampled from the blurred image at
	 *        locations:
	 *        @code
	 *        above_pos = origin_pos - orthogonal(direction) * ortho_dir_sigma * shoulder_length;
	 *        below_pos = origin_pos + orthogonal(direction) * ortho_dir_sigma * shoulder_length;
	 *        @endcode
	 *        This parameter specifies the shoulder length in standard deviation units
	 *        in direction orthogonal to the principal direction of the gaussian.
	 * @return A filtered image.
	 */
	virtual Grid<float> textFilterBank(Grid<float> const& src,
		std::vector<Vec2f> const& directions,
		std::vector<Vec2f> const& sigmas, float shoulder_length) const = 0;
};

#endif
