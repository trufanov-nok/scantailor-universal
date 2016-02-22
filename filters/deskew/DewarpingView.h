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

#ifndef DESKEW_DEWARPING_VIEW_H_
#define DESKEW_DEWARPING_VIEW_H_

#include "ImageViewBase.h"
#include "ImagePixmapUnion.h"
#include "InteractionHandler.h"
#include "InteractiveXSpline.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "DewarpingMode.h"
#include "dewarping/DistortionModel.h"
#include "dewarping/DepthPerception.h"
#include "acceleration/AcceleratableOperations.h"
#include "Settings.h"
#include "PageId.h"
#include <QTransform>
#include <QPointF>
#include <QRectF>
#include <QPolygonF>
#include <vector>
#include <memory>

namespace imageproc
{
	class AffineTransformedImage;
}

namespace spfit
{
	class PolylineModelShape;
}

namespace deskew
{

class DewarpingView : public ImageViewBase, protected InteractionHandler
{
	Q_OBJECT
public:
	/**
	 * @note When used for perspective distortion only (no warping),
	 *       \p fixed_number_of_control_points needs to be set to true
	 *       to prevent a user from adding more control points, turing
	 *       top/bottom lines into curves. Also, \p depth_perception
	 *       doesn't matter in perspective-only mode, so a default-constructed
	 *       DepthPerception instance may be used.
	 */
	DewarpingView(
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		imageproc::AffineTransformedImage const& full_size_image,
		ImagePixmapUnion const& downscaled_image,
		PageId const& page_id,
		dewarping::DistortionModel const& distortion_model,
		dewarping::DepthPerception const& depth_perception,
		bool fixed_number_of_control_points);
	
	virtual ~DewarpingView();
signals:
	void distortionModelChanged(dewarping::DistortionModel const& model);
public slots:
	void depthPerceptionChanged(double val);	
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);
private:
	static void initNewSpline(XSpline& spline, QPointF const& p1, QPointF const& p2);

	static void fitSpline(XSpline& spline, std::vector<QPointF> const& polyline);

	static void curvatureAwareControlPointPositioning(
		XSpline& spline, spfit::PolylineModelShape const& model_shape);

	void paintXSpline(
		QPainter& painter, InteractionState const& interaction,
		InteractiveXSpline const& ispline);

	void curveModified(int curve_idx);

	void dragFinished();

	QPointF sourceToWidget(QPointF const& pt) const;

	QPointF widgetToSource(QPointF const& pt) const;

	PageId m_pageId;
	QPolygonF m_virtDisplayArea;
	dewarping::DistortionModel m_distortionModel;
	dewarping::DepthPerception m_depthPerception;
	InteractiveXSpline m_topSpline;
	InteractiveXSpline m_bottomSpline;
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;
};

} // namespace deskew

#endif
