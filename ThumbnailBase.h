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

#ifndef THUMBNAILBASE_H_
#define THUMBNAILBASE_H_

#include "NonCopyable.h"
#include "PageId.h"
#include "IntrusivePtr.h"
#include "ThumbnailPixmapCache.h"
#include "ThumbnailLoadResult.h"
#include "imageproc/AbstractImageTransform.h"
#include "imageproc/AffineImageTransform.h"
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QTransform>
#include <QGraphicsItem>
#include <QSize>
#include <QSizeF>
#include <QRectF>
#include <QPolygonF>
#include <memory>

class ThumbnailBase : public QGraphicsItem
{
	DECLARE_NON_COPYABLE(ThumbnailBase)
public:
	/**
	 * @param thumbnail_cache The storage of cached thumbnails.
	 * @param max_display_size The maximum width and height of this QGraphicsItem.
	 * @param page_id Identifies the page.
	 * @param full_size_image_transform The transformation from full size original
	 *        image to a transformed space.
	 * @param transformed_viewport The rectangle in transformed coordinates
	 *        that we want to display. If unspecified,
	 *        transform.transformedCropArea().boundingRect() will be used instead.
	 */
	ThumbnailBase(
		IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
		QSizeF const& max_display_size, PageId const& page_id,
		imageproc::AbstractImageTransform const& full_size_image_transform,
		boost::optional<QRectF> const& transformed_viewport = boost::none);
	
	virtual ~ThumbnailBase();
	
	virtual QRectF boundingRect() const;
	
	virtual void paint(QPainter* painter,
		QStyleOptionGraphicsItem const* option, QWidget *widget);
protected:
	/**
	 * \brief A hook to allow subclasses to draw over the thumbnail.
	 *
	 * \param painter The painter to be used for drawing.
	 * \param transformed_to_display Can be supplied to \p painter as a world
	 *        transformation in order to draw in transformed space of
	 *        m_ptrFullSizeImageTransform.
	 * \param thumb_to_display Can be supplied to \p painter as a world
	 *        transformation in order to draw in thumbnail coordinates.
	 *        Valid thumbnail coordinates lie within this->boundingRect().
	 *
	 * The painter is configured for drawing in thumbnail coordinates by
	 * default.  No clipping is configured, but drawing should be
	 * restricted to this->boundingRect(). When this method is called on
	 * a subclass, the paint device is a temporary pixmap with thumbnail
	 * image already drawn on it, with appropriate transformations.
	 * Areas not covered by the image are transparent, which gives sublasses
	 * the opportunity to decide whether they want to draw in outside-of-image
	 * areas by utilising appropriate composition modes.
	 *
	 * @note It's not necessary for subclasses to restore the painter state.
	 */
	virtual void paintOverImage(
		QPainter& painter, QTransform const& transformed_to_display,
		QTransform const& thumb_to_display) {}

	imageproc::AbstractImageTransform const& fullSizeImageTransform() const {
		return *m_ptrFullSizeImageTransform;
	}

	/**
	 * \brief Converts from the virtual image coordinates to thumbnail image coordinates.
	 *
	 * Virtual image coordinates is the transformed space of m_ptrFullSizeImageTransform.
	 */
	QTransform const& toThumb() const { return m_postTransform; }
private:
	class LoadCompletionHandler;
	
	/**
	 * @param transform The transformation from full size original image
	 *        to a transformed space.
	 * @param transformed_viewport The rectangle in transformed coordinates
	 *        that we want to display. If unspecified,
	 *        transform.transformedCropArea().boundingRect() will be used instead.
	 */
	void setFullSizeToVirtualTransform(
		imageproc::AbstractImageTransform const& transform,
		boost::optional<QRectF> const& transformed_viewport);

	void handleLoadResult(ThumbnailLoadResult::Status status);
	
	IntrusivePtr<ThumbnailPixmapCache> m_ptrThumbnailCache;
	QSizeF m_maxDisplaySize; /**< Maximum thumbnail size on screen. */
	PageId m_pageId;
	std::unique_ptr<imageproc::AbstractImageTransform> m_ptrFullSizeImageTransform;
	QRectF m_transformedViewport;

	/** @see QGraphicsItem::boundingRect() */
	QRectF m_boundingRect;
	
	/**
	 * Transforms full size transformed image coordinates into thumbnail
	 * coordinates. Valid thumbnail coordinates lie within this->boundingRect().
	 */
	QTransform m_postTransform;
	
	boost::shared_ptr<LoadCompletionHandler> m_ptrCompletionHandler;
	bool m_extendedClipArea;
};

#endif
