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

// REQUIRES: rgba_color_mixer.cl

kernel void affine_transform(
	read_only image2d_t src, write_only image2d_t dst,
	float2 const xform_m1, float2 const xform_m2, float2 const xform_bias,
	float2 const unit_size, int const outside_area_flags, float4 outside_color)
{
	// Use with outside_area_flags
	enum {
		HAS_OUTSIDE_COLOR = 1 << 0,
		USE_NEAREST_PIXEL = 1 << 1,
		WEAK_OUTSIDE_AREA = 1 << 2
	};

	int const sw = get_image_width(src);
	int const sh = get_image_height(src);
	int const dst_width = get_image_width(dst);
	int const dst_height = get_image_height(dst);
	int const dst_x = get_global_id(0);
	int const dst_y = get_global_id(1);
	int const range_begin = get_global_offset(0);
	bool out_of_bounds = (dst_x >= dst_width) | (dst_y >= dst_height);
	if (out_of_bounds) {
		return;
	}

	int2 const dst_coord = (int2)(dst_x, dst_y);
	sampler_t const sampler = CLK_NORMALIZED_COORDS_FALSE|CLK_ADDRESS_NONE|CLK_FILTER_NEAREST;

	// Map the center of the destination pixel into the source image.
	float2 const dst_center = (float2)((float)dst_x + 0.5f, (float)dst_y + 0.5f);
	float2 const src_center = xform_m1 * dst_center.x + xform_m2 * dst_center.y + xform_bias;

	// Create a box of unit_size around the source point.
	float f_src_left = src_center.x - unit_size.x * 0.5f;
	float f_src_right = src_center.x + unit_size.x * 0.5f;
	float f_src_top = src_center.y - unit_size.y * 0.5f;
	float f_src_bottom = src_center.y + unit_size.y * 0.5f;

	// This check needs to be done in floating points, as otherwise
	// we may overflow integers when converting floating point to integer.
	out_of_bounds = (f_src_right <= 0.f) | (f_src_bottom <= 0.f)
			| (f_src_left >= (float)sw) | (f_src_top >= (float)sh);
	if (out_of_bounds) {
		if (!(outside_area_flags & HAS_OUTSIDE_COLOR)) {
			int2 const nearest_coord = clamp(
				convert_int2_rte(src_center), (int2)(0, 0), (int2)(sw - 1, sh - 1)
			);
			outside_color = read_imagef(src, sampler, nearest_coord);
		}
		write_imagef(dst, dst_coord, outside_color);
		return;
	}

	// Note: the code below is more or less the same as the corresponding
	// code in dewarp.cl

	int src_left, src_right, src_top, src_bottom;
	float left_fraction, top_fraction, right_fraction, bottom_fraction;

	{
		float const f_src_left_floor = floor(f_src_left);
		float const f_src_right_floor = floor(f_src_right);
		float const f_src_top_floor = floor(f_src_top);
		float const f_src_bottom_floor = floor(f_src_bottom);

		left_fraction = (f_src_left_floor + 1.f) - f_src_left;
		top_fraction = (f_src_top_floor + 1.f) - f_src_top;
		right_fraction = f_src_right - f_src_right_floor;
		bottom_fraction = f_src_bottom - f_src_bottom_floor;

		src_left = (int)f_src_left_floor;
		src_right = (int)f_src_right_floor;
		src_top = (int)f_src_top_floor;
		src_bottom = (int)f_src_bottom_floor;
		// Note that src_right and src_bottom are inclusive.
	}

	float background_area = 0.f;

	// These are only valid if we end up with entire src area completely
	// outside of the image.
	int clamp_nearest_x = 0, clamp_nearest_y = 0;

	if (src_top < 0) {
		background_area += -f_src_top * (f_src_right - f_src_left);
		src_top = 0;
		f_src_top = 0.f;
		top_fraction = 1.f;
		clamp_nearest_y = src_top;
	}
	if (src_bottom >= sh) {
		background_area += (f_src_bottom - (float)sh) * (f_src_right - f_src_left);
		src_bottom = sh - 1; // inclusive
		f_src_bottom = (float)sh;
		bottom_fraction = 1.f;
		clamp_nearest_y = src_bottom;
	}
	if (src_left < 0) {
		background_area += -f_src_left * (f_src_bottom - f_src_top);
		src_left = 0;
		f_src_left = 0.f;
		left_fraction = 1.f;
		clamp_nearest_x = src_left;
	}
	if (src_right >= sw) {
		background_area += (f_src_right - (float)sw) * (f_src_bottom - f_src_top);
		src_right = sw - 1; // inclusive
		f_src_right = (float)sw;
		right_fraction = 1.f;
		clamp_nearest_x = src_right;
	}

	// This can happen due to the way we clamp edges.
	out_of_bounds = (src_left > src_right) | (src_top > src_bottom);
	if (out_of_bounds) {
		if (!(outside_area_flags & HAS_OUTSIDE_COLOR)) {
			outside_color = read_imagef(src, sampler, (int2)(clamp_nearest_x, clamp_nearest_y));
		}
		write_imagef(dst, dst_coord, outside_color);
		return;
	}

	float4 pixel;
	float4 color_accum = (float4)(0.f, 0.f, 0.f, 0.f);

	if (outside_area_flags & WEAK_OUTSIDE_AREA) {
		background_area = 0.f;
	} else {
		// assert(outside_area_flags & HAS_OUTSIDE_COLOR);
		rgba_color_mixer_add(&color_accum, outside_color, background_area);
	}

	float const src_area = (f_src_bottom - f_src_top) * (f_src_right - f_src_left);

	if (src_top == src_bottom) {
		if (src_left == src_right) {
			// dst pixel maps to a single src pixel
			float4 const pixel = read_imagef(src, sampler, (int2)(src_left, src_top));
			rgba_color_mixer_add(&color_accum, pixel, src_area);
		} else {
			// dst pixel maps to a horizontal line of src pixels
			float const vert_fraction = f_src_bottom - f_src_top;
			float const left_area = vert_fraction * left_fraction;
			float const middle_area = vert_fraction;
			float const right_area = vert_fraction * right_fraction;

			pixel = read_imagef(src, sampler, (int2)(src_left, src_top));
			rgba_color_mixer_add(&color_accum, pixel, left_area);

			for (int sx = src_left + 1; sx < src_right; ++sx) {
				pixel = read_imagef(src, sampler, (int2)(sx, src_top));
				rgba_color_mixer_add(&color_accum, pixel, middle_area);
			}

			pixel = read_imagef(src, sampler, (int2)(src_right, src_top));
			rgba_color_mixer_add(&color_accum, pixel, right_area);
		}
	} else if (src_left == src_right) {
		// dst pixel maps to a vertical line of src pixels
		float const hor_fraction = f_src_right - f_src_left;
		float const top_area = hor_fraction * top_fraction;
		float const middle_area = hor_fraction;
		float const bottom_area =  hor_fraction * bottom_fraction;

		pixel = read_imagef(src, sampler, (int2)(src_left, src_top));
		rgba_color_mixer_add(&color_accum, pixel, top_area);

		for (int sy = src_top + 1; sy < src_bottom; ++sy) {
			pixel = read_imagef(src, sampler, (int2)(src_left, sy));
			rgba_color_mixer_add(&color_accum, pixel, middle_area);
		}

		pixel = read_imagef(src, sampler, (int2)(src_left, src_bottom));
		rgba_color_mixer_add(&color_accum, pixel, bottom_area);
	} else {
		// dst pixel maps to a block of src pixels
		float const top_area = top_fraction;
		float const bottom_area = bottom_fraction;
		float const left_area = left_fraction;
		float const right_area = right_fraction;
		float const topleft_area = top_fraction * left_fraction;
		float const topright_area = top_fraction * right_fraction;
		float const bottomleft_area = bottom_fraction * left_fraction;
		float const bottomright_area = bottom_fraction * right_fraction;

		// process the top-left corner
		pixel = read_imagef(src, sampler, (int2)(src_left, src_top));
		rgba_color_mixer_add(&color_accum, pixel, topleft_area);

		// process the top line (without corners)
		for (int sx = src_left + 1; sx < src_right; ++sx) {
			pixel = read_imagef(src, sampler, (int2)(sx, src_top));
			rgba_color_mixer_add(&color_accum, pixel, top_area);
		}

		// process the top-right corner
		pixel = read_imagef(src, sampler, (int2)(src_right, src_top));
		rgba_color_mixer_add(&color_accum, pixel, topright_area);

		// process middle lines
		for (int sy = src_top + 1; sy < src_bottom; ++sy) {
			pixel = read_imagef(src, sampler, (int2)(src_left, sy));
			rgba_color_mixer_add(&color_accum, pixel, left_area);

			for (int sx = src_left + 1; sx < src_right; ++sx) {
				pixel = read_imagef(src, sampler, (int2)(sx, sy));
				rgba_color_mixer_add(&color_accum, pixel, 1.f);
			}

			pixel = read_imagef(src, sampler, (int2)(src_right, sy));
			rgba_color_mixer_add(&color_accum, pixel, right_area);
		}

		// process bottom-left corner
		pixel = read_imagef(src, sampler, (int2)(src_left, src_bottom));
		rgba_color_mixer_add(&color_accum, pixel, bottomleft_area);

		// process the bottom line (without corners)
		for (int sx = src_left + 1; sx < src_right; ++sx) {
			pixel = read_imagef(src, sampler, (int2)(sx, src_bottom));
			rgba_color_mixer_add(&color_accum, pixel, bottom_area);
		}

		// process the bottom-right corner
		pixel = read_imagef(src, sampler, (int2)(src_right, src_bottom));
		rgba_color_mixer_add(&color_accum, pixel, bottomright_area);
	}

	pixel = rgba_color_mixer_mix(color_accum, src_area + background_area);
	write_imagef(dst, dst_coord, pixel);
}
