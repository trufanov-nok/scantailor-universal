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

#include "VectorFieldImageView.h"
#include "ImagePresentation.h"
#include "ImagePixmapUnion.h"
#include "InteractionState.h"
#include "VecNT.h"
#include <QPointF>
#include <QRectF>
#include <QTransform>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <stdexcept>
#include <algorithm>
#include <stddef.h>

VectorFieldImageView::VectorFieldImageView(
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& image, Grid<Vec2f> const& directions, float representative_dir_magnitude)
:	ImageViewBase(
		accel_ops, image, ImagePixmapUnion(),
		ImagePresentation(QTransform(), QRectF(image.rect()))
	)
,	m_dragHandler(*this)
,	m_zoomHandler(*this)
,	m_directions(directions)
,	m_imageRect(image.rect())
,	m_repDirMagnitudeRecip(1.0f / std::max<float>(representative_dir_magnitude, 1e-5f))
,	m_dirScale(1.f)
,	m_lastWidgetSize(0, 0)
,	m_lastZoomLevel(0.0)
{
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
	rootInteractionHandler().makeLastFollower(*this);

	setMouseTracking(true);
}

VectorFieldImageView::~VectorFieldImageView()
{
}

void
VectorFieldImageView::onPaint(QPainter& painter, InteractionState const& interaction)
{
	if (interaction.captured()) {
		return;
	}

	if (!m_imageRect.contains(m_imageFocalPoint)) {
		return;
	}

	if (size() != m_lastWidgetSize || zoomLevel() != m_lastZoomLevel) {
		// Update m_dirScale.
		QTransform const w2i(widgetToImage());
		float const scale_upper_bound = 0.5f * float(std::min(viewport()->width(), viewport()->height()))
			* std::min(w2i.m11(), w2i.m22()) * m_repDirMagnitudeRecip;
		float const scale_lower_bound = 30.f * std::max(w2i.m11(), w2i.m22()) * m_repDirMagnitudeRecip;
		m_dirScale = qBound(scale_lower_bound, m_dirScale, scale_upper_bound);
		m_lastWidgetSize = size();
		m_lastZoomLevel = zoomLevel();
	}

	painter.setRenderHint(QPainter::Antialiasing);

	// Setup for drawing in image coordinates.
	painter.setWorldTransform(imageToWidget());

	QPen pen(QColor(0, 0, 255));
	pen.setCosmetic(true);
	pen.setWidthF(2.5);
	painter.setPen(pen);

	Vec2f vec(m_directions(m_imageFocalPoint.x(), m_imageFocalPoint.y()));
	vec *= m_dirScale;
	QPointF const arrow_tip(m_imageFocalPoint + vec);
	painter.drawLine(m_imageFocalPoint, arrow_tip);
	
	// Now we will be drawing  the > part of the arrow.

	QTransform rot45;
	rot45.rotate(45);
	
	QLineF arrow_line1(arrow_tip, arrow_tip - rot45.map(vec));
	rot45.rotate(-90); // Rotate the other way.
	QLineF arrow_line2(arrow_tip, arrow_tip - rot45.map(vec));

	// Draw in widget coordinates;
	painter.setWorldTransform(QTransform());
	
	arrow_line1 = imageToWidget().map(arrow_line1);
	arrow_line2 = imageToWidget().map(arrow_line2);
	arrow_line1.setLength(6.0);
	arrow_line2.setLength(6.0);

	painter.drawLine(arrow_line1);
	painter.drawLine(arrow_line2);

	// Finlly, print the scale of our direction vectors.
	QString const text(tr("Scale: %1").arg(m_dirScale, 0, 'g', 3));
	painter.drawText(viewport()->rect(), Qt::AlignBottom|Qt::AlignRight, text);
}

void
VectorFieldImageView::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
	m_imageFocalPoint = widgetToImage().map(event->pos());
	update();
}
