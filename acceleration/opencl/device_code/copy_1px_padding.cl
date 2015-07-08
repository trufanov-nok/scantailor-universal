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

kernel void copy_1px_column(
	int const height, global float* data,
	int const offset, int const stride, int const dx)
{
	int const y = get_global_id(0);
	if (y >= height) {
		return;
	}

	data += mad24(y, stride, offset);
	data[dx] = data[0];
}

/**
 * @brief Take an image with at least 1 pixel padding layer and copy the 4 corner
 *        pixels of the inner area into the diagonally-adjacent padding pixels.
 *
 * @param width Width of the image, counting only inner (non-padding) pixels.
 * @param height Height of the image, counting only inner (non-padding) pixels.
 * @param data Points to the first padding pixel.
 * @param inner_offset data[inner_offset] points to the first non-padding pixel.
 * @param stride The distance between a pixel and the one directly below it.
 *
 * @note This kernel needs to be enqueued as a task.
 */
kernel void copy_padding_corners(
	int const width, int const height,
	global float* const data,
	int const inner_offset, int const stride)
{
	global float* inner_data = data + inner_offset;

	inner_data[-1 - stride] = inner_data[0];
	inner_data[width - stride] = inner_data[width - 1];

	inner_data += (height - 1) * stride;

	inner_data[-1 + stride] = inner_data[0];
	inner_data[width + stride] = inner_data[width - 1];
}
