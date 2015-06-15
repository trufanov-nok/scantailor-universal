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

/* References:
 * [1] Young, Ian T., and Lucas J. Van Vliet. "Recursive implementation
 * of the Gaussian filter." Signal processing 44.2 (1995): 139-151.
 * [2] Triggs, Bill, and Michael Sdika. "Boundary conditions for
 * Young-van Vliet recursive filtering." Signal Processing,
 * IEEE Transactions on 54.6 (2006): 2365-2367.
 * [3] Geusebroek, J-M., Arnold WM Smeulders, and Joost Van De Weijer.
 * "Fast anisotropic gauss filtering." Image Processing, IEEE Transactions
 * on 12.8 (2003): 938-943.
 *
 * Paper [1] approximates convolution with a 1D gaussian by applying
 * a certain LTI system to the input signal x, producing an intermediate
 * signal w, and then applying the same system to time-reversed signal w
 * to produce a time-reversed output signal y.
 * The LTI system in [1] looks like:
 * y[n] = B*x[n] + (b_1*y[n - 1] + b_2*y[n - 2] + b_3*y[n - 3]) / b_0
 *
 * Paper [2] modifies the above system to be:
 * y[n] = x[n] + a_1*y[n - 1] + a_2*y[n - 2] + a_3*y[n - 3]
 *
 * The removal of B is compensated by scaling the final output by B^2
 * while a_i coefficients are set to b_i / b_0.
 *
 * Our implementation follows the scheme used in [2].
 */

#ifndef IMAGEPROC_GAUSSBLUR_H_
#define IMAGEPROC_GAUSSBLUR_H_

#include "Grid.h"
#include "ValueConv.h"
#include "GridAccessor.h"
#include "RasterOpGeneric.h"
#include <QSize>
#include <boost/scoped_array.hpp>
#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <stdexcept>

namespace imageproc
{

class GrayImage;

/**
 * \brief Applies gaussian blur on a GrayImage.
 *
 * \param src The image to apply gaussian blur to.
 * \param h_sigma The standard deviation in horizontal direction.
 * \param v_sigma The standard deviation in vertical direction.
 * \return The blurred image.
 */
GrayImage gaussBlur(GrayImage const& src, float h_sigma, float v_sigma);

/**
 * \brief Applies an oriented gaussian blur on a GrayImage.
 *
 * \param src The image to apply gaussian blur to.
 * \param dir_x (dir_x, dir_y) vector is a principal direction of a gaussian.
 *        The other principal direction is the one orthogonal to (dir_x, dir_y).
 *        The (dir_x, dir_y) vector doesn't have to be normalized, yet it can't
 *        be a zero vector.
 * \param dir_y @see dir_x
 * \param dir_sigma The standard deviation in (dir_x, dir_y) direction.
 * \param ortho_dir_sigma The standard deviation in a direction orthogonal
 *        to (dir_x, dir_y).
 * \return The blurred image.
 */
GrayImage anisotropicGaussBlur(
	GrayImage const& src, float dir_x, float dir_y,
	float dir_sigma, float ortho_dir_sigma);

/**
 * \brief Applies a 2D gaussian filter to an arbitrary data grid.
 *
 * \param size Data grid dimensions.
 * \param h_sigma The standard deviation in horizontal direction.
 * \param v_sigma The standard deviation in vertical direction.
 * \param input A random access iterator (usually a pointer)
 *        to the beginning of input data.
 * \param input_stride The distance (in terms of iterator difference)
 *        from an input grid cell to the one directly below it.
 * \param float_reader A functor to convert whatever value corresponds to *input
 *        into a float.  Consider using one of the functors from ValueConv.h
 *        The functor will be called like this:
 * \code
 * FloatReader const reader = ...;
 * float const val = reader(*(input + x));
 * \endcode
 * \param output A random access iterator (usually a pointer)
 *        to the beginning of output data.  Output may point to the same
 *        memory as input.
 * \param output_stride The distance (in terms of iterator difference)
 *        from an output grid cell to the one directly below it.
 * \param float_writer A functor that takes a float value, optionally
 *        converts it into another type and updates an output item.
 *        The functor will be called like this:
 * \code
 * FloatWriter const writer = ...;
 * float const val = ...;
 * writer(*(output + x), val);
 * \endcode
 * Consider using boost::lambda, possibly in conjunction with one of the functors
 * from ValueConv.h:
 * \code
 * using namespace boost::lambda;
 *
 * // Just copying.
 * gaussBlurGeneric(..., _1 = _2);
 *
 * // Convert to uint8_t, with rounding and clipping.
 * gaussBlurGeneric(..., _1 = bind<uint8_t>(RoundAndClipValueConv<uint8_t>(), _2));
 * \endcode
 */
template<typename SrcIt, typename DstIt, typename FloatReader, typename FloatWriter>
void gaussBlurGeneric(QSize size, float h_sigma, float v_sigma,
					  SrcIt input, int input_stride, FloatReader float_reader,
					  DstIt output, int output_stride, FloatWriter float_writer);

/**
 * \brief Applies an oriented 2D gaussian filter to an arbitrary data grid.
 *
 * \param size Data grid dimensions.
 * \param dir_x (dir_x, dir_y) vector is a principal direction of a gaussian.
 *        The other principal direction is the one orthogonal to (dir_x, dir_y).
 *        The (dir_x, dir_y) vector doesn't have to be normalized, yet it can't
 *        be a zero vector.
 * \param dir_y @see dir_x
 * \param dir_sigma The standard deviation in (dir_x, dir_y) direction.
 * \param ortho_dir_sigma The standard deviation in a direction orthogonal
 *        to (dir_x, dir_y).
 * \param input A random access iterator (usually a pointer)
 *        to the beginning of input data.
 * \param input_stride The distance (in terms of iterator difference)
 *        from an input grid cell to the one directly below it.
 * \param float_reader A functor to convert whatever value corresponds to *input
 *        into a float.  Consider using one of the functors from ValueConv.h
 *        The functor will be called like this:
 * \code
 * FloatReader const reader = ...;
 * float const val = reader(*(input + x));
 * \endcode
 * \param output A random access iterator (usually a pointer)
 *        to the beginning of output data.  Output may point to the same
 *        memory as input.
 * \param output_stride The distance (in terms of iterator difference)
 *        from an output grid cell to the one directly below it.
 * \param float_writer A functor that takes a float value, optionally
 *        converts it into another type and updates an output item.
 *        The functor will be called like this:
 * \code
 * FloatWriter const writer = ...;
 * float const val = ...;
 * writer(*(output + x), val);
 * \endcode
 * Consider using boost::lambda, possibly in conjunction with one of the functors
 * from ValueConv.h:
 * \code
 * using namespace boost::lambda;
 *
 * // Just copying.
 * anisotropicGaussBlurGeneric(..., _1 = _2);
 *
 * // Convert to uint8_t, with rounding and clipping.
 * anisotropicGaussBlurGeneric(..., _1 = bind<uint8_t>(RoundAndClipValueConv<uint8_t>(), _2));
 * \endcode
 */
template<typename SrcIt, typename DstIt, typename FloatReader, typename FloatWriter>
void anisotropicGaussBlurGeneric(
	QSize size, float dir_x, float dir_y,
	float dir_sigma, float ortho_dir_sigma,
	SrcIt input, int input_stride, FloatReader float_reader,
	DstIt output, int output_stride, FloatWriter float_writer);

namespace gauss_blur_impl
{

class FilterParams
{
public:
	float a1, a2, a3, B;

	FilterParams(float sigma);
private:
	float sigmaToQ(float sigma);
};

class AnisotropicParams
{
public:
	float sigma_x, sigma_phi, cot_phi;

	AnisotropicParams(
		float dir_x, float dir_y,
		float dir_sigma, float ortho_dir_sigma);
};

void calcBackwardPassInitialConditions(
	FilterParams const& p, float* w_end, float future_signal_val);

void initPaddingLayers(Grid<float>& intermediate_image);

} // namespace gauss_blur_impl

template<typename SrcIt, typename DstIt, typename FloatReader, typename FloatWriter>
void gaussBlurGeneric(QSize const size, float const h_sigma, float const v_sigma,
					  SrcIt const input, int const input_stride, FloatReader const float_reader,
					  DstIt const output, int const output_stride, FloatWriter const float_writer)
{
	using namespace gauss_blur_impl;

	if (size.isEmpty()) {
		return;
	}

	if (h_sigma < 0 || v_sigma < 0) {
		throw std::invalid_argument("gaussBlur: stddev can't be negative");
	} else if (h_sigma < 1e-2f && v_sigma < 1e-2f) {
		return;
	}

	int const width = size.width();
	int const height = size.height();
	int const width_height_max = width > height ? width : height;

	boost::scoped_array<float> const w(new float[3 + width_height_max + 3]);
	Grid<float> intermediate_image(width, height, /*padding=*/0);
	int const intermediate_stride = intermediate_image.stride();

	{ // Vertical pass.
		gauss_blur_impl::FilterParams const p(v_sigma);
		float const B2 = p.B * p.B;

		for (int x = 0; x < width; ++x) {
			// Forward pass.
			SrcIt inp_it = input + x;
			float pixel = float_reader(*inp_it);
			float* p_w = &w[3];
			p_w[-1] = p_w[-2] = p_w[-3] = pixel / p.B;

			for (int y = 0; y < height; ++y) {
				pixel = float_reader(*inp_it);
				*p_w = pixel + p.a1 * p_w[-1] + p.a2 * p_w[-2] + p.a3 * p_w[-3];
				inp_it += input_stride;
				++p_w;
			}

			// Backward pass.
			calcBackwardPassInitialConditions(p, p_w, pixel);
			float* p_int = intermediate_image.data() + x + height * intermediate_stride;
			for (int y = height - 1; y >= 0; --y) {
				--p_w;
				p_int -= intermediate_stride;
				*p_w = *p_w + p.a1 * p_w[1] + p.a2 * p_w[2] + p.a3 * p_w[3];
				*p_int = *p_w * B2; // Re-scale by B^2.
			}
		}
	}
	
	{ // Horizontal pass.
		gauss_blur_impl::FilterParams const p(h_sigma);
		float const B2 = p.B * p.B;
		float* intermediate_line = intermediate_image.data();
		DstIt output_line(output);

		for (int y = 0; y < height; ++y) {
			// Forward pass.
			float* p_int = intermediate_line;
			float* p_w = &w[3];
			p_w[-1] = p_w[-2] = p_w[-3] = intermediate_line[0] / p.B;
			for (int x = 0; x < width; ++x) {
				*p_w = *p_int + p.a1 * p_w[-1] + p.a2 * p_w[-2] + p.a3 * p_w[-3];
				++p_int;
				++p_w;
			}

			// Backward pass.
			calcBackwardPassInitialConditions(p, p_w, p_int[-1]);
			DstIt out_it = output_line + (width - 1);
			for (int x = width - 1; x >= 0; --x)
			{
				--p_w;
				*p_w = *p_w + p.a1 * p_w[1] + p.a2 * p_w[2] + p.a3 * p_w[3];
				float_writer(*out_it, *p_w * B2); // Re-scale by B^2.
				--out_it;
			}

			intermediate_line += intermediate_stride;
			output_line += output_stride;
		}
	}
}

template<typename SrcIt, typename DstIt, typename FloatReader, typename FloatWriter>
void anisotropicGaussBlurGeneric(
	QSize const size, float const dir_x, float const dir_y,
	float const dir_sigma, float const ortho_dir_sigma,
	SrcIt const input, int const input_stride, FloatReader const float_reader,
	DstIt const output, int const output_stride, FloatWriter const float_writer)
{
	using namespace gauss_blur_impl;

	if (size.isEmpty()) {
		return;
	}

	if (dir_x * dir_x + dir_y * dir_y < 1e-6) {
		throw std::invalid_argument(
			"anisotropicGaussBlur: (dir_x, dir_y) is too close to a null vector"
		);
	} else if (dir_sigma < 0 || ortho_dir_sigma < 0) {
		throw std::invalid_argument("anisotropicGaussBlur: stddev can't be negative");
	} else if (dir_sigma < 1e-2f && ortho_dir_sigma < 1e-2f) {
		return;
	}

	int const width = size.width();
	int const height = size.height();
	int const width_height_max = width > height ? width : height;

	struct InterpolatedCoord
	{
		/**
		 * Interpolation is always between a pixel at lower_bound
		 * and another one at lower_bound+1.
		 */
		int lower_bound;

		/** lower_bound + alpha is the actual coordinate. */
		float alpha;

		bool plusOffsetInRange(int offset, int lower, int upper) const {
			int const c0 = lower_bound + offset;
			return c0 >= lower && c0 < upper;
			// The above is equivalent to:
			// int const c0 = lower_bound + offset;
			// int const c1 = c0 + 1;
			// return c0 >= lower && c1 <= upper;
			// In other words, the [lower, upper] interval is actually closed.
		}
	};

	boost::scoped_array<float> const w(new float[3 + width_height_max + 3]);
	boost::scoped_array<InterpolatedCoord> skewed_line(new InterpolatedCoord[width_height_max]);

	// We add 2 extra pixels on each side. The inner 1px layer is necessary to
	// be able to interpolate pixels at a boundary. The outer layer is necessary
	// because on a "skewed" pass we write to positions offseted by -1 in order
	// for our writes not to contaminate reads on the next pass.
	Grid<float> intermediate_image(width, height, /*padding=*/2);
	int const intermediate_stride = intermediate_image.stride();

	gauss_blur_impl::AnisotropicParams const ap(dir_x, dir_y, dir_sigma, ortho_dir_sigma);

	{ // Horizontal pass.
		gauss_blur_impl::FilterParams const p(ap.sigma_x);
		float const B2 = p.B * p.B;
		SrcIt input_line = input;
		float* intermediate_line = intermediate_image.data();

		for (int y = 0; y < height; ++y) {
			// Forward pass.
			SrcIt inp_it = input_line;
			float pixel = *inp_it;
			float* p_w = &w[3];
			p_w[-1] = p_w[-2] = p_w[-3] = pixel / p.B;
			for (int x = 0; x < width; ++x) {
				pixel = float_reader(*inp_it);
				*p_w = pixel + p.a1 * p_w[-1] + p.a2 * p_w[-2] + p.a3 * p_w[-3];
				++p_w;
				++inp_it;
			}

			// Backward pass.
			calcBackwardPassInitialConditions(p, p_w, pixel);
			for (int x = width - 1; x >= 0; --x)
			{
				--p_w;
				*p_w = *p_w + p.a1 * p_w[1] + p.a2 * p_w[2] + p.a3 * p_w[3];
				intermediate_line[x] = *p_w * B2; // Re-scale by B^2.
			}

			input_line += input_stride;
			intermediate_line += intermediate_stride;
		}
	}

	// Initialise padded areas of intermediate_image. The outer area is filled with zeros
	// while the inner area mirrors the adjacent in-image pixels.
	initPaddingLayers(intermediate_image);

	// This will be either intermediate_image.data() - 1 or intermediate_image.data() -
	// intermediate_stride, depending on whether we traverse phi direction horizontally
	// or vertically.
	float* output_origin;

#if 1
	// It would make sense to traverse horizontally-leaning lines in horizontal direction,
	// as otherwise we are jumping over some pixels. However, if we traverse it vertically
	// anyway, the impulse response looks nicer for some reason. In [3] they always traverse
	// the phi direction vertically, without explaining why. We are going to do the same.
	if (true) {
#else
	if (std::abs(ap.cot_phi) <= 1.0f) { // Vertically-leaning phi-direction pass.
#endif
		float const adjusted_sigma_phi = ap.sigma_phi /
				std::sqrt(1.0f + ap.cot_phi * ap.cot_phi);
		gauss_blur_impl::FilterParams const p(adjusted_sigma_phi);
		float const B2 = p.B * p.B;

		// Initialise a vector of InterpolatedCoord values. These represent a single
		// skewed line that we will be moving horizontally and traversing vertically.
		for (int y = 0; y < height; ++y) {
			float const x_f = y * ap.cot_phi;
			float const lower_bound_f = std::floor(x_f);
			skewed_line[y] = InterpolatedCoord{
				static_cast<int>(lower_bound_f),
				x_f - lower_bound_f
			};
		}

		// We'll be "moving" skewed_line by adding x_offset values in
		// [min_x_offset, max_x_offset] to the lower_bound members of skewed_line.
		// Note that we don't actually modify the skewed_line itself.

		// When traversing a skewed line, we iterate y from y0 to y1 inclusively.
		// These do evolve as x_offset grows.
		int y0, y1;
		int min_x_offset, max_x_offset; // These are essentially constants.
		if (skewed_line[0].lower_bound < skewed_line[height - 1].lower_bound) {
			// '\'-like skewed line
			y0 = y1 = height - 1;

			// skewed_line[height - 1].lower_bound + min_x_offset == -1
			min_x_offset = -1 - skewed_line[height - 1].lower_bound;

			// skewed_line[0].lower_bound + 1 + max_x_offset == width
			max_x_offset = width - 1 - skewed_line[0].lower_bound;
		} else {
			// '/'-like skewed line
			y0 = y1 = 0;

			// skewed_line[0].lower_bound + min_x_offset == -1
			min_x_offset = -1 - skewed_line[0].lower_bound;

			// skewed_line[height - 1].lower_bound + 1 + max_x_offset == width
			max_x_offset = width - 1 - skewed_line[height - 1].lower_bound;
		}

		// Process skewed lines one by one.
		for (int x_offset = min_x_offset; x_offset <= max_x_offset; ++x_offset) {

			// Adjust y0 if necessary. Note that -1 and width are valid values for
			// lower_bound and lower_bound+1 respectively, as we do want to interpolate
			// pixels on the boundary. intermediate_image has padding pixels to accomodate
			// that.
			while (y0 > 0 && skewed_line[y0 - 1].plusOffsetInRange(x_offset, -1, width)) {
				--y0;
			}
			while (y0 < height - 1 && !skewed_line[y0].plusOffsetInRange(x_offset, -1, width)) {
				++y0;
			}

			// Adjust y1 if necessary.
			while (y1 > 0 && !skewed_line[y1].plusOffsetInRange(x_offset, -1, width)) {
				--y1;
			}
			while (y1 < height - 1 && skewed_line[y1 + 1].plusOffsetInRange(x_offset, -1, width)) {
				++y1;
			}

			float* intermediate_line = intermediate_image.data() + intermediate_stride * y0;
			InterpolatedCoord coord = skewed_line[y0];

			float pixel = (1.0f - coord.alpha) * intermediate_line[coord.lower_bound + x_offset]
				+ coord.alpha * intermediate_line[coord.lower_bound + x_offset + 1];
			float* p_w = &w[3];
			p_w[-1] = p_w[-2] = p_w[-3] = pixel / p.B;

			for (int y = y0;; ++y, intermediate_line += intermediate_stride) {
				coord = skewed_line[y];
				int const x0 = coord.lower_bound + x_offset;
				int const x1 = x0 + 1;

				pixel = (1.0f - coord.alpha) * intermediate_line[x0]
					+ coord.alpha * intermediate_line[x1];

				*p_w = pixel + p.a1 * p_w[-1] + p.a2 * p_w[-2] + p.a3 * p_w[-3];
				++p_w;

				if (y == y1) {
					calcBackwardPassInitialConditions(p, p_w, pixel);
					break;
				}
			}

			// Backward pass.
			for (int y = y1; y >= y0; --y, intermediate_line -= intermediate_stride) {
				--p_w;
				*p_w = *p_w + p.a1 * p_w[1] + p.a2 * p_w[2] + p.a3 * p_w[3];

				coord = skewed_line[y];
				pixel = *p_w * B2; // Re-scale by B^2.

				// Shifted by -1 relative to the forward pass, to avoid contaminating
				// future reads. The outer 1px padding layer of intermediate_image
				// accomodates that.
				int const x1 = coord.lower_bound + x_offset;
				int const x0 = x1 - 1;

				intermediate_line[x0] += (1.0f - coord.alpha) * pixel;
				intermediate_line[x1] = coord.alpha * pixel;

				// Note that on the very first backward pass the += operation will be
				// hitting the outer 1px padding layer of intermediate_image (see
				// min_x_ofset calculation), which is initialised to zero and is
				// outside of the area that will be copied to the output image.
			}
		}

		// Our writes are shifted horizontally by -1.
		output_origin = intermediate_image.data() - 1;

	} else { // Horizontally-leaning phi-direction pass.

		float const tan_phi = 1.0f / ap.cot_phi;
		float const adjusted_sigma_phi = ap.sigma_phi /
				std::sqrt(1.0f + tan_phi * tan_phi);
		gauss_blur_impl::FilterParams const p(adjusted_sigma_phi);
		float const B2 = p.B * p.B;

		// Initialise a vector of InterpolatedCoord values. These represent a single
		// skewed line that we will be moving vertically and traversing horizontally.
		for (int x = 0; x < width; ++x) {
			float const y_f = x * tan_phi;
			float const lower_bound_f = std::floor(y_f);
			skewed_line[x] = InterpolatedCoord{
				static_cast<int>(lower_bound_f),
				y_f - lower_bound_f
			};
		}

		// We'll be "moving" skewed_line by adding y_offset values in
		// [min_y_offset, max_y_offset] to the lower_bound members of skewed_line.
		// Note that we don't actually modify the skewed_line itself.

		// When traversing a skewed line, we iterate x from x0 to x1 inclusively.
		// These do evolve as y_offset grows.
		int x0, x1;
		int min_y_offset, max_y_offset; // These are essentially constants.
		if (skewed_line[0].lower_bound < skewed_line[width - 1].lower_bound) {
			// '-_'-like skewed line (remember the Y axis points down)
			x0 = x1 = width - 1;

			// skewed_line[width - 1].lower_bound + min_y_offset == -1
			min_y_offset = -1 - skewed_line[width - 1].lower_bound;

			// skewed_line[0].lower_bound + 1 + max_y_offset == height
			max_y_offset = height - 1 - skewed_line[0].lower_bound;
		} else {
			// '_-'-like skewed line (remember the Y axis points down)
			x0 = x1 = 0;

			// skewed_line[0].lower_bound + min_y_offset == -1
			min_y_offset = -1 - skewed_line[0].lower_bound;

			// skewed_line[width - 1].lower_bound + 1 + max_y_offset == height
			max_y_offset = height - 1 - skewed_line[width - 1].lower_bound;
		}

		// Process skewed lines one by one.
		for (int y_offset = min_y_offset; y_offset <= max_y_offset; ++y_offset) {

			// Adjust x0 if necessary. Note that -1 and height are valid values for
			// lower_bound and lower_bound+1 respectively, as we do want to interpolate
			// pixels on the boundary. intermediate_image has padding pixels to accomodate
			// that.
			while (x0 > 0 && skewed_line[x0 - 1].plusOffsetInRange(y_offset, -1, height)) {
				--x0;
			}
			while (x0 < width - 1 && !skewed_line[x0].plusOffsetInRange(y_offset, -1, height)) {
				++x0;
			}

			// Adjust x1 if necessary.
			while (x1 > 0 && !skewed_line[x1].plusOffsetInRange(y_offset, -1, height)) {
				--x1;
			}
			while (x1 < width - 1 && skewed_line[x1 + 1].plusOffsetInRange(y_offset, -1, height)) {
				++x1;
			}

			float* intermediate_col = intermediate_image.data() + x0;
			InterpolatedCoord coord = skewed_line[x0];

			float pixel = (1.0f - coord.alpha) *
				intermediate_col[(coord.lower_bound + y_offset) * intermediate_stride]
				+ coord.alpha *
				intermediate_col[(coord.lower_bound + y_offset + 1) * intermediate_stride];
			float* p_w = &w[3];
			p_w[-1] = p_w[-2] = p_w[-3] = pixel / p.B;

			for (int x = x0;; ++x, ++intermediate_col) {
				coord = skewed_line[x];
				int const y0 = coord.lower_bound + y_offset;
				int const y1 = y0 + 1;

				pixel = (1.0f - coord.alpha) * intermediate_col[y0 * intermediate_stride]
					+ coord.alpha * intermediate_col[y1 * intermediate_stride];

				*p_w = pixel + p.a1 * p_w[-1] + p.a2 * p_w[-2] + p.a3 * p_w[-3];
				++p_w;

				if (x == x1) {
					calcBackwardPassInitialConditions(p, p_w, pixel);
					break;
				}
			}

			// Backward pass.
			for (int x = x1; x >= x0; --x, --intermediate_col) {
				--p_w;
				*p_w = *p_w + p.a1 * p_w[1] + p.a2 * p_w[2] + p.a3 * p_w[3];

				coord = skewed_line[x];
				pixel = *p_w * B2; // Re-scale by B^2.

				// Shifted by -1 relative to the forward pass, to avoid contaminating
				// future reads. The outer 1px padding layer of intermediate_image
				// accomodates that.
				int const y1 = coord.lower_bound + y_offset;
				int const y0 = y1 - 1;

				intermediate_col[y0 * intermediate_stride] += (1.0f - coord.alpha) * pixel;
				intermediate_col[y1 * intermediate_stride] = coord.alpha * pixel;

				// Note that on the very first backward pass the += operation will be
				// hitting the outer 1px padding layer of intermediate_image (see
				// min_y_ofset calculation), which is initialised to zero and is
				// outside of the area that will be copied to the output image.
			}
		}

		// Our writes are shifted vertically by -1.
		output_origin = intermediate_image.data() - intermediate_stride;
	}

	// Copy from intermediate image to output image.
	using OutPixel = std::iterator_traits<DstIt>::value_type;
	rasterOpGeneric(
		[float_writer](OutPixel& out, float value) {
			float_writer(out, value);
		},
		GridAccessor<OutPixel>{output, output_stride, width, height},
		GridAccessor<float>{output_origin, intermediate_stride, width, height}
	);
}

} // namespace imageproc

#endif
