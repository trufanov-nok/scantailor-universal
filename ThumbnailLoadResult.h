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

#ifndef THUMBNAILLOADRESULT_H_
#define THUMBNAILLOADRESULT_H_

#include "imageproc/AffineImageTransform.h"
#include <QPixmap>
#include <boost/optional.hpp>
#include <assert.h>

class ThumbnailLoadResult
{
public:
	enum Status {
		/**
		 * \brief Load request has been queued.
		 *
		 * This status will never be returned through an asynchronous
		 * completion handler.
		 */
		QUEUED,

		/**
		 * \brief Thumbnail loaded successfully.
		 */
		LOADED,
		
		/**
		 * \brief Thumbnail failed to load.
		 */
		LOAD_FAILED,
		
		/**
		 * \brief Request has expired.
		 *
		 * Consider the following situation: we scroll our thumbnail
		 * list from the beginning all the way to the end.  This will
		 * result in every thumbnail being requested.  If we just
		 * load them in request order, that would be quite slow and
		 * inefficient.  It would be nice if we could cancel the load
		 * requests for items that went out of view.  Unfortunately,
		 * QGraphicsView doesn't provide "went out of view"
		 * notifications.  Instead, we load thumbnails starting from
		 * most recently requested, and expire requests after a certain
		 * number of newer requests are processed.  If the client is
		 * still interested in the thumbnail, it may request it again.
		 */
		REQUEST_EXPIRED
	};
	
	ThumbnailLoadResult(Status status) : m_status(status) {
		assert(status != LOADED);
	}

	ThumbnailLoadResult(Status status, QPixmap const& pixmap,
		boost::optional<imageproc::AffineImageTransform> const& pixmap_transform)
	: m_pixmap(pixmap), m_pixmapTransform(pixmap_transform), m_status(status) {}
	
	Status status() const { return m_status; }
	
	QPixmap const& pixmap() const { return m_pixmap; }

	/**
	 * @see ThumbnailPixmapCache::Item::pixmapTransform
	 * @note Guaranteed to be set if status() is LOADED.
	 */
	boost::optional<imageproc::AffineImageTransform> const& pixmapTransform() const {
		return m_pixmapTransform;
	}
private:
	QPixmap m_pixmap;
	boost::optional<imageproc::AffineImageTransform> m_pixmapTransform;
	Status m_status;
};

#endif
