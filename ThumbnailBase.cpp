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

#include "ThumbnailBase.h"
#include "ThumbnailPixmapCache.h"
#include "ThumbnailLoadResult.h"
#include "NonCopyable.h"
#include "AbstractCommand.h"
#include "PixmapRenderer.h"
#include "imageproc/PolygonUtils.h"
#include <boost/weak_ptr.hpp>
#include <QPixmap>
#include <QPixmapCache>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>
#include <QPalette>
#include <QStyle>
#include <QApplication>
#include <QPointF>
#include <Qt>
#include <QDebug>
#include <math.h>

using namespace imageproc;

class ThumbnailBase::LoadCompletionHandler :
	public AbstractCommand1<void, ThumbnailLoadResult::Status>
{
	DECLARE_NON_COPYABLE(LoadCompletionHandler)
public:
	LoadCompletionHandler(ThumbnailBase* thumb) : m_pThumb(thumb) {}
	
	virtual void operator()(ThumbnailLoadResult::Status status) {
		m_pThumb->handleLoadResult(status);
	}
private:
	ThumbnailBase* m_pThumb;
};


ThumbnailBase::ThumbnailBase(
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	QSizeF const& max_display_size, PageId const& page_id,
	AbstractImageTransform const& full_size_image_transform,
	boost::optional<QRectF> const& transformed_viewport)
:	m_ptrThumbnailCache(thumbnail_cache),
	m_maxDisplaySize(max_display_size),
	m_pageId(page_id),
	m_extendedClipArea(false)
{
	setFullSizeToVirtualTransform(full_size_image_transform, transformed_viewport);
}

ThumbnailBase::~ThumbnailBase()
{
}

QRectF
ThumbnailBase::boundingRect() const
{
	return m_boundingRect;
}

void
ThumbnailBase::paint(QPainter* painter,
	QStyleOptionGraphicsItem const* option, QWidget *widget)
{
	ThumbnailLoadResult res(ThumbnailLoadResult::QUEUED);

	if (!m_ptrCompletionHandler.get()) {
		boost::shared_ptr<LoadCompletionHandler> handler(
			new LoadCompletionHandler(this)
		);
		res = m_ptrThumbnailCache->loadRequest(
			m_pageId, *m_ptrFullSizeImageTransform, handler
		);
		if (res.status() == ThumbnailLoadResult::QUEUED) {
			m_ptrCompletionHandler.swap(handler);
		}
	}
	
	QTransform const full_size_transformed_to_display(
		m_postTransform * painter->worldTransform()
	);
	QTransform const thumb_to_display(painter->worldTransform());
	
	if (res.status() != ThumbnailLoadResult::LOADED) {
		double const border = 1.0;
		double const shadow = 2.0;
		QRectF rect(m_boundingRect);
		rect.adjust(border, border, -(border + shadow), -(border + shadow));
		
		painter->fillRect(m_boundingRect, QColor(0x00, 0x00, 0x00));
		painter->fillRect(rect, QColor(0xff, 0xff, 0xff));
		
		paintOverImage(*painter, full_size_transformed_to_display, thumb_to_display);
		return;
	}

	QTransform const pixmap_to_thumb(
		res.pixmapTransform()->transform() * m_postTransform
	);

	QRectF const display_rect(
		full_size_transformed_to_display.mapRect(m_transformedViewport)
	);

	QPixmap temp_pixmap;
	QString const cache_key(QString::fromLatin1("ThumbnailBase::temp_pixmap"));
	if (QPixmapCache::find(cache_key, temp_pixmap)
			&& temp_pixmap.width() >= display_rect.width()
			&& temp_pixmap.height() >= display_rect.width()) {
#if 0
		// Since below we are drawing with QPainter::CompositionMode_Source,
		// which overwrites destination pixels rather than blending with them,
		// we can skip clearing the pixmap.
		temp_pixmap.fill(Qt::transparent);
#endif
	} else {
		int w = (int)display_rect.width();
		int h = (int)display_rect.height();
		
		// Add some extra, to avoid rectreating the pixmap too often.
		w += w / 10;
		h += h / 10;
		
		temp_pixmap = QPixmap(w, h);
		
		if (!temp_pixmap.hasAlphaChannel()) {
			// This actually forces the alpha channel to be created.
			temp_pixmap.fill(Qt::transparent);
		}
		
		QPixmapCache::insert(cache_key, temp_pixmap);
	}

	QPainter temp_painter;
	temp_painter.begin(&temp_pixmap);
	temp_painter.save();

	QTransform temp_adjustment;
	temp_adjustment.translate(-display_rect.left(), -display_rect.top());

	// Setup for drawing in res.pixmap() coordinates.
	temp_painter.setWorldTransform(pixmap_to_thumb * thumb_to_display * temp_adjustment);

	// Draw the thumbnail.
	temp_painter.setCompositionMode(QPainter::CompositionMode_Source);
	temp_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
	temp_painter.setRenderHint(QPainter::Antialiasing, true);
	temp_painter.drawPixmap(0, 0, res.pixmap());

	// We could have used setClipPath() before drawing the pixmap above,
	// however QPainter::Antialiasing flag is getting ignored in that case
	// for some reason, even though it works fine in ImageViewBase.
	// Instead we are going to clear the areas in temp_pixmap outside of
	// the crop area.

	// Setup for drawing in display coordinates.
	temp_painter.setWorldTransform(temp_adjustment);

	QPainterPath outer_path;
	outer_path.addRect(display_rect.adjusted(-1, -1, 1, 1));

	QPainterPath inner_path;
	inner_path.addPolygon(
		full_size_transformed_to_display.map(
			m_ptrFullSizeImageTransform->transformedCropArea()
		)
	);

	// Clear the area outside of the thumbnail.
	temp_painter.fillPath(outer_path.subtracted(inner_path), Qt::transparent);

	temp_painter.restore();

	// Setup the painter for drawing in thumbnail coordinates,
	// as required for paintOverImage().
	temp_painter.setWorldTransform(thumb_to_display * temp_adjustment);

	paintOverImage(
		temp_painter, full_size_transformed_to_display * temp_adjustment,
		thumb_to_display * temp_adjustment
	);

	temp_painter.end();

	painter->setWorldTransform(QTransform());
	painter->setClipRect(display_rect);
	painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
	painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter->drawPixmap(display_rect.topLeft(), temp_pixmap);
}

void
ThumbnailBase::setFullSizeToVirtualTransform(
	AbstractImageTransform const& transform,
	boost::optional<QRectF> const& transformed_viewport)
{
	m_ptrFullSizeImageTransform = transform.clone();

	if (transformed_viewport) {
		m_transformedViewport = *transformed_viewport;
	} else {
		m_transformedViewport = transform.transformedCropArea().boundingRect();
	}

	QSizeF const unscaled_size(m_transformedViewport.size().expandedTo(QSize(1, 1)));
	QSizeF scaled_size(unscaled_size);
	scaled_size.scale(m_maxDisplaySize, Qt::KeepAspectRatio);
	
	m_boundingRect = QRectF(QPointF(0.0, 0.0), scaled_size);

	double const x_post_scale = scaled_size.width() / unscaled_size.width();
	double const y_post_scale = scaled_size.height() / unscaled_size.height();

	m_postTransform.reset();
	m_postTransform.scale(x_post_scale, y_post_scale);
	m_postTransform.translate(-m_transformedViewport.x(), -m_transformedViewport.y());
}

void
ThumbnailBase::handleLoadResult(ThumbnailLoadResult::Status status)
{
	m_ptrCompletionHandler.reset();

	if (status != ThumbnailLoadResult::LOAD_FAILED) {
		update();
	}
}
