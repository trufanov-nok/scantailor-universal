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
 * @brief Take an image with at least 1 pixel padding layer and copy the outermost
 *        layer of image pixels into the innermost padding layer.
 *
 * @param width Width of the image, counting only inner (non-padding) pixels.
 * @param height Height of the image, counting only inner (non-padding) pixels.
 * @param data Points to the first padding pixel.
 * @param inner_offset data[inner_offset] points to the first non-padding pixel.
 * @param stride The distance between a pixel and the one directly below it.
 *
 * @note This kernel is parametrised by a 1D range of [0, 2*width+2*height+4).
 */
kernel void copy_1px_padding(
	int const width, int const height,
	global float* const data,
	int const inner_offset, int const stride)
{
	int offset = get_global_id(0);

	global float* inner_data = data + inner_offset;

	if (offset == 0) {
		// Top-left pixel.
		inner_data[-1 - stride] = inner_data[0];
		return;
	}

	offset -= 1;

	if (offset < width) {
		// Top row.
		inner_data[offset - stride] = inner_data[offset];
		return;
	}

	offset -= width;

	if (offset == 0) {
		// Top-right pixel.
		inner_data[width - stride] = inner_data[width - 1];
		return;
	}

	offset -= 1;

	if (offset < (height << 1)) {
		int const y = offset >> 1;
		int const x = (offset & 1) * (width - 1);
		int const dx = ((offset & 1) << 1) - 1;
		inner_data[y * stride + x + dx] = inner_data[y * stride + x];
		return;
	}

	offset -= height << 1;
	inner_data += (height - 1) * stride;

	if (offset == 0) {
		// Bottom-left pixel.
		inner_data[-1 + stride] = inner_data[0];
		return;
	}

	offset -= 1;

	if (offset < width) {
		// Bottom row.
		inner_data[offset + stride] = inner_data[offset];
		return;
	}

	if (offset == width) {
		// Bottom-right corner.
		inner_data[width + stride] = inner_data[width - 1];
		return;
	}
}
