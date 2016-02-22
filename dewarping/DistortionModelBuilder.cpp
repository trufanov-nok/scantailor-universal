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

#include "DistortionModelBuilder.h"
#include "DistortionModel.h"
#include "CylindricalSurfaceDewarper.h"
#include "LineBoundedByRect.h"
#include "ToLineProjector.h"
#include "SidesOfLine.h"
#include "XSpline.h"
#include "DebugImages.h"
#include "VecNT.h"
#include "ToVec.h"
#include <QTransform>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QDebug>
#include <boost/foreach.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <algorithm>
#include <exception>
#include <iterator>
#include <limits>
#include <cmath>
#include <cassert>

using namespace imageproc;

namespace dewarping
{

struct DistortionModelBuilder::TracedCurve
{
	std::vector<QPointF> trimmedPolyline;  // Both are left to right.
	std::vector<QPointF> extendedPolyline; //
	XSpline extendedSpline;
	double order; // Lesser values correspond to upper curves.

	TracedCurve(std::vector<QPointF> const& trimmed_polyline,
		XSpline const& extended_spline, double ord)
		: trimmedPolyline(trimmed_polyline),
		extendedPolyline(extended_spline.toPolyline()),
		extendedSpline(extended_spline), order(ord) {}

	bool operator<(TracedCurve const& rhs) const { return order < rhs.order; }
};

struct DistortionModelBuilder::RansacModel
{
	TracedCurve const* topCurve;
	TracedCurve const* bottomCurve; 
	double totalError;

	RansacModel() : topCurve(0), bottomCurve(0), totalError(NumericTraits<double>::max()) {}

	bool isValid() const { return topCurve && bottomCurve; }
};

class DistortionModelBuilder::RansacAlgo
{
public:
	RansacAlgo(std::vector<TracedCurve> const& all_curves)
		: m_rAllCurves(all_curves) {}

	void buildAndAssessModel(
		TracedCurve const* top_curve, TracedCurve const* bottom_curve);

	RansacModel& bestModel() { return m_bestModel; }

	RansacModel const& bestModel() const { return m_bestModel; }
private:
	RansacModel m_bestModel;
	std::vector<TracedCurve> const& m_rAllCurves;
};


class DistortionModelBuilder::BadCurve : public std::exception
{
public:
	virtual char const* what() const throw() {
		return "Bad curve";
	}
};


DistortionModelBuilder::DistortionModelBuilder(Vec2d const& down_direction)
:	m_downDirection(down_direction),
	m_rightDirection(down_direction[1], -down_direction[0])
{
	assert(down_direction.squaredNorm() > 0);
}

void
DistortionModelBuilder::setVerticalBounds(QLineF const& bound1, QLineF const& bound2)
{
	m_bound1 = bound1;
	m_bound2 = bound2;
}

std::pair<QLineF, QLineF>
DistortionModelBuilder::verticalBounds() const
{
	return std::pair<QLineF, QLineF>(m_bound1, m_bound2);
}

void
DistortionModelBuilder::addHorizontalCurve(std::vector<QPointF> const& polyline)
{
	if (polyline.size() < 2) {
		return;
	}

	if (Vec2d(polyline.back() - polyline.front()).dot(m_rightDirection) > 0) {
		m_ltrPolylines.push_back(polyline);
	} else {
		m_ltrPolylines.push_back(std::vector<QPointF>(polyline.rbegin(), polyline.rend()));
	}
}

void
DistortionModelBuilder::transform(QTransform const& xform)
{
	assert(xform.isAffine());

	QLineF const down_line(xform.map(QLineF(QPointF(0, 0), m_downDirection)));
	QLineF const right_line(xform.map(QLineF(QPointF(0, 0), m_rightDirection)));

	m_downDirection = down_line.p2() - down_line.p1();
	m_rightDirection = right_line.p2() - right_line.p1();
	m_bound1 = xform.map(m_bound1);
	m_bound2 = xform.map(m_bound2);

	BOOST_FOREACH(std::vector<QPointF>& polyline, m_ltrPolylines) {
		BOOST_FOREACH(QPointF& pt, polyline) {
			pt = xform.map(pt);
		}
	}
}

DistortionModel
DistortionModelBuilder::tryBuildModel(DebugImages* dbg, QImage const* dbg_background) const
{
	int num_curves = m_ltrPolylines.size();

	if (num_curves < 2 || m_bound1.p1() == m_bound1.p2() || m_bound2.p1() == m_bound2.p2()) {
		return DistortionModel();
	}

	std::vector<TracedCurve> ordered_curves;
	ordered_curves.reserve(num_curves);
	
	BOOST_FOREACH(std::vector<QPointF> const& polyline, m_ltrPolylines) {
		try {
			ordered_curves.push_back(polylineToCurve(polyline));
		} catch (BadCurve const&) {
			// Just skip it.
		}
	}
	num_curves = ordered_curves.size();
	if (num_curves < 2) {
		return DistortionModel();
	}

	std::sort(ordered_curves.begin(), ordered_curves.end());

	// Select the best pair using RANSAC.
	RansacAlgo ransac(ordered_curves);

	ransac.buildAndAssessModel(&ordered_curves.front(), &ordered_curves.back());

	// First let's try to combine each of the 5 top-most lines
	// with each of the 3 bottom-most ones.
	int const exhaustive_search_threshold = 5;
	for (int i = 0; i < std::min<int>(exhaustive_search_threshold, num_curves); ++i) {
		for (int j = std::max<int>(0, num_curves - exhaustive_search_threshold); j < num_curves; ++j) {
			if (i < j) {
				ransac.buildAndAssessModel(&ordered_curves[i], &ordered_curves[j]);
			}
		}
	}

	// Continue by throwing in some random pairs of lines.

	// Repeatablity is important, so don't seed the RNG.
	boost::random::mt19937 rng;
	boost::random::uniform_int_distribution<> rng_dist(0, num_curves - 1);
	int random_pairs_remaining = num_curves <= exhaustive_search_threshold
		? 0 : exhaustive_search_threshold * exhaustive_search_threshold;
	while (random_pairs_remaining-- > 0) {
		int i = rng_dist(rng);
		int j = rng_dist(rng);
		if (i > j) {
			std::swap(i, j);
		}
		if (i < j) {
			ransac.buildAndAssessModel(&ordered_curves[i], &ordered_curves[j]);
		}
	}

	if (dbg && dbg_background) {
		dbg->add(visualizeTrimmedPolylines(*dbg_background, ordered_curves), "trimmed_polylines");
		dbg->add(visualizeModel(*dbg_background, ordered_curves, ransac.bestModel()), "distortion_model");
	}

	DistortionModel model;
	if (ransac.bestModel().isValid()) {
		model.setTopCurve(Curve(ransac.bestModel().topCurve->extendedPolyline));
		model.setBottomCurve(Curve(ransac.bestModel().bottomCurve->extendedPolyline));
	}
	return model;
}

DistortionModelBuilder::TracedCurve
DistortionModelBuilder::polylineToCurve(std::vector<QPointF> const& polyline) const
{
	std::pair<QLineF, QLineF> const bounds(frontBackBounds(polyline));

	// Trim the polyline if necessary.
	std::vector<QPointF> const trimmed_polyline(maybeTrimPolyline(polyline, bounds));
	
	// Fit a spline to a polyline, extending it to bounds at the same time.
	XSpline const extended_spline(fitExtendedSpline(trimmed_polyline, bounds));
	
	double const order = centroid(polyline).dot(m_downDirection);
	return TracedCurve(trimmed_polyline, extended_spline, order);
}

Vec2d
DistortionModelBuilder::centroid(std::vector<QPointF> const& polyline)
{
	int const num_points = polyline.size();
	if (num_points == 0) {
		return Vec2d();
	} else if (num_points == 1) {
		return Vec2d(polyline.front());
	}

	Vec2d accum(0, 0);
	double total_weight = 0;

	for (int i = 1; i < num_points; ++i) {
		QLineF const segment(polyline[i - 1], polyline[i]);
		Vec2d const center(0.5 * (segment.p1() + segment.p2()));
		double const weight = segment.length();
		accum += center * weight;
		total_weight += weight;
	}

	if (total_weight < 1e-06) {
		return Vec2d(polyline.front());
	} else {
		return accum / total_weight;
	}
}

/**
 * \brief Returns bounds ordered according to the direction of \p polyline.
 *
 * The first and second bounds will correspond to polyline.front() and polyline.back()
 * respectively.
 */
std::pair<QLineF, QLineF>
DistortionModelBuilder::frontBackBounds(std::vector<QPointF> const& polyline) const
{
	assert(!polyline.empty());

	ToLineProjector const proj1(m_bound1);
	ToLineProjector const proj2(m_bound2);
	if (proj1.projectionDist(polyline.front()) + proj2.projectionDist(polyline.back()) <
			proj1.projectionDist(polyline.back()) + proj2.projectionDist(polyline.front())) {
		return std::pair<QLineF, QLineF>(m_bound1, m_bound2);
	} else {
		return std::pair<QLineF, QLineF>(m_bound2, m_bound1);
	}
}

std::vector<QPointF>
DistortionModelBuilder::maybeTrimPolyline(
	std::vector<QPointF> const& polyline, std::pair<QLineF, QLineF> const& bounds)
{
	std::deque<QPointF> trimmed_polyline(polyline.begin(), polyline.end());
	maybeTrimFront(trimmed_polyline, bounds.first);
	maybeTrimBack(trimmed_polyline, bounds.second);
	return std::vector<QPointF>(trimmed_polyline.begin(), trimmed_polyline.end());
}

bool
DistortionModelBuilder::maybeTrimFront(std::deque<QPointF>& polyline, QLineF const& bound)
{
	if (sidesOfLine(bound, polyline.front(), polyline.back()) >= 0) {
		// Doesn't need trimming.
		return false;
	}

	while (polyline.size() > 2 && sidesOfLine(bound, polyline.front(), polyline[1]) > 0) {
		polyline.pop_front();
	}
	
	intersectFront(polyline, bound);
	
	return true;
}

bool
DistortionModelBuilder::maybeTrimBack(std::deque<QPointF>& polyline, QLineF const& bound)
{
	if (sidesOfLine(bound, polyline.front(), polyline.back()) >= 0) {
		// Doesn't need trimming.
		return false;
	}

	while (polyline.size() > 2 && sidesOfLine(bound, polyline[polyline.size() - 2], polyline.back()) > 0) {
		polyline.pop_back();
	}
	
	intersectBack(polyline, bound);
	
	return true;
}

void
DistortionModelBuilder::intersectFront(
	std::deque<QPointF>& polyline, QLineF const& bound)
{
	assert(polyline.size() >= 2);

	QLineF const front_segment(polyline.front(), polyline[1]);
	QPointF intersection;
	if (bound.intersect(front_segment, &intersection) != QLineF::NoIntersection) {
		polyline.front() = intersection;
	}
}

void
DistortionModelBuilder::intersectBack(
	std::deque<QPointF>& polyline, QLineF const& bound)
{
	assert(polyline.size() >= 2);

	QLineF const back_segment(polyline[polyline.size() - 2], polyline.back());
	QPointF intersection;
	if (bound.intersect(back_segment, &intersection) != QLineF::NoIntersection) {
		polyline.back() = intersection;
	}
}

XSpline
DistortionModelBuilder::fitExtendedSpline(
	std::vector<QPointF> const& polyline, std::pair<QLineF, QLineF> const& bounds)
{
	XSpline spline;

	// Left extension.
	{
		QLineF const line(polyline[0], polyline[1]);
		QPointF intersection;
		if (line.intersect(bounds.first, &intersection) != QLineF::NoIntersection) {
			if (Vec2d(intersection - polyline[0]).squaredNorm() > 1.0) {
				spline.appendControlPoint(intersection, -1.0);
			}
		}
	}

	for (QPointF const& pt : polyline) {
		spline.appendControlPoint(pt, -1.0);
	}

	// Right extension.
	{
		QLineF const line(polyline[polyline.size() - 2], polyline.back());
		QPointF intersection;
		if (line.intersect(bounds.second, &intersection) != QLineF::NoIntersection) {
			if (Vec2d(intersection - polyline.back()).squaredNorm() > 1.0) {
				spline.appendControlPoint(intersection, -1.0);
			}
		}
	}

	return spline;
}


/*============================== RansacAlgo ============================*/

void
DistortionModelBuilder::RansacAlgo::buildAndAssessModel(
	TracedCurve const* top_curve, TracedCurve const* bottom_curve)
try {
	DistortionModel model;
	model.setTopCurve(Curve(top_curve->extendedPolyline));
	model.setBottomCurve(Curve(bottom_curve->extendedPolyline));
	if (!model.isValid()) {
		return;
	}

	double const depth_perception = 2.0; // Doesn't matter much here.
	CylindricalSurfaceDewarper const dewarper(
		top_curve->extendedPolyline, bottom_curve->extendedPolyline, depth_perception
	);

	// CylindricalSurfaceDewarper maps the curved quadrilateral to a unit
	// square. We introduce additional scaling to map it to a 1000x1000
	// square, so that sqrt() applied on distances has a more familiar effect.
	auto dewarp = [&dewarper](QPointF const& pt) {
		return dewarper.mapToDewarpedSpace(pt) * 1000.0;
	};

	double total_error = 0;
	for (TracedCurve const& curve : m_rAllCurves) {
		size_t const polyline_size = curve.trimmedPolyline.size();
		assert(polyline_size > 0); // Guaranteed by addHorizontalCurve().

		// We want to penalize the line both for being not straight and also
		// for being non-horizontal. The penalty metric we use is:
		// sqrt(max_y(dewarped_points) - min_y(dewarped_points) + 1.0) - 1.0
		// The square root is necessary to de-emphasize outliers.

		double min_y = std::numeric_limits<double>::max();
		double max_y = std::numeric_limits<double>::min();

		for (QPointF const& warped_pt : curve.trimmedPolyline) {
			// TODO: add another signature with hint for efficiency.
			QPointF const dewarped_pt(dewarp(warped_pt));

			min_y = std::min<double>(min_y, dewarped_pt.y());
			max_y = std::max<double>(max_y, dewarped_pt.y());
		}

		total_error += std::sqrt(max_y - min_y + 1.0) - 1.0;
	}

	// We want to promote curves that reach to the vertical boundaries
	// over those that had to be extended. To do that, we add a square
	// root of extension distance to total_error.

	auto get_dewarped_distance = [dewarp](QPointF const& warped_pt1, QPointF const& warped_pt2) {
		QPointF const dewarped_pt1 = dewarp(warped_pt1);
		QPointF const dewarped_pt2 = dewarp(warped_pt2);
		return Vec2d(dewarped_pt1 - dewarped_pt2).norm();
	};

	auto get_extension_distance = [get_dewarped_distance](TracedCurve const* curve) {
		auto const& extended = curve->extendedPolyline;
		auto const& trimmed = curve->trimmedPolyline;
		double const front = get_dewarped_distance(extended.front(), trimmed.front());
		double const back = get_dewarped_distance(extended.back(), trimmed.back());
		return front + back;
	};

	double const top_curve_extension = get_extension_distance(top_curve);
	double const bottom_curve_extension = get_extension_distance(bottom_curve);
	double const extension_importance_factor = 1.0;

	total_error += extension_importance_factor * (
		std::sqrt(top_curve_extension + 1.0) - 1.0 +
		std::sqrt(bottom_curve_extension + 1.0) - 1.0
	);

	if (total_error < m_bestModel.totalError) {
		m_bestModel.topCurve = top_curve;
		m_bestModel.bottomCurve = bottom_curve;
		m_bestModel.totalError = total_error;
	}
} catch (std::runtime_error const&) {
	// Probably CylindricalSurfaceDewarper didn't like something.
}

QImage
DistortionModelBuilder::visualizeTrimmedPolylines(
	QImage const& background, std::vector<TracedCurve> const& curves) const
{
	QImage canvas(background.convertToFormat(QImage::Format_RGB32));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);

	int const width = background.width();
	int const height = background.height();
	double const stroke_width = sqrt(double(width * width + height * height)) / 500;

	// Extend / trim bounds.
	QLineF bound1(m_bound1);
	QLineF bound2(m_bound2);
	lineBoundedByRect(bound1, background.rect());
	lineBoundedByRect(bound2, background.rect());

	// Draw bounds.
	QPen pen(QColor(0, 0, 255, 180));
	pen.setWidthF(stroke_width);
	painter.setPen(pen);
	painter.drawLine(bound1);
	painter.drawLine(bound2);

	BOOST_FOREACH(TracedCurve const& curve, curves) {
		if (!curve.trimmedPolyline.empty()) {
			painter.drawPolyline(&curve.trimmedPolyline[0], curve.trimmedPolyline.size());
		}
	}

	// Draw polyline knots.
	QBrush knot_brush(Qt::magenta);
	painter.setBrush(knot_brush);
	painter.setPen(Qt::NoPen);
	BOOST_FOREACH(TracedCurve const& curve, curves) {
		QRectF rect(0, 0, stroke_width, stroke_width);
		BOOST_FOREACH(QPointF const& knot, curve.trimmedPolyline) {
			rect.moveCenter(knot);
			painter.drawEllipse(rect);
		}
	}

	return canvas;
}

QImage
DistortionModelBuilder::visualizeModel(
	QImage const& background, std::vector<TracedCurve> const& curves,
	RansacModel const& model) const
{
	QImage canvas(background.convertToFormat(QImage::Format_RGB32));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);

	int const width = background.width();
	int const height = background.height();
	double const stroke_width = sqrt(double(width * width + height * height)) / 500;

	// Extend / trim bounds.
	QLineF bound1(m_bound1);
	QLineF bound2(m_bound2);
	lineBoundedByRect(bound1, background.rect());
	lineBoundedByRect(bound2, background.rect());

	// Draw bounds.
	QPen bounds_pen(QColor(0, 0, 255, 180));
	bounds_pen.setWidthF(stroke_width);
	painter.setPen(bounds_pen);
	painter.drawLine(bound1);
	painter.drawLine(bound2);

	QPen active_curve_pen(QColor(0x45, 0xff, 0x53, 180));
	active_curve_pen.setWidthF(stroke_width);
	
	QPen inactive_curve_pen(QColor(0, 0, 255, 140));
	inactive_curve_pen.setWidthF(stroke_width);

	QPen reverse_segments_pen(QColor(0xff, 0x28, 0x05, 140));
	reverse_segments_pen.setWidthF(stroke_width);

	QBrush control_point_brush(QColor(0xff, 0x00, 0x00, 255));

	QBrush junction_point_brush(QColor(0xff, 0x00, 0xff, 255));

	QBrush polyline_knot_brush(QColor(0xff, 0x00, 0xff, 180));

	BOOST_FOREACH(TracedCurve const& curve, curves) {
		if (curve.extendedPolyline.empty()) {
			continue;
		}
		if (&curve == model.topCurve || &curve == model.bottomCurve) {
			painter.setPen(active_curve_pen);
		} else {
			painter.setPen(inactive_curve_pen);
		}

		size_t const size = curve.extendedPolyline.size();
		painter.drawPolyline(&curve.extendedPolyline[0], curve.extendedPolyline.size());

		Vec2d const main_direction(curve.extendedPolyline.back() - curve.extendedPolyline.front());
		std::list<std::vector<int> > reverse_segments;

		for (size_t i = 1; i < size; ++i) {
			Vec2d const dir(curve.extendedPolyline[i] - curve.extendedPolyline[i - 1]);
			if (dir.dot(main_direction) >= 0) {
				continue;
			}

			// We've got a reverse segment.
			if (!reverse_segments.empty() && reverse_segments.back().back() == int(i) - 1) {
				// Continue the previous sequence.
				reverse_segments.back().push_back(i);
			} else {
				// Start a new sequence.
				reverse_segments.push_back(std::vector<int>());
				std::vector<int>& sequence = reverse_segments.back();
				sequence.push_back(i - 1);
				sequence.push_back(i);
			}
		}

		QVector<QPointF> polyline;

		if (!reverse_segments.empty()) {
			painter.setPen(reverse_segments_pen);
			BOOST_FOREACH(std::vector<int> const& sequence, reverse_segments) {
				assert(!sequence.empty());
				polyline.clear();
				BOOST_FOREACH(int idx, sequence) {
					polyline << curve.extendedPolyline[idx];
				}
				painter.drawPolyline(polyline); 
			}
		}

		int const num_control_points = curve.extendedSpline.numControlPoints();
		QRectF rect(0, 0, stroke_width, stroke_width);
#if 0
		// Draw junction points.
		painter.setPen(Qt::NoPen);
		painter.setBrush(junction_point_brush);
		for (int i = 0; i < num_control_points; ++i) {
			double const t = curve.extendedSpline.controlPointIndexToT(i);
			rect.moveCenter(curve.extendedSpline.pointAt(t));
			painter.drawEllipse(rect);
		}

		// Draw control points.
		painter.setPen(Qt::NoPen);
		painter.setBrush(control_point_brush);
		for (int i = 0; i < num_control_points; ++i) {
			rect.moveCenter(curve.extendedSpline.controlPointPosition(i));
			painter.drawEllipse(rect);
		}
#endif
		// Draw original polyline knots.
		painter.setPen(Qt::NoPen);
		painter.setBrush(polyline_knot_brush);
		for (QPointF const& knot : curve.trimmedPolyline) {
			rect.moveCenter(knot);
			painter.drawEllipse(rect);
		}
	}

	return canvas;
}

} // namespace dewarping
