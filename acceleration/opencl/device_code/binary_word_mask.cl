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
 * Returns edge_masks.s0 (the left one) if "word_idx == 0", edge_masks.s1 (the right one)
 * if "word_idx + 1 == num_words" and a mask with all bits set otherwise.
 */
uint binary_word_mask(uint word_idx, uint num_words, uint2 edge_masks)
{
	uint const is_left = word_idx == 0;
	uint const is_right = word_idx + 1 == num_words;

	uint const inner_mask = ~(uint)0;
	return select(select(inner_mask, edge_masks.s0, is_left), edge_masks.s1, is_right);
}
