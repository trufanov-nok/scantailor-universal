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
#include "imageproc/BinaryImage.h"
#include "imageproc/BWColor.h"
#include "imageproc/GrayImage.h"
#include "imageproc/PolynomialSurface.h"
#include "dewarping/CylindricalSurfaceDewarper.h"
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QRect>
#include <QRectF>
#include <QColor>
#include <QTransform>
#include <vector>
#include <cstdint>
#include <utility>

namespace imageproc
{
	class OutsidePixels;
}

/**
 * @brief A collection of heavy-weight operations that may be accelerated by
 *        something like OpenCL.
 *
 * @note Methods of this class may be called from multiple threads concurrently.
 * @note This interface is completely inline and is not DLL-exported. That's done
 *       to avoid a circular dependency involving imageproc and acceleration modules.
 */
class AcceleratableOperations
{
public:
	virtual ~AcceleratableOperations() {}

	/**
	 * @brief Applies an axis-aligned 2D gaussian filter to a grid of float values.
	 *
	 * @param src The grid to apply gaussian filtering to.
	 * @param h_sigma The standard deviation in horizontal direction.
	 * @param h_sigma The standard deviation in vertical direction.
	 * @return The blurred grid.
	 */
	virtual Grid<float> gaussBlur(
		Grid<float> const& src, float h_sigma, float v_sigma) const = 0;

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
	 *        normalized, that is directions[i].norm() has to be 1. The maximum size of
	 *        this vector is 256, as we return a map of indexes into this vector as
	 *        Grid<uint8_t>.
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
	 * @return A pair consisting of the filtered image and a map of directions for each pixel.
	 *         A direction is represented by an index into the @p directions argument.
	 *         Because the index is uint8_t, the maximum length of the @p directions argument
	 *         is 256.
	 */
	virtual std::pair<Grid<float>, Grid<uint8_t>> textFilterBank(
		Grid<float> const& src, std::vector<Vec2f> const& directions,
		std::vector<Vec2f> const& sigmas, float shoulder_length) const = 0;

	/**
	 * @brief Performs a dewarping operation.
	 *
	 * @param src The warped image. Can't be null.
	 * @param dst_size The dimensions of the output image.
	 * @param distortion_model A curved quadrilateral in source image coordinates.
	 * @param model_domain The rectangle in output image coordinates that the
	 *        curved quadrilateral will map to.
	 * @param background_color The color to use for pixels outside of source image boundaries.
	 * @param min_density The minimum allowed ratio of a distance in src image coordinates
	 *        over the corresponding distance in normalized dewarped coordinates.
	 *        For interpretation of normalized dewarped coordinates, see comments at the top
	 *        of CylindricalSurfaceDewarper.cpp. Dewarped pixels where density is less than
	 *        min_density will be treated the same way as pixels mapping outside of the
	 *        warped image.
	 * @param max_density The maximum allowed ratio of a distance in src image coordinates
	 *        over the corresponding distance in normalized dewarped coordinates.
	 *        For interpretation of normalized dewarped coordinates, see comments at the top
	 *        of CylindricalSurfaceDewarper.cpp. Dewarped pixels where density is more than
	 *        max_density will be treated the same way as pixels mapping outside of the
	 *        warped image.
	 * @param min_mapping_area Defines the minimum rectangle in the source image
	 *        that maps to a destination pixel.  This can be used to control
	 *        smoothing.
	 * @return The dewarped image.
	 */
	virtual QImage dewarp(
		QImage const& src, QSize const& dst_size,
		dewarping::CylindricalSurfaceDewarper const& distortion_model,
		QRectF const& model_domain, QColor const& background_color,
		float min_density, float max_density,
		QSizeF const& min_mapping_area = QSizeF(0.9, 0.9)) const = 0;

	/**
	 * \brief Apply an affine transformation to the image.
	 *
	 * \param src The source image.
	 * \param xform The transformation from source to destination.
	 *        Only affine transformations are supported.
	 * \param dst_rect The area in source image coordinates to return
	 *        as a destination image.
	 * \param outside_pixels Configures handling of pixels outside of the source image.
	 * \param min_mapping_area Defines the minimum rectangle in the source image
	 *        that maps to a destination pixel.  This can be used to control
	 *        smoothing.
	 * \return The transformed image.  It's format may differ from the
	 *         source image format, for example Format_Indexed8 may
	 *         be transformed to Format_RGB32, if the source image
	 *         contains colors other than shades of gray.
	 */
	virtual QImage affineTransform(
		QImage const& src, QTransform const& xform,
		QRect const& dst_rect, imageproc::OutsidePixels const& outside_pixels,
		QSizeF const& min_mapping_area = QSizeF(0.9, 0.9)) const = 0;

	/**
	 * @brief Render a polynomial sorface into a grayscale image.
	 *
	 * @param surface The surface to render.
	 * @param width The width of the destination image.
	 * @param height The height of the destination image.
	 * @return The rendered surface.
	 */
	virtual imageproc::GrayImage renderPolynomialSurface(
		imageproc::PolynomialSurface const& surface, int width, int height) = 0;

	/**
	 * @brief Performs a grayscale smoothing using the Savitzky-Golay method.
	 *
	 * @see imageproc::savGolKernel()
	 */
	virtual imageproc::GrayImage savGolFilter(
		imageproc::GrayImage const& src, QSize const& window_size,
		int hor_degree, int vert_degree) = 0;

	/**
	 * @brief Performs a series of match-replace operations on a binary image.
	 *
	 * @see imageproc::hitMissReplaceInPlace()
	 */
	virtual void hitMissReplaceInPlace(
		imageproc::BinaryImage& img, imageproc::BWColor img_surroundings,
		std::vector<Grid<char>> const& patterns) = 0;
};

#endif
