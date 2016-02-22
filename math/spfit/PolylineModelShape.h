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

#ifndef SPFIT_POLYLINE_MODEL_SHAPE_H_
#define SPFIT_POLYLINE_MODEL_SHAPE_H_

#include "math_config.h"
#include "NonCopyable.h"
#include "ModelShape.h"
#include "SqDistApproximant.h"
#include "XSpline.h"
#include "FlagOps.h"
#include <QPointF>
#include <vector>
#include <functional>

namespace spfit
{

class MATH_EXPORT PolylineModelShape : public ModelShape
{
	DECLARE_NON_COPYABLE(PolylineModelShape)
public:
	enum Flags {
		DEFAULT_FLAGS  = 0,
		POLYLINE_FRONT = 1 << 0,
		POLYLINE_BACK  = 1 << 1
	};

	PolylineModelShape(std::vector<QPointF> const& polyline);

	virtual SqDistApproximant localSqDistApproximant(
		QPointF const& pt, FittableSpline::SampleFlags sample_flags) const;

	void uniformArcLengthSampling(int num_samples,
		std::function<void(QPointF const& pt, double abs_curvature)> const& sink) const;
protected:
	virtual SqDistApproximant calcApproximant(
		QPointF const& pt, FittableSpline::SampleFlags sample_flags,
		Flags polyline_flags, FrenetFrame const& frenet_frame, double signed_curvature) const;
private:
	struct Vertex
	{
		XSpline::PointAndDerivs pd;

		double signedCurvature;

		/** Arc length from the beginning of a spline to this vertex. */
		double cumulativeArcLength;
	};

	std::vector<Vertex> m_vertices;
};

DEFINE_FLAG_OPS(PolylineModelShape::Flags)

} // namespace spfit

#endif
