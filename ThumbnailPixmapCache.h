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

#ifndef THUMBNAILPIXMAPCACHE_H_
#define THUMBNAILPIXMAPCACHE_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "ThumbnailLoadResult.h"
#include "AbstractCommand.h"
#include <boost/weak_ptr.hpp>
#include <memory>

class PageId;
class QImage;
class QPixmap;
class QString;
class QSize;

namespace imageproc
{
	class AbstractImageTransform;
	class AffineImageTransform;
}

class ThumbnailPixmapCache : public RefCountable
{
	DECLARE_NON_COPYABLE(ThumbnailPixmapCache)
public:
	typedef AbstractCommand1<void, ThumbnailLoadResult::Status> CompletionHandler;
	
	/**
	 * \brief Constructor.  To be called from the GUI thread only.
	 *
	 * \param thumb_dir The directory to store thumbnails in.  If the
	 *        provided directory doesn't exist, it will be created.
	 * \param max_size The maximum width and height for thumbnails.
	 *        The actual thumbnail size is going to depend on its aspect
	 *        ratio, but it won't exceed the provided maximum.
	 * \param max_cached_pixmaps The maximum number of pixmaps to store
	 *        in memory.
	 * \param expiration_threshold Requests are served from newest to
	 *        oldest ones. If a request is still not served after a certain
	 *        number of newer requests have been served, that request is
	 *        expired.  \p expiration_threshold specifies the exact number
	 *        of requests that cause older requests to expire.
	 *
	 * \see ThumbnailLoadResult::REQUEST_EXPIRED
	 */
	ThumbnailPixmapCache(QString const& thumb_dir, QSize const& max_size,
		int max_cached_pixmaps, int expiration_threshold);
	
	/**
	 * \brief Destructor.  To be called from the GUI thread only.
	 */
	virtual ~ThumbnailPixmapCache();
	
	void setThumbDir(QString const& thumb_dir);

	/**
	 * \brief Take the pixmap from cache or schedule a load request.
	 *
	 * If the pixmap is in cache, return it immediately.  Otherwise,
	 * schedule its loading in background.  Once the load request
	 * has been processed, the provided \p call_handler will be called.
	 *
	 * \note This function is to be called from the GUI thread only.
	 *
	 * \param page_id The identifier of the full size image and its thumbnail.
	 * \param full_size_image_transform Transformation of the original, full
	 *        size image. The result of thumbnail loading is a pixmap plus an
	 *        affine transformation of that pixmap that's equivalent of applying
	 *        \p full_size_image_transform on the original image. Both transforms
	 *        would produce a full size image, even though one of them takes a
	 *        thumbnail as its input. Note that while \p full_size_image_transform
	 *        is an arbitrary transform, the resulting pixmap transform will
	 *        always be affine. If necessary, the pixmap will be dewarped to
	 *        achieve that.
	 * \param completion_handler A weak pointer to an instance of CompletionHandler
	 *        class, that will be called to handle requrest completion.
	 *        if the thumbnail was already in cache, it will be returned
	 *        immediately, and \p completion_handler will never get called.
	 *        It's allowed to pass a null completion handler. In this case,
	 *        thumbnail loading into cache will still happen.
	 */
	ThumbnailLoadResult loadRequest(PageId const& page_id,
		imageproc::AbstractImageTransform const& full_size_image_transform,
		boost::weak_ptr<CompletionHandler> const& completion_handler);
	
	/**
	 * \brief If no thumbnail exists for this image, create it.
	 *
	 * Using this function is optional.  It just presents an optimization
	 * opportunity.  Suppose you have the full size image already loaded,
	 * and want to avoid a second load when its thumbnail is requested.
	 *
	 * \param page_id The identifier of the full size image and its thumbnail.
	 * \param full_size_image The full size, original image.
	 * \param full_size_image_transform The transformation to apply to
	 *        \p full_size_image before downscaling it to thumbnail size.
	 *
	 * \note This function may be called from any thread, even concurrently.
	 */
	void ensureThumbnailExists(
		PageId const& page_id, QImage const& full_size_image,
		imageproc::AbstractImageTransform const& full_size_image_transform);

	/**
	 * \brief Re-create the thumbnail, replacing the existing one.
	 *
	 * Similar to ensureThumbnailExists(), except this one will re-create the
	 * thumbnail even if one already exists and is up to date.
	 *
	 * \param page_id The identifier of the full size image and its thumbnail.
	 * \param full_size_image The full size, original image.
	 * \param full_size_image_transform The transformation to apply to
	 *        \p full_size_image before downscaling it to thumbnail size.
	 *
	 * \note This function may be called from any thread, even concurrently.
	 */
	void recreateThumbnail(
		PageId const& page_id, QImage const& full_size_image,
		imageproc::AbstractImageTransform const& full_size_image_transform);
private:
	struct ThumbId;
	class Item;
	class Impl;
	
	std::auto_ptr<Impl> m_ptrImpl;
};

#endif
