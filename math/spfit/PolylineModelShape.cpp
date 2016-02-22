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

#include "PolylineModelShape.h"
#include "FrenetFrame.h"
#include "NumericTraits.h"
#include "XSpline.h"
#include "VecNT.h"
#include "ToLineProjector.h"
#include "ToVec.h"
#include <stdexcept>
#include <limits>
#include <cmath>
#include <cassert>

namespace spfit
{

PolylineModelShape::PolylineModelShape(std::vector<QPointF> const& polyline)
{
	if (polyline.size() <= 1) {
		throw std::invalid_argument("PolylineModelShape: polyline must have at least 2 vertices");
	}

	// We build an interpolating X-spline with control points at the vertices
	// of our polyline.  We'll use it to calculate curvature at polyline vertices.
	XSpline spline;

	for (QPointF const& pt : polyline) {
		spline.appendControlPoint(pt, -1);
	}
	
	int const num_control_points = spline.numControlPoints();
	double const scale = 1.0 / (num_control_points - 1);
	double cumulative_arc_length = 0;
	QPointF last_pt;

	for (int i = 0; i < num_control_points; ++i) {
		XSpline::PointAndDerivs const& pd = spline.pointAndDtsAt(i * scale);
		if (i > 0) {
			cumulative_arc_length += QLineF(last_pt, pd.point).length();
		}
		m_vertices.push_back(Vertex{pd, pd.signedCurvature(), cumulative_arc_length});
		last_pt = pd.point;
	}
}

SqDistApproximant
PolylineModelShape::localSqDistApproximant(
	QPointF const& pt, FittableSpline::SampleFlags sample_flags) const
{
	if (m_vertices.empty()) {
		return SqDistApproximant();
	}

	// First, find the point on the polyline closest to pt.
	QPointF best_foot_point;
	double best_sqdist = NumericTraits<double>::max();
	double segment_t = -1;
	int segment_idx = -1; // If best_foot_point is on a segment, its index goes here.
	int vertex_idx = -1; // If best_foot_point is a vertex, its index goes here.

	// Project pt to each segment.
	int const num_segments = int(m_vertices.size()) - 1;
	for (int i = 0; i < num_segments; ++i) {
		QPointF const pt1(m_vertices[i].pd.point);
		QPointF const pt2(m_vertices[i + 1].pd.point);
		QLineF const segment(pt1, pt2);
		double const s = ToLineProjector(segment).projectionScalar(pt);
		if (s > 0 && s < 1) {
			QPointF const foot_point(segment.pointAt(s));
			Vec2d const vec(pt - foot_point);
			double const sqdist = vec.squaredNorm();
			if (sqdist < best_sqdist) {
				best_sqdist = sqdist;
				best_foot_point = foot_point;
				segment_idx = i;
				segment_t = s;
				vertex_idx = -1;
			}
		}
	}

	// Check if pt is closer to a vertex than to any segment.
	int const num_points = m_vertices.size();
	for (int i = 0; i < num_points; ++i) {
		QPointF const vtx(m_vertices[i].pd.point);
		Vec2d const vec(pt - vtx);
		double const sqdist = vec.squaredNorm();
		if (sqdist < best_sqdist) {
			best_sqdist = sqdist;
			best_foot_point = vtx;
			vertex_idx = i;
			segment_idx = -1;
		}
	}

	if (segment_idx != -1) {
		// The foot point is on a line segment.
		assert(segment_t >= 0 && segment_t <= 1);

		Vertex const& vtx1 = m_vertices[segment_idx];
		Vertex const& vtx2 = m_vertices[segment_idx + 1];
		FrenetFrame const frenet_frame(toVec(best_foot_point), toVec(vtx2.pd.point - vtx1.pd.point));

		double const k1 = vtx1.signedCurvature;
		double const k2 = vtx2.signedCurvature;
		double const weighted_k = k1 + segment_t * (k2 - k1);
		
		return calcApproximant(pt, sample_flags, DEFAULT_FLAGS, frenet_frame, weighted_k);
	} else {
		// The foot point is a vertex of the polyline.
		assert(vertex_idx != -1);

		Vertex const& vtx = m_vertices[vertex_idx];
		FrenetFrame const frenet_frame(toVec(best_foot_point), toVec(vtx.pd.firstDeriv));

		Flags polyline_flags = DEFAULT_FLAGS;
		if (vertex_idx == 0) {
			polyline_flags |= POLYLINE_FRONT;
		}
		if (vertex_idx == int(m_vertices.size()) - 1) {
			polyline_flags |= POLYLINE_BACK;
		}

		return calcApproximant(pt, sample_flags, polyline_flags, frenet_frame, vtx.signedCurvature);
	}
}

SqDistApproximant
PolylineModelShape::calcApproximant(
	QPointF const& pt, FittableSpline::SampleFlags const sample_flags,
	Flags const polyline_flags, FrenetFrame const& frenet_frame,
	double const signed_curvature) const
{
	if (sample_flags & (FittableSpline::HEAD_SAMPLE|FittableSpline::TAIL_SAMPLE)) {
		return SqDistApproximant::pointDistance(frenet_frame.origin());
	} else {
		return SqDistApproximant::curveDistance(toVec(pt), frenet_frame, signed_curvature);
	}
}

void
PolylineModelShape::uniformArcLengthSampling(int num_samples,
	std::function<void(QPointF const& pt, double abs_curvature)> const& sink) const
{
	if (num_samples <= 0 || m_vertices.empty()) {
		return;
	}

	auto vtx_it = m_vertices.begin();
	sink(vtx_it->pd.point, std::abs(vtx_it->signedCurvature));

	++vtx_it;
	auto const vtx_end = m_vertices.end();
	if (vtx_it == vtx_end)
	{
		return;
	}

	double const arc_length_step = m_vertices.back().cumulativeArcLength / (num_samples - 1);

	for (int sample_idx = 1; sample_idx < num_samples - 1; ++sample_idx) {
		double const target_arc_length = arc_length_step * sample_idx;

		while (vtx_it != vtx_end) {
			if (vtx_it->cumulativeArcLength >= target_arc_length) {
				double const gap = vtx_it->cumulativeArcLength - (vtx_it - 1)->cumulativeArcLength;
				double const alpha = (vtx_it->cumulativeArcLength - target_arc_length) / gap;
				QPointF const pt = alpha * (vtx_it - 1)->pd.point + (1 - alpha) * vtx_it->pd.point;
				double const abs_curvature = alpha * std::abs((vtx_it - 1)->signedCurvature) +
						(1 - alpha) * std::abs(vtx_it->signedCurvature);

				sink(pt, abs_curvature);
				break; // Advance to the next output sample.
			}
			++vtx_it;
		}
	}

	sink(m_vertices.back().pd.point, std::abs(m_vertices.back().signedCurvature));
}

} // namespace spfit
