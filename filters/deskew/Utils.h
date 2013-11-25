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

#ifndef DESKEW_UTILS_H_
#define DESKEW_UTILS_H_

#include <vector>

class QPointF;
class QLineF;

namespace dewarping
{
	class DepthPerception;
}

namespace deskew
{

class Utils
{
public:
	/**
	 * Generates horizontal and vertical components of a grid used to
	 * visualize the warping model.
	 *
	 * @param top_curve Top curve, left to right.
	 * @param bottom_curve Bottom curve, left to right.
	 * @param depth_perception Affects density of vertical lines in curved areas.
	 * @param num_horizontal_curves The number of horizontal curves to generate.
	 *        Has to be > 1.
	 * @param num_vertical_lines The number of vertical lines to generate.
	 *        Has to be > 1.
	 * @param out_horizontal_curves Generated horizontal curves are saved here.
	 *        The container has to be initially empty.
	 * @param out_vertical_lines Generated vertical curves are saved here.
	 *        The container has to be initially empty.
	 */
	static void buildWarpVisualization(
		std::vector<QPointF> const& top_curve,
		std::vector<QPointF> const& bottom_curve,
		dewarping::DepthPerception const& depth_perception,
		unsigned num_horizontal_curves, unsigned num_vertical_lines,
		std::vector<std::vector<QPointF>>& out_horizontal_curves,
		std::vector<QLineF>& out_vertical_lines);
};

} // namespace deskew

#endif
