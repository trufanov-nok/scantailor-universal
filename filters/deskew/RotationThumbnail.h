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

#ifndef DESKEW_ROTATION_THUMBNAIL_H_
#define DESKEW_ROTATION_THUMBNAIL_H_

#include "ThumbnailBase.h"
#include "IntrusivePtr.h"

class QSizeF;
class ThumbnailPixmapCache;
class PageId;

namespace imageproc
{
	class AffineTransformedImage;
}

namespace deskew
{

class RotationThumbnail : public ThumbnailBase
{
public:
	RotationThumbnail(IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
		QSizeF const& max_display_size, PageId const& page_id,
		imageproc::AffineImageTransform const& full_size_image_transform,
		double compensation_angle_deg, bool draw_grid);
	
	virtual void paintOverImage(
		QPainter& painter, QTransform const& transformed_to_display,
		QTransform const& thumb_to_display);
private:
	bool m_drawGrid;
};

} // namespace deskew

#endif
