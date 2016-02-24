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

/**
 * @param src The source texture.
 * @param dst The destination texture. When doing two 1D passes,
 *        the destination texture on the first pass should be a float
 *        one, as otherwise we are clamping to min/max values too early,
 *        in addition to doing unwanted rounding.
 * @param coord_normalizer A pair of coefficients to convert an integer
 *        pair of coordinates into normalized [0, 1] coordinates.
 * @param coord_delta Same as coord_normalizer, except one of the coordinates
 *        will be set to zero. Adding @p coord_delta to a normalized pair
 *        of coordinates is equivalent to adding 1 to one of the integer
 *        coordinates while leaving the other one unchanged. The non-zero
 *        coordinate effectively specifies the direction of this pass.
 * @param coeffs A vector of convolution coefficients.
 * @param window_first_offset If the pixel we are estimating is at position 0,
 *        @p window_first_offset is the position of the furthest neighbour
 *        to the left (or top) participating in estimation. @p window_first_offset
 *        is generally negative.
 * @param window_last_offset If the pixel we are estimating is at position 0,
 *        @p window_last_offset is the position of the furthest neighbour
 *        to the right (or bottom) participating in estimation. The window size
 *        is given by (window_last_offset - window_first_offset + 1).
 */
kernel void sav_gol_filter_1d(
	read_only image2d_t src, write_only image2d_t dst,
	float2 const coord_normalizer, float2 const coord_delta,
	constant float const* coeffs, int window_first_offset, int window_last_offset)
{
	int const width = get_image_width(src);
	int const height = get_image_height(src);
	int const dst_x = get_global_id(0);
	int const dst_y = get_global_id(1);
	bool out_of_bounds = (dst_x >= width) | (dst_y >= height);
	if (out_of_bounds) {
		return;
	}

	int2 const dst_coord = (int2)(dst_x, dst_y);
	sampler_t const sampler = CLK_NORMALIZED_COORDS_TRUE|CLK_ADDRESS_MIRRORED_REPEAT|CLK_FILTER_NEAREST;

	float2 const base_src_coord = convert_float2(dst_coord) * coord_normalizer;
	float accum = 0.f;
	for (int delta = window_first_offset; delta <= window_last_offset; ++delta) {
		float2 const src_coord = base_src_coord + (float)delta * coord_delta;
		float4 const pixel = read_imagef(src, sampler, src_coord);
		float const coeff = coeffs[delta - window_first_offset];
		accum += pixel.x * coeff;
	}

	write_imagef(dst, dst_coord, (float4)(accum, accum, accum, 1.0f));
}
