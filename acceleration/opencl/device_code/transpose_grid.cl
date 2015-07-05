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

kernel void transpose_float_grid(
	int const src_width, int const src_height,
	global float const* src, int const src_offset, int const src_stride,
	global float* dst, int const dst_offset, int const dst_stride,
	int const tile_dim, local float* const tile_data, int const tile_stride)
{
	src += src_offset;
	dst += dst_offset;

	int const group_x = get_group_id(0);
	int const group_y = get_group_id(1);
	int const local_width = get_local_size(0);
	int const local_height = get_local_size(1);

	int const local_x = get_local_id(0);
	int const local_y = get_local_id(1);

	int const group_x_tile_dim = mul24(group_x, tile_dim);
	int const group_y_tile_dim = mul24(group_y, tile_dim);

	bool out_of_bounds;

	// Read rows from global memory into rows of local memory.
	for (int tile_y = local_y;; tile_y += local_height) {
		int const global_y = group_y_tile_dim + tile_y;

		out_of_bounds = (tile_y >= tile_dim) | (global_y >= src_height);
		if (out_of_bounds) {
			break;
		}

		for (int tile_x = local_x;; tile_x += local_width) {
			int const global_x = group_x_tile_dim + tile_x;

			out_of_bounds = (tile_x >= tile_dim) | (global_x >= src_width);
			if (out_of_bounds) {
				break;
			}

			tile_data[mad24(tile_y, tile_stride, tile_x)] = src[mad24(global_y, src_stride, global_x)];
		}
	}

	barrier(CLK_LOCAL_MEM_FENCE);

	// Write columns of local memory into rows of global memory.
	for (int tile_x = local_y;; tile_x += local_height) {
		int const global_y = group_x_tile_dim + tile_x;

		out_of_bounds = (tile_x >= tile_dim) | (global_y >= src_width);
		if (out_of_bounds) {
			break;
		}

		for (int tile_y = local_x;; tile_y += local_width) {
			int const global_x = group_y_tile_dim + tile_y;

			out_of_bounds = (tile_y >= tile_dim) | (global_x >= src_height);
			if (out_of_bounds) {
				break;
			}

			dst[mad24(global_y, dst_stride, global_x)] = tile_data[mad24(tile_y, tile_stride, tile_x)];
		}
	}
}
