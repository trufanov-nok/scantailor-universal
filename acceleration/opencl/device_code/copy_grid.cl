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

kernel void copy_float_grid(
	int const width, int const height,
	global float const* const src, int const src_offset, int const src_stride,
	global float* const dst, int const dst_offset, int const dst_stride)
{
	int const x = get_global_id(0);
	int const y = get_global_id(1);
	bool const outside_bounds = (x >= width) | (y >= height);
	if (outside_bounds) {
		return;
	}

	dst[dst_offset + dst_stride * y + x] = src[src_offset + src_stride * y + x];
}
