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

#ifndef VECTOR_FIELD_IMAGE_VIEW_H_
#define VECTOR_FIELD_IMAGE_VIEW_H_

#include "ImageViewBase.h"
#include "InteractionHandler.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "Grid.h"
#include "VecNT.h"
#include "acceleration/AcceleratableOperations.h"
#include <QPoint>
#include <QRect>
#include <memory>

class VectorFieldImageView : public ImageViewBase, public InteractionHandler
{
	Q_OBJECT
public:
	VectorFieldImageView(
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		QImage const& image, Grid<Vec2f> const& directions, float max_dir_magnitude);
	
	virtual ~VectorFieldImageView();
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);

	virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction);
private:
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;
	Grid<Vec2f> m_directions;
	QRect m_imageRect;
	QPoint m_imageFocalPoint;
	float m_repDirMagnitudeRecip;
	float m_dirScale;
	QSize m_lastWidgetSize;
	double m_lastZoomLevel;
};

#endif
