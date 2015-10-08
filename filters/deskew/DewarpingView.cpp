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

#include "DewarpingView.h"
#include "ImagePresentation.h"
#include "VecNT.h"
#include "NumericTraits.h"
#include "ToLineProjector.h"
#include "XSpline.h"
#include "Utils.h"
#include "dewarping/Curve.h"
#include "spfit/SplineFitter.h"
#include "spfit/ConstraintSet.h"
#include "spfit/PolylineModelShape.h"
#include "spfit/LinearForceBalancer.h"
#include "spfit/OptimizationResult.h"
#include "imageproc/AffineTransformedImage.h"
#include "imageproc/Constants.h"
#include <QCursor>
#include <QLineF>
#include <QPolygonF>
#include <QTransform>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QColor>
#include <Qt>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#endif
#include <vector>
#include <stdexcept>

namespace deskew
{

using namespace imageproc;

DewarpingView::DewarpingView(
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	AffineTransformedImage const& full_size_image,
	ImagePixmapUnion const& downscaled_image,
	PageId const& page_id,
	dewarping::DistortionModel const& distortion_model,
	dewarping::DepthPerception const& depth_perception,
	bool const fixed_number_of_control_points)
:	ImageViewBase(
		accel_ops, full_size_image.origImage(), downscaled_image,
		ImagePresentation(
			full_size_image.xform().transform(),
			full_size_image.xform().transformedCropArea()
		),
		QMarginsF(5.0, 5.0, 5.0, 5.0)
	),
	m_pageId(page_id),
	m_virtDisplayArea(full_size_image.xform().transformedCropArea()),
	m_distortionModel(distortion_model),
	m_depthPerception(depth_perception),
	m_dragHandler(*this),
	m_zoomHandler(*this)
{
	setMouseTracking(true);
	
	QRectF const virt_crop_rect(
		full_size_image.xform().transformedCropArea().boundingRect()
	);
	QPolygonF const source_crop_rect(virtualToImage().map(virt_crop_rect));

	XSpline top_spline(m_distortionModel.topCurve().xspline());
	XSpline bottom_spline(m_distortionModel.bottomCurve().xspline());
	if (top_spline.numControlPoints() < 2) {
		std::vector<QPointF> const& polyline = m_distortionModel.topCurve().polyline();
		
		XSpline new_top_spline;
		if (polyline.size() < 2) {
			initNewSpline(new_top_spline, source_crop_rect[0], source_crop_rect[1]);
		} else {
			initNewSpline(new_top_spline, polyline.front(), polyline.back());
			fitSpline(new_top_spline, polyline);
		}

		top_spline.swap(new_top_spline);
	}
	if (bottom_spline.numControlPoints() < 2) {
		std::vector<QPointF> const& polyline = m_distortionModel.bottomCurve().polyline();

		XSpline new_bottom_spline;
		if (polyline.size() < 2) {
			initNewSpline(new_bottom_spline, source_crop_rect[3], source_crop_rect[2]);
		} else {
			initNewSpline(new_bottom_spline, polyline.front(), polyline.back());
			fitSpline(new_bottom_spline, polyline);
		}

		bottom_spline.swap(new_bottom_spline);
	}

	m_topSpline.setSpline(top_spline, fixed_number_of_control_points);
	m_bottomSpline.setSpline(bottom_spline, fixed_number_of_control_points);
	
	InteractiveXSpline* splines[2] = { &m_topSpline, &m_bottomSpline };
	int curve_idx = -1;
	BOOST_FOREACH(InteractiveXSpline* spline, splines) {
		++curve_idx;
		spline->setModifiedCallback(boost::bind(&DewarpingView::curveModified, this, curve_idx));
		spline->setDragFinishedCallback(boost::bind(&DewarpingView::dragFinished, this));
		spline->setStorageTransform(
			boost::bind(&DewarpingView::sourceToWidget, this, _1),
			boost::bind(&DewarpingView::widgetToSource, this, _1)
		);
		makeLastFollower(*spline);
	}

	m_distortionModel.setTopCurve(dewarping::Curve(m_topSpline.spline()));
	m_distortionModel.setBottomCurve(dewarping::Curve(m_bottomSpline.spline()));

	rootInteractionHandler().makeLastFollower(*this);
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

DewarpingView::~DewarpingView()
{
}

void
DewarpingView::initNewSpline(XSpline& spline, QPointF const& p1, QPointF const& p2)
{
	QLineF const line(p1, p2);
	spline.appendControlPoint(line.p1(), 0);
	spline.appendControlPoint(line.pointAt(1.0/4.0), 1);
	spline.appendControlPoint(line.pointAt(2.0/4.0), 1);
	spline.appendControlPoint(line.pointAt(3.0/4.0), 1);
	spline.appendControlPoint(line.p2(), 0);
}

void
DewarpingView::fitSpline(XSpline& spline, std::vector<QPointF> const& polyline)
{
	using namespace spfit;

	PolylineModelShape const model_shape(polyline);
	curvatureAwareControlPointPositioning(spline, polyline);

	SplineFitter fitter(&spline);

	ConstraintSet constraints(&spline);
	constraints.constrainSplinePoint(0.0, polyline.front());
	constraints.constrainSplinePoint(1.0, polyline.back());
	fitter.setConstraints(constraints);

	FittableSpline::SamplingParams sampling_params;
	sampling_params.maxDistBetweenSamples = 10;
	fitter.setSamplingParams(sampling_params);

	int iterations_remaining = 20;
	LinearForceBalancer balancer(0.8);
	balancer.setTargetRatio(0.1);
	balancer.setIterationsToTarget(iterations_remaining - 1);

	for (; iterations_remaining > 0; --iterations_remaining, balancer.nextIteration()) {
		fitter.addAttractionForces(model_shape);
		fitter.addInternalForce(spline.controlPointsAttractionForce());

		double internal_force_weight = balancer.calcInternalForceWeight(
			fitter.internalForce(), fitter.externalForce()
		);
		OptimizationResult const res(fitter.optimize(internal_force_weight));
		if (dewarping::Curve::splineHasLoops(spline)) {
			fitter.undoLastStep();
			break;
		}

		if (res.improvementPercentage() < 0.5) {
			break;
		}
	}
}

void
DewarpingView::curvatureAwareControlPointPositioning(
	XSpline& spline, spfit::PolylineModelShape const& model_shape)
{
	using namespace spfit;

	struct Node
	{
		QPointF pt;
		double cumulativeAbsCurvature;
	};
	std::vector<Node> nodes;

	double cumulative_abs_curvature = 0;
	model_shape.uniformArcLengthSampling(100,
		[&cumulative_abs_curvature, &nodes](QPointF const& pt, double abs_curvature) {
			cumulative_abs_curvature += abs_curvature;
			nodes.push_back(Node{pt, cumulative_abs_curvature});
		}
	);

	int const num_control_points = spline.numControlPoints();
	auto it = nodes.begin();
	for (int cp_idx = 1; cp_idx < num_control_points - 1; ++cp_idx) {
		double const target_cumulative_abs_curvature =
				cumulative_abs_curvature * cp_idx / (num_control_points - 1);
		for (; it != nodes.end(); ++it) {
			if (it->cumulativeAbsCurvature > target_cumulative_abs_curvature) {
				spline.moveControlPoint(cp_idx, it->pt);
				break;
			}
		}
	}
}

void
DewarpingView::depthPerceptionChanged(double val)
{
	m_depthPerception.setValue(val);
	update();
}

void
DewarpingView::onPaint(QPainter& painter, InteractionState const& interaction)
{
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setWorldTransform(imageToVirtual() * painter.worldTransform());
	painter.setBrush(Qt::NoBrush);

	QPen grid_pen;
	grid_pen.setColor(Qt::blue);
	grid_pen.setCosmetic(true);
	grid_pen.setWidthF(1.2);

	painter.setPen(grid_pen);
	painter.setBrush(Qt::NoBrush);

	bool valid_model = m_distortionModel.isValid();

	if (valid_model) {
		try {
			unsigned const num_horizontal_curves = 30;
			unsigned const num_vertical_lines = 30;
			std::vector<std::vector<QPointF>> horizontal_curves;
			std::vector<QLineF> vertical_lines;

			Utils::buildWarpVisualization(
				m_distortionModel.topCurve().polyline(),
				m_distortionModel.bottomCurve().polyline(),
				m_depthPerception,
				num_horizontal_curves, num_vertical_lines,
				horizontal_curves, vertical_lines
			);

			for (auto const& curve : horizontal_curves) {
				painter.drawPolyline(curve.data(), curve.size());
			}

			for (auto const& line : vertical_lines) {
				painter.drawLine(line);
			}
		} catch (std::runtime_error const&) {
			// Still probably a bad model, even though DistortionModel::isValid() was true.
			valid_model = false;
		}
	} // valid_model

	if (!valid_model) {
		// Just draw the frame.
		dewarping::Curve const& top_curve = m_distortionModel.topCurve();
		dewarping::Curve const& bottom_curve = m_distortionModel.bottomCurve();
		painter.drawLine(top_curve.polyline().front(), bottom_curve.polyline().front());
		painter.drawLine(top_curve.polyline().back(), bottom_curve.polyline().back());
		painter.drawPolyline(QVector<QPointF>::fromStdVector(top_curve.polyline()));
		painter.drawPolyline(QVector<QPointF>::fromStdVector(bottom_curve.polyline()));
	}

	paintXSpline(painter, interaction, m_topSpline);
	paintXSpline(painter, interaction, m_bottomSpline);
}

void
DewarpingView::paintXSpline(
	QPainter& painter, InteractionState const& interaction,
	InteractiveXSpline const& ispline)
{
	XSpline const& spline = ispline.spline();

	painter.save();
	painter.setBrush(Qt::NoBrush);

#if 0 // No point in drawing the curve itself - we already draw the grid.
	painter.setWorldTransform(imageToVirtual() * virtualToWidget());
	
	QPen curve_pen(Qt::blue);
	curve_pen.setWidthF(1.5);
	curve_pen.setCosmetic(true);
	painter.setPen(curve_pen);
	
	std::vector<QPointF> const polyline(spline.toPolyline());
	painter.drawPolyline(&polyline[0], polyline.size());
#endif

	// Drawing cosmetic points in transformed coordinates seems unreliable,
	// so let's draw them in widget coordinates.
	painter.setWorldMatrixEnabled(false);

	QPen existing_point_pen(Qt::red);
	existing_point_pen.setWidthF(4.0);
	existing_point_pen.setCosmetic(true);
	painter.setPen(existing_point_pen);

	int const num_control_points = spline.numControlPoints();
	for (int i = 0; i < num_control_points; ++i) {
		painter.drawPoint(sourceToWidget(spline.controlPointPosition(i)));
	}

	QPointF pt;
	if (ispline.curveIsProximityLeader(interaction, &pt)) {
		QPen new_point_pen(existing_point_pen);
		new_point_pen.setColor(QColor(0x00ffff));
		painter.setPen(new_point_pen);
		painter.drawPoint(pt);
	}

	painter.restore();
}

void
DewarpingView::curveModified(int curve_idx)
{
	if (curve_idx == 0) {
		m_distortionModel.setTopCurve(dewarping::Curve(m_topSpline.spline()));
	} else {
		m_distortionModel.setBottomCurve(dewarping::Curve(m_bottomSpline.spline()));
	}
	update();
}

void
DewarpingView::dragFinished()
{
	emit distortionModelChanged(m_distortionModel);
}

/** Source image coordinates to widget coordinates. */
QPointF
DewarpingView::sourceToWidget(QPointF const& pt) const
{
	return virtualToWidget().map(imageToVirtual().map(pt));
}

/** Widget coordinates to source image coordinates. */
QPointF
DewarpingView::widgetToSource(QPointF const& pt) const
{
	return virtualToImage().map(widgetToVirtual().map(pt));
}

} // namespace deskew
