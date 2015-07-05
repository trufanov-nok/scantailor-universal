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

kernel void text_filter_bank_combine(
	int const width, int const height,
	global float const* blurred, int const blurred_offset, int const blurred_stride,
	global float* accum, int const accum_offset, int const accum_stride,
	global uchar* dir_map, int const dir_map_offset, int const dir_map_stride,
	int2 const shoulder, uchar const direction_idx)
{
	blurred += blurred_offset;
	accum += accum_offset;
	dir_map += dir_map_offset;

	int2 const origin = (int2)(get_global_id(0), get_global_id(1));
	bool const outside_bounds = (origin.x >= width) | (origin.y >= height);
	if (outside_bounds) {
		return;
	}

	int2 const pt1 = origin + shoulder;
	int2 const pt2 = origin - shoulder;

	float const origin_px = blurred[origin.x + origin.y * blurred_stride];
	float pt1_px = origin_px;
	float pt2_px = origin_px;

	if (pt1.x >= 0 && pt1.x < width && pt1.y >= 0 && pt1.y < height) {
		pt1_px = blurred[pt1.x + pt1.y * blurred_stride];
	}

	if (pt2.x >= 0 && pt2.x < width && pt2.y >= 0 && pt2.y < height) {
		pt2_px = blurred[pt2.x + pt2.y * blurred_stride];
	}

	float const response = 0.5f * (pt1_px + pt2_px) - origin_px;
	global float* p_accum = accum + origin.x + origin.y * accum_stride;
	if (response > *p_accum) {
		*p_accum = response;
		dir_map[origin.x + origin.y * dir_map_stride] = direction_idx;
	}
}
