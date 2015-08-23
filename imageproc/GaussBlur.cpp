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

#include "GaussBlur.h"
#include "GrayImage.h"
#include "Constants.h"
#include <Eigen/Core>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdint>

using namespace Eigen;

namespace imageproc
{

namespace gauss_blur_impl
{

FilterParams::FilterParams(float sigma)
{
	float const q = sigmaToQ(sigma);
	float const q2 = q * q;
	float const q3 = q2 * q;
	assert(q > 0); // Guaranteed by sigmaToQ()

	// Formula 8c in [1].
	float const b0 = 1.57825f + 2.44413f * q + 1.4281f * q2 + 0.422205f * q3;
	float const b1 = 2.44413f * q + 2.85619f * q2 + 1.26661f * q3;
	float const b2 = -1.4281f * q2 - 1.26661f * q3;
	float const b3 = 0.422205f * q3;
	assert(b0 > 0); // Because q is > 0

	a1 = b1 / b0;
	a2 = b2 / b0;
	a3 = b3 / b0;
	B = 1.0f - (a1 + a2 + a3);
}

float
FilterParams::sigmaToQ(float sigma)
{
	// Formula 11b in [1].
	if (sigma >= 2.5f) {
		return 0.98711f * sigma - 0.9633f;
	} else {
		return 3.97156f - 4.14554f * std::sqrt(1.f - 0.26891f * std::max<float>(sigma, 0.5f));
	}
}

HorizontalDecompositionParams::HorizontalDecompositionParams(
	float dir_x, float dir_y, float dir_sigma, float ortho_dir_sigma)
{
	Eigen::Vector2f cos_sin(dir_x, dir_y);
	cos_sin.normalize();

	// Constraining sigma_u and sigma_v to be slightly positive
	// prevents sum_squares below from being zero.
	float const sigma_u = std::max<float>(0.01f, dir_sigma);
	float const sigma_v = std::max<float>(0.01f, ortho_dir_sigma);
	float const sigma2_u = sigma_u * sigma_u;
	float const sigma2_v = sigma_v * sigma_v;
	float const cos2 = cos_sin[0] * cos_sin[0];
	float const sin2 = cos_sin[1] * cos_sin[1];
	float const sum_squares = sigma2_v * cos2 + sigma2_u * sin2;

	// Formula 9 in [3].
	sigma_x = sigma_u * sigma_v / std::sqrt(sum_squares);

	// Formula 11 in [3], except we calculate cotangent rather than tangent.
	cot_phi = ((sigma2_u - sigma2_v) * cos_sin[0] * cos_sin[1]) / sum_squares;

	// Equivalent to formula 10 in [3], except it avoids a negative sigma.
	sigma_phi = std::sqrt((cot_phi * cot_phi + 1.0f) * sum_squares);
}

VerticalDecompositionParams::VerticalDecompositionParams(
	float dir_x, float dir_y, float dir_sigma, float ortho_dir_sigma)
{
	HorizontalDecompositionParams const p(dir_y, -dir_x, dir_sigma, ortho_dir_sigma);
	sigma_y = p.sigma_x;
	sigma_phi = p.sigma_phi;
	tan_phi = -p.cot_phi;
}

/**
 * The second application of an LTI system involves a 3-step look-ahead into the
 * output signal. That is, we need 3 values from the future of our output signal
 * in order to compute the rest of it. Paper [2] is all about computing those 3
 * values.
 *
 * @param p Parameters of our LTI system.
 * @param w_end We use array w to store both the intermediate signal w and the
 *        output signal. We allocate memory for 3 additional values on each side
 *        to store the initial conditions. The w_end parameter corresponds to the
 *        position just beyond the computed intermediate signal w. The 3 elements
 *        starting from w_end[0] will be filled with the initial conditions for
 *        the second pass, in which the intermediate signal in w gets overwritten
 *        by the output signal.
 * @param future_signal_val We assume the input signal continues into the future
 *        with a constant value. This parameter specifies that value.
 */
void
calcBackwardPassInitialConditions(FilterParams const& p, float* w_end, float future_signal_val)
{
	// Formula 15 in [2].
	float const u_plus = future_signal_val / p.B;
	float const v_plus = u_plus / p.B;

	// Formula 3 in [2].
	Matrix3f M;
	M(0, 0) = -p.a3 * p.a1 + 1.0 - p.a3 * p.a3 - p.a2;
	M(0, 1) = (p.a3 + p.a1) * (p.a2 + p.a3 * p.a1);
	M(0, 2) = p.a3 * (p.a1 + p.a3 * p.a2);
	M(1, 0) = p.a1 + p.a3 * p.a2;
	M(1, 1) = -(p.a2 - 1.0f) * (p.a2 + p.a3 * p.a1);
	M(1, 2) = -(p.a3 * p.a1 + p.a3 * p.a3 + p.a2 - 1.0f) * p.a3;
	M(2, 0) = p.a3 * p.a1 + p.a2 + p.a1 * p.a1 - p.a2 * p.a2;
	M(2, 1) = p.a1 * p.a2 + p.a3 * p.a2 * p.a2 - p.a1 * p.a3 * p.a3
			- p.a3 * p.a3 * p.a3 - p.a3 * p.a2 + p.a3;
	M(2, 2) = p.a3 * (p.a1 + p.a3 * p.a2);
	M /= (1.0f + p.a1 - p.a2 + p.a3) * p.B * (1.0f + p.a2 + (p.a1 - p.a3) * p.a3);

	Vector3f u(w_end[-1], w_end[-2], w_end[-3]);
	u -= Vector3f::Constant(u_plus);

	// Formula 14 in [2].
	Vector3f result(M * u);
	result += Vector3f::Constant(v_plus);

	w_end[0] = result[0];
	w_end[1] = result[1];
	w_end[2] = result[2];
}

void initPaddingLayers(Grid<float>& intermediate_image)
{
	assert(intermediate_image.padding() == 2);

	int const width = intermediate_image.width();
	int const height = intermediate_image.height();
	int const stride = intermediate_image.stride();
	float* line = intermediate_image.paddedData();

	// Outer padding.
	memset(line, 0, stride);
	line += stride;

	// 1px outer padding - inner padding - 1px outer padding.
	line[0] = 0.0f;
	line[1] = line[2 + stride]; // top-left corner
	for (int x = 0; x < width; ++x) {
		line[2 + x] = line[2 + x + stride];
	}
	line[2 + width] = line[2 + width - 1 + stride]; // top-right corner
	line[2 + width + 1] = 0.0f;
	line += stride;

	for (int y = 0; y < height; ++y, line += stride) {
		line[0] = 0.0f;
		line[1] = line[2];
		line[2 + width] = line[2 + width - 1];
		line[2 + width + 1] = 0.0f;
	}

	// 1px outer padding - inner padding - 1px outer padding.
	line[0] = 0.0f;
	line[1] = line[2 - stride]; // bottom-left corner
	for (int x = 0; x < width; ++x) {
		line[2 + x] = line[2 + x - stride];
	}
	line[2 + width] = line[2 + width - 1 - stride]; // bottom-right corner
	line[2 + width + 1] = 0.0f;
	line += stride;

	// Outer padding.
	memset(line, 0, stride);
}

} // namespace gauss_blur_impl

GrayImage gaussBlur(GrayImage const& src, float h_sigma, float v_sigma)
{
	if (src.isNull()) {
		return src;
	}

	RoundAndClipValueConv<uint8_t> const float2byte;

	GrayImage dst(src.size());
	gaussBlurGeneric(
		src.size(), h_sigma, v_sigma,
		src.data(), src.stride(), StaticCastValueConv<float>(),
		dst.data(), dst.stride(), [float2byte](uint8_t& dst, float src) { dst = float2byte(src); }
	);

	return dst;
}

GrayImage anisotropicGaussBlur(
	GrayImage const& src, float dir_x, float dir_y,
	float dir_sigma, float ortho_dir_sigma)
{
	if (src.isNull()) {
		return src;
	}

	RoundAndClipValueConv<uint8_t> const float2byte;

	GrayImage dst(src.size());
	anisotropicGaussBlurGeneric(
		src.size(), dir_x, dir_y, dir_sigma, ortho_dir_sigma,
		src.data(), src.stride(), StaticCastValueConv<float>(),
		dst.data(), dst.stride(), [float2byte](uint8_t& dst, float src) { dst = float2byte(src); }
	);

	return dst;
}

} // namespace imageproc
