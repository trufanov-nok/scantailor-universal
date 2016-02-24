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

float4 updateHistoryForBackwardPass(
	float4 const history, float const pixel,
	float3 const m1, float3 const m2, float3 const m3)
{
	float4 const u = history - pixel;
	return (float4)(m1 * u.s0 + m2 * u.s1 + m3 * u.s2 + pixel, 0.f);
}

/**
 * @brief Apply gauss filtering to either rows or columns of an image.
 *
 * @param signal_length If working on rows, the row length (image width),
 *        otherwise the column length (image height).
 * @param src Pointer to the source image datb. If the image has padding
 *        pixels, @p src will point to padding data rather than inner datb.
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
 * @param dst The destination image datb. @see src.
 * @param dst_offset @see src_offset.
 * @param dst_id_delta @see src_id_deltb.
 * @param dst_sample_delta @see src_sample_deltb.
 * @param b The vector of filter coefficients. @see imageproc::gauss_blur_impl::FilterParams.
 * @param m1 The first column of the history update matrix.
 *        @see imageproc::gaus_blur_impl::calcBackwardPassInitialConditions()
 * @param m2 @see m1
 * @param m3 @see m1
 *
 * @note This kernel supports in-place operation, where src == dst, provided the
 *       offsets and strides either match or are set up in such a way that read
 *       and write regions don't overlap at all.
 */
kernel void gauss_blur_1d(
	int const signal_length,
	int const num_signals,
	global float const* const src, int const src_offset,
	int const src_id_delta, int const src_sample_delta,
	global float* const dst, int const dst_offset,
	int const dst_id_delta, int const dst_sample_delta,
	float4 const b, float3 const m1, float3 const m2, float3 const m3)
{
	int const id = get_global_id(0);
	if (id >= num_signals) {
		return;
	}

	// Forward pass.
	global float const* p_src = src + src_offset + id * src_id_delta;
	global float* p_dst = dst + dst_offset + id * dst_id_delta;
	float pixel = *p_src;
	float4 history = (float4)(pixel, pixel, pixel, pixel);
	for (int i = 0;;) {
		// Calculate and write out the next output value.
		history.s0 = dot(b, history);
		*p_dst = history.s0;

		if (++i >= signal_length) {
			break;
		}

		// Move to the next sample.
		p_src += src_sample_delta;
		p_dst += dst_sample_delta;

		// Read the next input pixel.
		pixel = *p_src;

		// Update history.
		history = history.s0012;
		history.s0 = pixel;
	}

	history = updateHistoryForBackwardPass(history, pixel, m1, m2, m3);

	// Backward pass.
	for (int i = 0; i < signal_length; ++i) {
		// Shift history.
		history = history.s0012;

		// Calculate the next output value, put it to history.s0 and write it out.
		history.s0 = *p_dst;
		history.s0 = dot(b, history);
		*p_dst = history.s0;

		// Move to the previous sample.
		p_dst -= dst_sample_delta;
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

kernel void gauss_blur_skewed_vert_traversal_stage1(
	int const src_width, int const src_height,
	global float const* const src, int const src_offset, int const src_stride,
	global float* const dst, int const dst_offset, int const dst_stride,
	int const min_x_offset, int const max_x_offset, float const dx,
	float4 const b, float3 const m1, float3 const m2, float3 const m3)
{
	int const dst_x = get_global_id(0);
	int const x_offset = min_x_offset + dst_x;
	if (x_offset > max_x_offset) {
		return;
	}

	int y0 = 0;
	InterpolatedCoord coord = get_interpolated_coord(y0, dx);
	// Note that -1 and src_width are the valid x coordinates thanks to the padding of src.
	bool out_of_bounds = (coord.lower_bound + x_offset < -1) |
			(coord.lower_bound + x_offset + 1 > src_width);
	if (out_of_bounds) {
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
		for (;; ++y0) {
			coord = get_interpolated_coord(y0, dx);
			out_of_bounds = (coord.lower_bound + x_offset < -1) |
					(coord.lower_bound + x_offset + 1 > src_width);
			if (!out_of_bounds) {
				break;
			}
		}
	}

	global float const* src_line = src + src_offset + src_stride * y0;
	global float* dst_line = dst + dst_offset + dst_stride * y0;

	int x0 = coord.lower_bound + x_offset;
	int x1 = x0 + 1;
	float pixel = (1.f - coord.alpha) * src_line[x0] + coord.alpha * src_line[x1];
	float4 history = (float4)(pixel, pixel, pixel, pixel);

	// Forward pass.
	int y = y0; // Note that y corresponds to both src and dst images.
	for (;;) {
		// Calculate and write out the next output value.
		history.s0 = dot(b, history);
		dst_line[x0] = history.s0;

		// Initiate moving to the next line.
		++y;
		coord = get_interpolated_coord(y, dx);
		x0 = coord.lower_bound + x_offset;
		x1 = x0 + 1;
		out_of_bounds = (y >= src_height) | (x0 < -1) | (x1 > src_width);
		if (out_of_bounds) {
			break;
		}

		// Complete moving to the next line.
		src_line += src_stride;
		dst_line += dst_stride;

		// Read the next input pixel.
		pixel = (1.f - coord.alpha) * src_line[x0] + coord.alpha * src_line[x1];

		// Update history.
		history = history.s0012;
		history.s0 = pixel;
	}

	history = updateHistoryForBackwardPass(history, pixel, m1, m2, m3);

	// Backward pass.
	for (--y; y >= y0; --y) {
		// Shift history.
		history = history.s0012;

		x0 = get_interpolated_coord(y, dx).lower_bound + x_offset;

		// Calculate the next output value, put it to history.s0 and write it out.
		history.s0 = dst_line[x0];
		history.s0 = dot(b, history);
		dst_line[x0] = history.s0;

		// Move to the previous line.
		dst_line -= dst_stride;
	}
}

kernel void gauss_blur_skewed_hor_traversal_stage1(
	int const src_width, int const src_height,
	global float const* const src, int const src_offset, int const src_stride,
	global float* const dst, int const dst_offset, int const dst_stride,
	int const min_y_offset, int const max_y_offset, float const dy,
	float4 const b, float3 const m1, float3 const m2, float3 const m3)
{
	int const dst_y = get_global_id(0);
	int const y_offset = min_y_offset + dst_y;
	if (y_offset > max_y_offset) {
		return;
	}

	int const y_offset0 = min_y_offset + get_group_id(0) * get_local_size(0);
	int const y_offset1 = y_offset0 + get_local_size(0) - 1;

	int x0 = 0;
	InterpolatedCoord coord = get_interpolated_coord(x0, dy);
	// Note that -1 and src_height are the valid y coordinates thanks to the padding of src.
	bool out_of_bounds = (coord.lower_bound + y_offset < -1) |
			(coord.lower_bound + y_offset + 1 > src_height);
	if (out_of_bounds) {
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
		for (;; ++x0) {
			coord = get_interpolated_coord(x0, dy);
			out_of_bounds = (coord.lower_bound + y_offset < -1) |
					(coord.lower_bound + y_offset + 1 > src_height);
			if (!out_of_bounds) {
				break;
			}
		}
	}

	global float const* p_src = src + src_offset + x0;
	global float* p_dst = dst + dst_offset + x0;

	int y0 = coord.lower_bound + y_offset;
	int y1 = y0 + 1;
	float pixel = (1.f - coord.alpha) * p_src[y0 * src_stride]
			+ coord.alpha * p_src[y1 * src_stride];
	float4 history = (float4)(pixel, pixel, pixel, pixel);

	// Forward pass.
	int x = x0; // Note that x corresponds to both src and dst images.
	for (;;) {
		// Calculate and write out the next output value.
		history.s0 = dot(b, history);
		p_dst[y0 * dst_stride] = history.s0;

		// Initiate moving to the next column.
		++x;
		coord = get_interpolated_coord(x, dy);
		y0 = coord.lower_bound + y_offset;
		y1 = y0 + 1;
		out_of_bounds = (x >= src_width) | (y0 < -1) | (y1 > src_height);
		if (out_of_bounds) {
			break;
		}

		// Complete moving to the next column.
		++p_src;
		++p_dst;

		// Read the next input pixel.
		pixel = (1.f - coord.alpha) * p_src[y0 * src_stride]
				+ coord.alpha * p_src[y1 * src_stride];

		// Update history.
		history = history.s0012;
		history.s0 = pixel;
	}

	history = updateHistoryForBackwardPass(history, pixel, m1, m2, m3);

	// Backward pass.
	for (--x; x >= x0; --x) {
		// Shift history.
		history = history.s0012;

		y0 = get_interpolated_coord(x, dy).lower_bound + y_offset;

		// Calculate the next output value, put it to history.s0 and write it out.
		history.s0 = p_dst[y0 * dst_stride];
		history.s0 = dot(b, history);
		p_dst[y0 * dst_stride] = history.s0;

		// Move to the previous column.
		--p_dst;
	}
}

kernel void gauss_blur_skewed_vert_traversal_stage2(
	int const width, int const height,
	global float const* const src, int const src_offset, int const src_stride,
	global float* const dst, int const dst_offset, int const dst_stride,
	float const dx)
{
	int const x = get_global_id(0);
	int const y = get_global_id(1);
	bool const outside_bounds = (x >= width) | (y >= height);
	if (outside_bounds) {
		return;
	}

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

kernel void gauss_blur_skewed_hor_traversal_stage2(
	int const width, int const height,
	global float const* const src, int const src_offset, int const src_stride,
	global float* const dst, int const dst_offset, int const dst_stride,
	float const dy)
{
	int const x = get_global_id(0);
	int const y = get_global_id(1);
	bool const outside_bounds = (x >= width) | (y >= height);
	if (outside_bounds) {
		return;
	}

	global float const* const p_src = src + src_offset + x;
	global float* const p_dst = dst + dst_offset + x;
	float const alpha = get_interpolated_coord(x, dy).alpha;

	p_dst[y * dst_stride] = p_src[(y - 1) * src_stride] * alpha
		+ p_src[y * src_stride] * (1.f - alpha);
}
