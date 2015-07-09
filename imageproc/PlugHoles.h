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

#ifndef IMAGEPROC_PLUG_HOLES_H_
#define IMAGEPROC_PLUG_HOLES_H_

#include "imageproc_config.h"
#include "GridAccessor.h"
#include "Connectivity.h"
#include "TraverseBorders.h"
#include "RasterOpGeneric.h"
#include <QPoint>
#include <QRect>
#include <boost/optional.hpp>
#include <vector>
#include <limits>
#include <algorithm>
#include <type_traits>

namespace imageproc
{

/**
 * @brief Takes a labelled image (a segmentation) and removes regions
 *        completely contained in another region.
 *
 * By "completely contained" we mean that the embedded region only borders
 * a single other region and doesn't touch image borders. Such embedded
 * regions are removed by filling their pixels with the label of the containing
 * region.
 * T has to be an integral type and its values encountered in this image have
 * to be non-negative. This function may need to be called multiple times in
 * order to fill holes-in-holes.
 *
 * @param grid The [mutable] image to operate on.
 * @param conn Determines with neighbouring pixels are considered connected.
 * @param If known, the highest label present in the image.
 *
 * @note The memory consumption of this algorithm is proportional to the value
 *       of the highest label in the grid.
 */
template<typename T>
void plugHoles(GridAccessor<T> const grid,
	Connectivity conn, boost::optional<T> const max_label = boost::none)
{
	static_assert(std::is_integral<T>::value, "plugHoles() only works with integral labels");

	struct Region
	{
		T lowestNeighbour = std::numeric_limits<T>::max();
		T highestNeighbour = std::numeric_limits<T>::min();

		bool const hasExactlyOneNeighbour() const {
			return lowestNeighbour == highestNeighbour;
		}
	};

	std::vector<Region> regions;
	if (max_label) {
		regions.resize(*max_label + 1);
	}

	QPoint const offsets4[] = {
		QPoint(0, -1),
		QPoint(-1, 0), QPoint(1, 0),
		QPoint(0, 1)
	};

	QPoint const offsets8[] = {
		QPoint(-1, -1), QPoint(0, -1), QPoint(1, -1),
		QPoint(-1, 0),                 QPoint(1, 0),
		QPoint(-1, 1),  QPoint(0, 1),  QPoint(1, 1)
	};

	QPoint const* offsets = conn == CONN4 ? offsets4 : offsets8;
	int const num_neighbours = conn == CONN4 ? 4 : 8;

	QRect const rect(0, 0, grid.width, grid.height);
	T* line = grid.data;
	for (int y = 0; y < grid.height; ++y, line += grid.stride) {
		for (int x = 0; x < grid.width; ++x) {
			QPoint const pos(x, y);

			T const label = line[x];
			if (label >= regions.size()) {
				regions.resize(label + 1);
			}
			Region& reg = regions[label];

			for (int i = 0; i < num_neighbours; ++i) {
				QPoint const nbh_pos(pos + offsets[i]);
				if (rect.contains(nbh_pos)) {
					T const nbh_label = grid.data[grid.stride * nbh_pos.y() + nbh_pos.x()];
					if (nbh_label != label) {
						reg.highestNeighbour = std::max<T>(reg.highestNeighbour, nbh_label);
						reg.lowestNeighbour = std::min<T>(reg.lowestNeighbour, nbh_label);
					}
				}
			}
		}
	}

	traverseBorders(grid, [&regions](T label) {
		regions[label] = Region();
	});

	rasterOpGeneric([&regions](T& label) {
		Region const reg(regions[label]);
		if (reg.hasExactlyOneNeighbour()) {
			label = reg.lowestNeighbour;
		}
	}, grid);
}

} // namespace imageproc

#endif
