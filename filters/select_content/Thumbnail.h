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

#ifndef SELECT_CONTENT_THUMBNAIL_H_
#define SELECT_CONTENT_THUMBNAIL_H_

#include "ThumbnailBase.h"
#include <QRectF>

class QSizeF;
class PageId;
class ThumbnailPixmapCache;
class ContentBox;

namespace imageproc
{
	class AbstractImageTransform;
}

namespace select_content
{

class Thumbnail : public ThumbnailBase
{
public:
	Thumbnail(IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
		QSizeF const& max_display_size, PageId const& page_id,
		imageproc::AbstractImageTransform const& full_size_image_transform,
		ContentBox const& content_box);
	
	virtual void paintOverImage(
		QPainter& painter, QTransform const& transformed_to_display,
		QTransform const& thumb_to_display);
private:
	/**
	 * Content rectangle in transformed coordinates of AbstractImageTransform
	 * that was passed to the constructor.
	 */
	QRectF m_transformedContentRect;
};

} // namespace select_content

#endif
