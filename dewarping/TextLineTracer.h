/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015 Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef DEWARPING_TEXT_LINE_TRACER_H_
#define DEWARPING_TEXT_LINE_TRACER_H_

#include "dewarping_config.h"
#include "Grid.h"
#include "VecNT.h"
#include "acceleration/AcceleratableOperations.h"
#include <QPointF>
#include <QLineF>
#include <vector>
#include <list>
#include <utility>
#include <memory>

class QImage;
class TaskStatus;
class DebugImages;

namespace imageproc
{
	class GrayImage;
	class AffineTransformedImage;
}

namespace dewarping
{

class DistortionModelBuilder;

class DEWARPING_EXPORT TextLineTracer
{
public:
	static void trace(
		imageproc::AffineTransformedImage const& input,
		DistortionModelBuilder& output,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		TaskStatus const& status, DebugImages* dbg = nullptr);
private:
	static float attractionForceAt(
		Grid<float> const& field, Vec2f pos, float outside_force);

	static Grid<Vec2f> calcGradient(imageproc::GrayImage const& image, DebugImages* dbg);

	static Grid<float> calcDirectionalDerivative(
		Grid<Vec2f> const& gradient, Grid<Vec2f> const& downscaled_direction_map);

	static void filterOutOfBoundsCurves(
		std::list<std::vector<QPointF>>& polylines,
		QLineF const& left_bound, QLineF const& right_bound);

	static void filterShortCurves(
		std::list<std::vector<QPointF>>& polylines,
		QLineF const& left_bound, QLineF const& right_bound);

	static QImage visualizeVerticalBounds(
		QImage const& background, std::pair<QLineF, QLineF> bounds);

	static QImage visualizeGradient(QImage const& background, Grid<float> const& grad);

	static QImage visualizePolylines(
		QImage const& background, std::list<std::vector<QPointF>> const& polylines,
		std::pair<QLineF, QLineF> const* vert_bounds = 0);
};

} // namespace dewarping

#endif
