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

typedef struct
{
	float2 origin;
	float2 vector;
	float2 homog_m1;
	float2 homog_m2;
	int2 dst_y_range;
}
Generatrix;

float evaluate_1d_homography(Generatrix const* g, float val)
{
	float2 const out = g->homog_m1 * val + g->homog_m2;
	return out.s0 / out.s1;
}

float2 sample_generatrix(
	Generatrix const* g, int y, float model_domain_top, float model_y_scale)
{
	float const model_y = ((float)y - model_domain_top) * model_y_scale;
	return g->origin + g->vector * evaluate_1d_homography(g, model_y);
}

kernel void dewarp(
	read_only image2d_t src, write_only image2d_t dst,
	constant Generatrix const* const generatrix_list,
	int const range_end, float const model_domain_top,
	float const model_y_scale, float4 const bg_color,
	float2 const min_mapping_area)
{
	int const sw = get_image_width(src);
	int const sh = get_image_height(src);
	int const dst_width = get_image_width(dst);
	int const dst_height = get_image_height(dst);
	int const dst_x = get_global_id(0);
	int const dst_y = get_global_id(1);
	int const range_begin = get_global_offset(0);
	bool out_of_bounds = (dst_x >= range_end) | (dst_y >= dst_height);
	if (out_of_bounds) {
		return;
	}

	int2 const dst_coord = (int2)(dst_x, dst_y);
	sampler_t const sampler = CLK_NORMALIZED_COORDS_FALSE|CLK_ADDRESS_NONE|CLK_FILTER_NEAREST;

	Generatrix const left_generatrix = generatrix_list[dst_x - range_begin];
	Generatrix const right_generatrix = generatrix_list[dst_x - range_begin + 1];

	float2 const src_left_points[2] = {
		sample_generatrix(&left_generatrix, dst_y, model_domain_top, model_y_scale),
		sample_generatrix(&left_generatrix, dst_y + 1, model_domain_top, model_y_scale)
	};
	float2 const src_right_points[2] = {
		sample_generatrix(&right_generatrix, dst_y, model_domain_top, model_y_scale),
		sample_generatrix(&right_generatrix, dst_y + 1, model_domain_top, model_y_scale)
	};

	float2 f_src_quad[4];

	// Take a mid-point of each edge.
	f_src_quad[0] = 0.5f * (src_left_points[0] + src_right_points[0]);
	f_src_quad[1] = 0.5f * (src_right_points[0] + src_right_points[1]);
	f_src_quad[2] = 0.5f * (src_right_points[1] + src_left_points[1]);
	f_src_quad[3] = 0.5f * (src_left_points[0] + src_left_points[1]);

	// Calculate the bounding box of src_quad.
	float f_src_left = f_src_quad[0].x;
	float f_src_top = f_src_quad[0].y;
	float f_src_right = f_src_left;
	float f_src_bottom = f_src_top;

	for (int i = 1; i < 4; ++i) {
		float2 const pt = f_src_quad[i];
		f_src_left = min(f_src_left, pt.x);
		f_src_right = max(f_src_right, pt.x);
		f_src_top = min(f_src_top, pt.y);
		f_src_bottom = max(f_src_bottom, pt.y);
	}

	// Enforce the minimum mapping area.
	if (f_src_right - f_src_left < min_mapping_area.x) {
		float const midpoint = 0.5f * (f_src_left + f_src_right);
		f_src_left = midpoint - min_mapping_area.x * 0.5f;
		f_src_right = midpoint + min_mapping_area.x * 0.5f;
	}
	if (f_src_bottom - f_src_top < min_mapping_area.y) {
		float const midpoint = 0.5f * (f_src_top + f_src_bottom);
		f_src_top = midpoint - min_mapping_area.y * 0.5f;
		f_src_bottom = midpoint + min_mapping_area.y * 0.5f;
	}

	// Note: the code below is more or less the same as in transformGeneric()
	// in imageproc/Transform.cpp

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

	out_of_bounds =
			(src_right < 0) | (src_bottom < 0) |
			(src_left >= sw) | (src_top >= sh) |
			(dst_y < min(left_generatrix.dst_y_range.s0, right_generatrix.dst_y_range.s0)) |
			(dst_y > max(left_generatrix.dst_y_range.s1, right_generatrix.dst_y_range.s1));

	if (out_of_bounds) {
		write_imagef(dst, dst_coord, bg_color);
		return;
	}

	float background_area = 0.f;

	if (src_top < 0) {
		background_area += -f_src_top * (f_src_right - f_src_left);
		src_top = 0;
		f_src_top = 0.f;
		top_fraction = 1.f;
	}
	if (src_bottom >= sh) {
		background_area += (f_src_bottom - (float)sh) * (f_src_right - f_src_left);
		src_bottom = sh - 1; // inclusive
		f_src_bottom = (float)sh;
		bottom_fraction = 1.f;
	}
	if (src_left < 0) {
		background_area += -f_src_left * (f_src_bottom - f_src_top);
		src_left = 0;
		f_src_left = 0.f;
		left_fraction = 1.f;
	}
	if (src_right >= sw) {
		background_area += (f_src_right - (float)sw) * (f_src_bottom - f_src_top);
		src_right = sw - 1; // inclusive
		f_src_right = (float)sw;
		right_fraction = 1.f;
	}

	out_of_bounds = (src_left > src_right) | (src_top > src_bottom);
	if (out_of_bounds) {
		// This can happen due to the way we clamp edges.
		write_imagef(dst, dst_coord, bg_color);
		return;
	}

	float4 pixel;
	float4 color_accum = (float4)(0.f, 0.f, 0.f, 0.f);
	rgba_color_mixer_add(&color_accum, bg_color, background_area);

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
