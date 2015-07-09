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

#include "Thumbnail.h"
#include "Utils.h"
#include "imageproc/AbstractImageTransform.h"
#include <QApplication>
#include <QPalette>
#include <QPolygonF>
#include <QTransform>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>

using namespace imageproc;

namespace page_layout
{

Thumbnail::Thumbnail(
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	QSizeF const& max_display_size, PageId const& page_id, Params const& params,
	AbstractImageTransform const& full_size_image_transform, PageLayout const& page_layout)
:	ThumbnailBase(
		thumbnail_cache, max_display_size, page_id,
		full_size_image_transform, page_layout.outerRect()
	)
,	m_params(params)
,	m_pageLayout(page_layout)
{
}

void
Thumbnail::paintOverImage(
	QPainter& painter, QTransform const& transformed_to_display,
	QTransform const& thumb_to_display)
{	
	// We work in display coordinates because we want to be
	// pixel-accurate with what we draw.
	painter.setWorldTransform(QTransform());

	painter.setRenderHint(QPainter::Antialiasing, true);

	QRectF const inner_rect(transformed_to_display.mapRect(m_pageLayout.innerRect()));
	QRectF const outer_rect(transformed_to_display.mapRect(m_pageLayout.outerRect()));

	// Fill the transparent (that is not covered by the thumbnail) portions
	// of outer_rect with "window" color. That's done because we are going to
	// draw translucent margins on top of that area, and the whole thing
	// will be eventually blitted on top of "selected page" indicator,
	// which is typically of dark color. Introducing the "window" color
	// in those areas will make margins more visible.
	painter.setCompositionMode(QPainter::CompositionMode_DestinationOver);
	painter.fillRect(outer_rect, qApp->palette().window());
	
	QPainterPath outer_outline;
	outer_outline.addPolygon(outer_rect);
	
	QPainterPath content_outline;
	content_outline.addPolygon(inner_rect);
	
	QColor const bg_color(Utils::backgroundColorForMatchSizeMode(m_params.matchSizeMode()));
	QColor const border_color(Utils::borderColorForMatchSizeMode(m_params.matchSizeMode()));

	// Draw margins.
	painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
	painter.fillPath(outer_outline.subtracted(content_outline), bg_color);
	
	// Draw the inner border of margins.
	QPen pen(border_color);
	pen.setCosmetic(true);
	pen.setWidthF(1.0);
	painter.setPen(pen);
	painter.setBrush(Qt::NoBrush);
	painter.setRenderHint(QPainter::Antialiasing, false);
	painter.drawRect(inner_rect);
}

} // namespace page_layout
