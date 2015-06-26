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

static float3 updateHistoryForBackwardPass(
	float3 const history, float3 const a, float const B_recip, float const pixel)
{
	float const u_plus = pixel * B_recip;
	float const v_plus = u_plus * B_recip;

	// OpenCL doesn't have matrix types, so we represent
	// this matrix as individual columns.
	float3 m1, m2, m3;

	m1.x = -a.z * a.x + 1.f - a.z * a.z - a.y;
	m2.x = (a.z + a.x) * (a.y + a.z * a.x);
	m3.x = a.z * (a.x + a.z * a.y);
	m1.y = a.x + a.z * a.y;
	m2.y = -(a.y - 1.f) * (a.y + a.z * a.x);
	m3.y = -(a.z * a.x + a.z * a.z + a.y - 1.f) * a.z;
	m1.z = a.z * a.x + a.y + a.x * a.x - a.y * a.y;
	m2.z = a.x * a.y + a.z * a.y * a.y - a.x * a.z * a.z
			- a.z * a.z * a.z - a.z * a.y + a.z;
	m3.z = a.z * (a.x + a.z * a.y);

	float const normalizer = B_recip /
		((1.f + a.x - a.y + a.z) * (1.f + a.y + (a.x - a.z) * a.z));

	float3 const u = history - u_plus;
	return (m1 * u.x + m2 * u.y + m3 * u.z) * normalizer + v_plus;
}

/**
 * @brief Apply gauss filtering to either rows or columns of an image.
 *
 * @param signal_length If working on rows, the row length (image width),
 *        otherwise the column length (image height).
 * @param src Pointer to the source image data. If the image has padding
 *        pixels, @p src will point to padding data rather than inner data.
 * @param src_offset Zero when the source image doesn't have any padding.
 *        otherwise, src + src_offset will point to the first (top-left)
 *        non-padding pixel.
 * @param src_id_delta When filtering rows, the global id within the
 *        kernel corresponds to the row number (the y coordinate). In this
 *        case, @p src_id_delta is the distance between a pixel and the one
 *        directly below it. When filtering columns, the global id is the
 *        column number (the x coordinate) and @p src_id_delta is the distance
 *        between a pixel and the adjacent one to the right. Normally such
 *        distance would be 1.
 * @param src_sample_delta The distance between adjacent pixels in filtering
 *        direction. That is, when filtering rows, it's normally 1 and when
 *        filtering columns, it's the image stride.
 * @param dst The destination image data. @see src.
 * @param dst_offset @see src_offset.
 * @param dst_id_delta @see src_id_delta.
 * @param dst_sample_delta @see src_sample_delta.
 * @param a The vector of filter coefficients. @see imageproc::gauss_blur_impl::FilterParams.
 * @param B_recip One over B, where B is yet another filter coefficient. @see a.
 * @param B_square B squared, where B is yet another filter coefficient. @see a.
 *
 * @note This kernel supports in-place operation, where src == dst, provided the
 *       offsets and strides either match or are set up in such a way that read
 *       and write regions don't overlap at all.
 */
kernel void gauss_blur_1d(
	int const signal_length,
	global float const* const src, int const src_offset,
	int const src_id_delta, int const src_sample_delta,
	global float* const dst, int const dst_offset,
	int const dst_id_delta, int const dst_sample_delta,
	float3 const a, float const B_recip, float const B_square)
{
	int const id = get_global_id(0);

	// Forward pass.
	global float const* p_src = src + src_offset + id * src_id_delta;
	global float* p_dst = dst + dst_offset + id * dst_id_delta;
	float const initial = *p_src * B_recip;
	float3 history = (float3)(initial, initial, initial);
	for (int i = 0; i < signal_length; ++i) {
		float const out = *p_src + dot(a, history);

		// Update history.
		history = history.s001;
		history.x = out;

		*p_dst = out * B_square;

		// Advance pointers.
		p_src += src_sample_delta;
		p_dst += dst_sample_delta;
	}

	p_src -= src_sample_delta;
	history = updateHistoryForBackwardPass(history, a, B_recip, *p_src) * B_square;

	// Backward pass.
	for (int i = 0; i < signal_length; ++i) {
		p_dst -= dst_sample_delta;
		float const out = *p_dst + dot(a, history);

		// Update history.
		history = history.s001;
		history.x = out;

		*p_dst = out;
	}
}

typedef struct
{
	int lower_bound;
	float alpha;
} InterpolatedCoord;

static InterpolatedCoord get_interpolated_coord(int const other_coord, float const delta)
{
	float const target_coord = (float)other_coord * delta;
	float const lower_bound = floor(target_coord);
	InterpolatedCoord coord = { (int)lower_bound, target_coord - lower_bound };
	return coord;
}

kernel void gauss_blur_h_decomp_stage1(
	int const src_width, int const src_height,
	global float const* const src, int const src_offset, int const src_stride,
	global float* const dst, int const dst_offset, int const dst_stride,
	int const min_x_offset, float const dx,
	float3 const a, float const B_recip, float const B_square)
{
	int const dst_x = get_global_id(0);
	int const x_offset = min_x_offset + dst_x;

	int y0 = 0;
	InterpolatedCoord coord = get_interpolated_coord(y0, dx);
	// Note that -1 and src_height are the valid y coordinates thanks to the padding of src.
	if (coord.lower_bound + x_offset < -1 || coord.lower_bound + x_offset + 1 > src_width) {
		// src_x = x_offset + y * dx
		// y = (src_x - x_offset) / dx
		// src_x is either -1 - E or (width + E), where E is some small number.
		float const y0f = min(
			(-1.01f - (float)x_offset) / dx,
			(0.01f + (float)(src_width - x_offset)) / dx
		);
		y0 = max(0, (int)floor(y0f));

		// The above calculation for y0 may produce y0 where the corresponding x
		// is still outside of the image. In this case, we just move a bit forward.
		for (; y0 < src_height; ++y0) {
			coord = get_interpolated_coord(y0, dx);
			if (coord.lower_bound + x_offset >= -1 && coord.lower_bound + x_offset + 1 <= src_width) {
				break;
			}
		}
	}

	global float const* src_line = src + src_offset + src_stride * y0;
	global float* dst_line = dst + dst_offset + dst_stride * y0;

	float pixel = (1.f - coord.alpha) * src_line[coord.lower_bound + x_offset]
		+ coord.alpha * src_line[coord.lower_bound + x_offset + 1];
	float const initial = pixel * B_recip;
	float3 history = (float3)(initial, initial, initial);

	// Forward pass.
	int y = y0; // Note that y corresponds to both src and dst images.
	for (; y < src_height; ++y) {
		coord = get_interpolated_coord(y, dx);
		int const x0 = coord.lower_bound + x_offset;
		int const x1 = x0 + 1;
		if (x0 < -1 || x1 > src_width) {
			break;
		}

		pixel = (1.f - coord.alpha) * src_line[x0] + coord.alpha * src_line[x1];
		float const out = pixel + dot(a, history);

		// Update history.
		history = history.s001;
		history.s0 = out;

		// Write out the scaled result.
		dst_line[x0] = out * B_square;

		// Move to the next line.
		src_line += src_stride;
		dst_line += dst_stride;
	}

	history = updateHistoryForBackwardPass(history, a, B_recip, pixel) * B_square;

	// Backward pass.
	for (--y; y >= y0; --y) {
		dst_line -= dst_stride;
		int const x0 = get_interpolated_coord(y, dx).lower_bound + x_offset;

		float const out = dst_line[x0] + dot(a, history);

		// Update history.
		history = history.s001;
		history.s0 = out;

		// Write out the result.
		dst_line[x0] = out;
	}
}

kernel void gauss_blur_v_decomp_stage1(
	int const src_width, int const src_height,
	global float const* const src, int const src_offset, int const src_stride,
	global float* const dst, int const dst_offset, int const dst_stride,
	int const min_y_offset, float const dy,
	float3 const a, float const B_recip, float const B_square)
{
	int const dst_y = get_global_id(0);
	int const y_offset = min_y_offset + dst_y;

	int x0 = 0;
	InterpolatedCoord coord = get_interpolated_coord(x0, dy);
	// Note that -1 and src_height are the valid y coordinates thanks to the padding of src.
	if (coord.lower_bound + y_offset < -1 || coord.lower_bound + y_offset + 1 > src_height) {
		// src_y = y_offset + x * dy
		// x = (src_y - y_offset) / dy
		// src_y is either -1 - E or (height + E), where E is some small number.
		float const x0f = min(
			(-1.01f - (float)y_offset) / dy,
			(0.01f + (float)(src_height - y_offset)) / dy
		);
		x0 = max(0, (int)floor(x0f));

		// The above calculation for x0 may produce x0 where the corresponding y
		// is still outside of the image. In this case, we just move a bit forward.
		for (; x0 < src_width; ++x0) {
			coord = get_interpolated_coord(x0, dy);
			if (coord.lower_bound + y_offset >= -1 && coord.lower_bound + y_offset + 1 <= src_height) {
				break;
			}
		}
	}

	global float const* p_src = src + src_offset + x0;
	global float* p_dst = dst + dst_offset + x0;

	float pixel = (1.f - coord.alpha) * p_src[(coord.lower_bound + y_offset) * src_stride]
		+ coord.alpha * p_src[(coord.lower_bound + y_offset + 1) * src_stride];
	float const initial = pixel * B_recip;
	float3 history = (float3)(initial, initial, initial);

	// Forward pass.
	int x = x0; // Note that x corresponds to both src and dst images.
	for (; x < src_width; ++x) {
		coord = get_interpolated_coord(x, dy);
		int const y0 = coord.lower_bound + y_offset;
		int const y1 = y0 + 1;
		if (y0 < -1 || y1 > src_height) {
			break;
		}

		pixel = (1.f - coord.alpha) * p_src[y0 * src_stride]
			+ coord.alpha * p_src[y1 * src_stride];
		float const out = pixel + dot(a, history);

		// Update history.
		history = history.s001;
		history.s0 = out;

		// Write out the scaled result.
		p_dst[y0 * dst_stride] = out * B_square;

		// Move to the next column.
		++p_src;
		++p_dst;
	}

	history = updateHistoryForBackwardPass(history, a, B_recip, pixel) * B_square;

	// Backward pass.
	for (--x; x >= x0; --x) {
		--p_dst;
		int const y0 = get_interpolated_coord(x, dy).lower_bound + y_offset;

		float const out = p_dst[y0 * dst_stride] + dot(a, history);

		// Update history.
		history = history.s001;
		history.s0 = out;

		// Write out the result.
		p_dst[y0 * dst_stride] = out;
	}
}

kernel void gauss_blur_h_decomp_stage2(
	global float const* const src, int const src_offset, int const src_stride,
	global float* const dst, int const dst_offset, int const dst_stride,
	float const dx)
{
	int const x = get_global_id(0);
	int const y = get_global_id(1);

	global float const* const src_line = src + src_offset + y * src_stride;
	global float* const dst_line = dst + dst_offset + y * dst_stride;
	float const alpha = get_interpolated_coord(y, dx).alpha;

	// The skewed line that generated the pixel at src(x, y) passed through
	// the (x + alpha, y) point. The stage1 kernel aggregated the data from
	// (x, y) and (x + 1, y) and put it to (from our perspective) src(x, y).
	// Now we want to do the opposite operation, that is to spread the data
	// from src(x, y) to dst(x, y) and dst(x + 1, y). However, we are called
	// once for each destination rather than source pixel. That means we need
	// to be aggregating the data from src(x - 1, y) and src(x, y) into
	// dst(x, y). When alpha is zero, there is a one to one mapping between
	// src(x, y) and dst(x, y).
	dst_line[x] = src_line[x - 1] * alpha + src_line[x] * (1.f - alpha);
}

kernel void gauss_blur_v_decomp_stage2(
	global float const* const src, int const src_offset, int const src_stride,
	global float* const dst, int const dst_offset, int const dst_stride,
	float const dy)
{
	int const x = get_global_id(0);
	int const y = get_global_id(1);

	global float const* const p_src = src + src_offset + x;
	global float* const p_dst = dst + dst_offset + x;
	float const alpha = get_interpolated_coord(x, dy).alpha;

	p_dst[y * dst_stride] = p_src[(y - 1) * src_stride] * alpha
		+ p_src[y * src_stride] * (1.f - alpha);
}
