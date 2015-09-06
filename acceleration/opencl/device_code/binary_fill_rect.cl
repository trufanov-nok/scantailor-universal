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

// REQUIRES: binary_word_mask.cl

kernel void binary_fill_rect(
	int const rect_width, int const rect_height,
	global uint* image, int const image_offset, int const image_stride,
	uint const fill_word, uint2 const edge_masks)
{
	int const word_x = get_global_id(0);
	int const word_y = get_global_id(1);

	if (word_x >= rect_width | word_y >= rect_height) {
		return;
	}

	image += image_offset + mad24(word_y, image_stride, word_x);

	uint const mask = binary_word_mask(get_global_id(0), rect_width, edge_masks);

	if (mask == ~(uint)0) {
		*image = fill_word;
	} else {
		// This branch alone would do just fine, though having two
		// branches does result in slightly better performance on all 3
		// OpenCL devices I've got.
		*image = bitselect(*image, fill_word, mask);
	}
}
