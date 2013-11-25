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

#include "Utils.h"
#include "dewarping/DepthPerception.h"
#include "dewarping/CylindricalSurfaceDewarper.h"
#include <QPointF>
#include <QLineF>
#include <vector>
#include <cassert>

using namespace dewarping;

namespace deskew
{

void
Utils::buildWarpVisualization(
	std::vector<QPointF> const& top_curve,
	std::vector<QPointF> const& bottom_curve,
	dewarping::DepthPerception const& depth_perception,
	unsigned num_horizontal_curves, unsigned num_vertical_lines,
	std::vector<std::vector<QPointF>>& out_horizontal_curves,
	std::vector<QLineF>& out_vertical_lines)
{
	assert(out_horizontal_curves.empty());
	assert(out_vertical_lines.empty());
	assert(num_vertical_lines > 1);
	assert(num_horizontal_curves > 1);

	out_horizontal_curves.resize(num_horizontal_curves);
	out_vertical_lines.reserve(num_vertical_lines);

	CylindricalSurfaceDewarper const dewarper(
		top_curve, bottom_curve, depth_perception.value()
	);
	CylindricalSurfaceDewarper::State state;

	for (unsigned j = 0; j < num_vertical_lines; ++j) {
		double const x = j / (num_vertical_lines - 1.0);
		CylindricalSurfaceDewarper::Generatrix const gtx(dewarper.mapGeneratrix(x, state));
		QPointF const gtx_p0(gtx.imgLine.pointAt(gtx.pln2img(0)));
		QPointF const gtx_p1(gtx.imgLine.pointAt(gtx.pln2img(1)));
		out_vertical_lines.push_back(QLineF(gtx_p0, gtx_p1));

		for (unsigned i = 0; i < num_horizontal_curves; ++i) {
			double const y = i / (num_horizontal_curves- 1.0);
			out_horizontal_curves[i].push_back(gtx.imgLine.pointAt(gtx.pln2img(y)));
		}
	}
}

} // namespace deskew
