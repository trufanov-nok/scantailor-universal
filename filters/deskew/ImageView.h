/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef DESKEW_IMAGEVIEW_H_
#define DESKEW_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "ImagePixmapUnion.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "ObjectDragHandler.h"
#include "DraggablePoint.h"
#include "imageproc/AffineImageTransform.h"
#include "acceleration/AcceleratableOperations.h"
#include <QPolygonF>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QPixmap>
#include <QString>
#include <utility>
#include <memory>

class QRect;

namespace imageproc
{
	class AffineTransformedImage;
}

namespace deskew
{

class ImageView :
	public ImageViewBase,
	private InteractionHandler
{
	Q_OBJECT
public:
	/**
	 * @param accel_ops OpenCL-acceleratable operations.
	 * @param full_size_image The original, full size image plus a transformation
	 *        excluding the rotation that's done at this stage.
	 * @param downscaled_image A downscaled version of full_size_image.origImage().
	 *        @see ImageViewBase::createDownscaledImage()
	 * @param rotation_angle_deg Initial rotation angle, in degrees.
	 */
	ImageView(
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		imageproc::AffineTransformedImage const& full_size_image,
		ImagePixmapUnion const& downscaled_image,
		double rotation_angle_deg);
	
	virtual ~ImageView();
signals:
	void manualDeskewAngleSet(double degrees);
public slots:
	void manualDeskewAngleSetExternally(double degrees);
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	virtual void onWheelEvent(QWheelEvent* event, InteractionState& interaction);
private:
	void setRotationAngle(double degrees, bool preserve_scale);

	QPointF handlePosition(int idx) const;

	void handleMoveRequest(int idx, QPointF const& pos);

	virtual void dragFinished();

	QPointF getImageRotationOrigin() const;

	QRectF getRotationArcSquare() const;

	std::pair<QPointF, QPointF> getRotationHandles(QRectF const& arc_square) const;

	static int const m_cellSize;
	static double const m_maxRotationDeg;
	static double const m_maxRotationSin;

	QPixmap m_handlePixmap;
	DraggablePoint m_handles[2];
	ObjectDragHandler m_handleInteractors[2];
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;
	imageproc::AffineImageTransform m_beforeRotationTransform;
	double m_rotationAngleDeg;
};

} // namespace deskew

#endif
