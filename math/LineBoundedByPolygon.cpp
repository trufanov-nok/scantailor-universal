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

#include "LineBoundedByPolygon.h"
#include "LineIntersectionScalar.h"
#include <limits>

bool lineBoundedByPolygon(QLineF& line, QPolygonF const& poly)
{
	double min_line_proj = std::numeric_limits<double>::max();
	double max_line_proj = std::numeric_limits<double>::lowest();
	int const num_vertices = poly.size() - (poly.isClosed() ? 1 : 0);

	for (int src = 0; src < num_vertices; ++src) {
		int const dst = (src + 1) % num_vertices;
		QLineF const edge(QLineF(poly[src], poly[dst]));

		double edge_proj;
		double line_proj;
		if (!lineIntersectionScalar(edge, line, edge_proj, line_proj)) {
			continue;
		}

		if (edge_proj < -std::numeric_limits<double>::epsilon()) {
			continue;
		}

		if (edge_proj > 1.0 + std::numeric_limits<double>::epsilon()) {
			continue;
		}

		min_line_proj = std::min<double>(min_line_proj, line_proj);
		max_line_proj = std::max<double>(max_line_proj, line_proj);
	}

	if (max_line_proj > min_line_proj) {
		line = QLineF(line.pointAt(min_line_proj), line.pointAt(max_line_proj));
		return true;
	}

	return false;
}
