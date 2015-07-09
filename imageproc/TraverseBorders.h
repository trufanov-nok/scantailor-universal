/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) 2015  Joseph Artsimovich <joseph.artsimich@gmail.com>

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

#ifndef IMAGEPROC_TRAVERSE_BORDERS_H_
#define IMAGEPROC_TRAVERSE_BORDERS_H_

#include "imageproc_config.h"
#include "GridAccessor.h"

namespace imageproc
{

/**
 * @brief Applies a functor to border pixels of an image.
 *
 * The functor is called like this:
 * @code
 * T& pixel = ...;
 * operation(pixel);
 * @endcode
 */
template<typename T, typename Op>
void traverseBorders(GridAccessor<T> const grid, Op operation)
{
	// Top line.
	T* line = grid.data;
	for (int x = 0; x < grid.width; ++x) {
		operation(line[x]);
	}

	// Middle lines.
	for (int y = 1; y < grid.height - 1; ++y) {
		line += grid.stride;
		operation(line[0]);
		operation(line[grid.width - 1]);
	}

	// Last line.
	line += grid.stride;
	for (int x = 0; x < grid.width; ++x) {
		operation(line[x]);
	}
}

} // namespace imageproc

#endif
