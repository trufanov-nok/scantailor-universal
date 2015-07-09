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

#ifndef DEWARPING_DETECT_VERTICAL_BOUNDS_H_
#define DEWARPING_DETECT_VERTICAL_BOUNDS_H_

#include "dewarping_config.h"
#include <QPointF>
#include <QLineF>
#include <utility>
#include <vector>
#include <list>

namespace dewarping
{

/**
 * @brief Takes a list of traced curves and fits two lines to their endpoints.
 *
 * The sampling direction within a curve has to be consistent among all curves.
 * That is, the head of each polyline should correspond to the same side of the page.
 *
 * @param curves The list of curves to process.
 * @param dist_threshold The maximum distance between a vertical bound and a
 *        curve endpoint where the vertical bound is considered supported by
 *        the endpoint.
 * @return A pair of lines representing the vertical bounds. The first line
 *         in a pair corresponds to head endpoints of curves. In case of a
 *         failure, one or both of the returned lines will be null.
 */
DEWARPING_EXPORT std::pair<QLineF, QLineF> detectVerticalBounds(
	std::list<std::vector<QPointF>> const& curves, double dist_threshold);

} // namespace dewarping

#endif
