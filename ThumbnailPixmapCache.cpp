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

#include "ThumbnailPixmapCache.h"
#include "ImageId.h"
#include "PageId.h"
#include "ImageLoader.h"
#include "AtomicFileOverwriter.h"
#include "RelinkablePath.h"
#include "OutOfMemoryHandler.h"
#include "imageproc/AffineImageTransform.h"
#include "imageproc/AffineTransformedImage.h"
#include "imageproc/Scale.h"
#include "imageproc/GrayImage.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QString>
#include <QChar>
#include <QImage>
#include <QImageReader>
#include <QPixmap>
#include <QEvent>
#include <QSize>
#include <QDebug>
#include <Qt>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <algorithm>
#include <memory>
#include <vector>
#include <new>
#include <cstdint>

using namespace ::boost;
using namespace ::boost::multi_index;
using namespace imageproc;

/** For QImageReader::text() and QImage::setText() */
static char const TRANSFORM_FINGERPRINT_KEY[] = "transform-fingerprint";

static std::shared_ptr<AbstractImageTransform const>
normalizedFullSizeImageTransform(AbstractImageTransform const& full_size_image_transform);

/**
 * This class captures the fact that we store different thumbnails for
 * different sub-pages only for dewarped thumbnails.
 */
struct ThumbnailPixmapCache::ThumbId
{
	/**
	 * We can't delete a loaded thumbnail from a non-GUI thread and yet
	 * we may want to replace it immediately with an updated version
	 * (e.g. its transformation was updated). In such a case we give the
	 * thumbnail a new identity by setting retirementIdentity to a unique
	 * nonzero value while posting a removal request to the GUI thread.
	 */
	uint64_t retirementIdentity = 0;

	PageId pageId;
	bool isAffineTransform;

	ThumbId() : isAffineTransform(false) {}

	ThumbId(PageId const& page_id, bool is_affine_transform)
	: pageId(page_id), isAffineTransform(is_affine_transform) {}

	bool operator<(ThumbId const& rhs) const {
		if (int comp = pageId.imageId().filePath().compare(rhs.pageId.imageId().filePath())) {
			return comp < 0;
		} else if (pageId.imageId().page() != rhs.pageId.imageId().page()) {
			return pageId.imageId().page() < rhs.pageId.imageId().page();
		} else if (isAffineTransform != rhs.isAffineTransform) {
			return isAffineTransform;
		} else if (!isAffineTransform && pageId.subPage() != rhs.pageId.subPage()) {
			// Sub-page only matters for dewarped thumbnails.
			return pageId.subPage() < rhs.pageId.subPage();
		} else if (retirementIdentity != rhs.retirementIdentity) {
			return retirementIdentity < rhs.retirementIdentity;
		} else {
			return false;
		}
	}
};

class ThumbnailPixmapCache::Item
{
public:
	enum Status {
		/**
		 * The background threaed hasn't touched it yet.
		 */
		QUEUED,
		
		/**
		 * The image is currently being loaded by a background
		 * thread, or it has been loaded, but the main thread
		 * hasn't yet received the loaded image, or it's currently
		 * converting it to a pixmap.
		 */
		IN_PROGRESS,
		
		/**
		 * The image was loaded and then converted to a pixmap
		 * by the main thread.
		 */
		LOADED,
		
		/**
		 * The image could not be loaded.
		 */
		LOAD_FAILED
	};
	
	/**
	 * @brief PageId + isAffineTransform flag.
	 *
	 * This one serves as a key in boost::multi_index, and therefore
	 * is not mutable.
	 */
	ThumbId thumbId;

	/**
	 * @brief Strong pointer to an ordinary pointer to this object.
	 *
	 * Only set when \p status == IN_PROGRESS. Pending operations will be
	 * holding a weak pointer to it. Resetting this member is a way to ensure
	 * this Item is locked away from any operations that are still pending.
	 */
	mutable std::shared_ptr<Item const*> thisPtr;

	/**
	 * @brief Transformation applied to the original, full size image
	 *        before downscaling to thumbnail size.
	 *
	 * If the requested transform was affine, this member is forced to an
	 * identity transform, as we want to use the same thumbnail image for any
	 * affine transforms the client may request. The crop area inside
	 * AffineImageTransform doesn't matter in this case.
	 */
	mutable std::shared_ptr<AbstractImageTransform const> fullSizeImageTransform;
	
	/**
	 * Guaranteed to be non-null if status is LOADED. May still be non-null
	 * otherwise, as we can't destroy a QPixmap from a non-GUI thread.
	 */
	mutable QPixmap pixmap;

	/**
	 * @brief Transformation applied to \p pixmap, equivalent to applying
	 *        \p fullSizeImageTransform on the original, full size image.
	 *
	 * In particular, pixmapTransform->transformedCropArea() and
	 * fullSizeImageTransform->transformedCropArea() shall be equal up to
	 * floating point accuracy.
	 *
	 * @note Guaranteed to be set if status is LOADED and clear otherwise.
	 */
	mutable boost::optional<AffineImageTransform> pixmapTransform;
	
	mutable std::vector<boost::weak_ptr<CompletionHandler>> completionHandlers;
	
	/**
	 * The total image loading attempts (of any images) by
	 * ThumbnailPixmapCache at the time of the creation of this item.
	 * This information is used for request expiration.
	 * \see ThumbnailLoadResult::REQUEST_EXPIRED
	 */
	int precedingLoadAttempts;
	
	mutable Status status;

	Item(ThumbId const& thumb_id,
		AbstractImageTransform const& full_size_image_transform,
		int preceding_load_attepmts, Status status);
private:
	Item& operator=(Item const& other); // Assignment is forbidden.
};


class ThumbnailPixmapCache::Impl : public QThread
{
public:
	Impl(QString const& thumb_dir,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		QSize const& max_thumb_size, int max_cached_pixmaps, int expiration_threshold);
	
	~Impl();

	void setThumbDir(QString const& thumb_dir);

	void setAccelOps(std::shared_ptr<AcceleratableOperations> const& accel_ops);
	
	ThumbnailLoadResult request(ThumbId const& thumb_id,
		AbstractImageTransform const& full_size_image_transform,
		boost::weak_ptr<CompletionHandler> const& completion_handler);
	
	void ensureThumbnailExists(
		ThumbId const& thumb_id, QImage const& full_size_image,
		AbstractImageTransform const& full_size_image_transform,
		bool force_recreate);
	
	void recreateThumbnail(PageId const& page_id, QImage const& image);
protected:
	virtual void run();
	
	virtual void customEvent(QEvent* e);
private:
	class LoadResultEvent;
	class ItemsByKeyTag;
	class LoadQueueTag;
	class RemoveQueueTag;
	
	typedef multi_index_container<
		Item,
		indexed_by<
			ordered_unique<tag<ItemsByKeyTag>, member<Item, ThumbId, &Item::thumbId>>,
			sequenced<tag<LoadQueueTag> >,
			sequenced<tag<RemoveQueueTag> >
		>
	> Container;
	
	typedef Container::index<ItemsByKeyTag>::type ItemsByKey;
	typedef Container::index<LoadQueueTag>::type LoadQueue;
	typedef Container::index<RemoveQueueTag>::type RemoveQueue;
	
	class BackgroundLoader : public QObject
	{
	public:
		BackgroundLoader(Impl& owner);
	protected:
		virtual void customEvent(QEvent* e);
	private:
		Impl& m_rOwner;
	};
	
	void backgroundProcessing();
	
	static QString getThumbFilePath(
		ThumbId const& thumb_id, QString const& thumb_dir);

	static std::unique_ptr<QImageReader> validateThumbnailOnDisk(
		QString const& thumb_file_path,
		AbstractImageTransform const& full_size_image_transform);

	static boost::optional<AffineTransformedImage>
	loadSaveThumbnail(ThumbId const& thumb_id,
		AbstractImageTransform const& full_size_image_transform,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		QString const& thumb_dir, QSize const& max_thumb_size);
	
	static void saveThumbnail(QImage const& thumb, QString const& thumb_file_path);
	
	static boost::optional<AffineTransformedImage> makeThumbnail(
		QSize const& max_thumb_size, QImage const& full_size_image,
		AbstractImageTransform const& full_size_image_transform,
		std::shared_ptr<AcceleratableOperations> const& accel_ops);

	static QImage downscaleImage(
		QImage const& image, QSize const& max_thumb_size);
	
	void postRequestExpiredResult(std::weak_ptr<Item const*> const& item);

	void postLoadFailedResult(std::weak_ptr<Item const*> const& item);

	void postLoadedResult(std::weak_ptr<Item const*> const& item,
		AffineTransformedImage const& thumb);

	void processLoadResult(LoadResultEvent* result);
	
	void removeExcessGuiThreadLocked();
	
	void removeItemGuiThreadLocked(Item const& item);

	void transitionToInProgressStatusLocked(Item const& item);

	void transitionToLoadedStatusGuiThreadLocked(
		Item const& item, QPixmap const& pixmap,
		AffineImageTransform const& pixmap_transform);

	void transitionToLoadFailedStatusLocked(Item const& item);

	/** Don't call directly. To be called by transitionTo*() functions. */
	void transitionFromCurrentStatusLocked(Item const& item);

	/** Don't call directly. To be called by transitionFromCurrentStatusLocked(). */
	void prepareTransitionFromQueuedStatusLocked(Item const& item);

	/** Don't call directly. To be called by transitionFromCurrentStatusLocked(). */
	void prepareTransitionFromInProgressStatusLocked(Item const& item);

	/** Don't call directly. To be called by transitionFromCurrentStatusLocked(). */
	void prepareTransitionFromLoadedStatusLocked(Item const& item);

	mutable QMutex m_mutex;

	/**
	 * @note Accessing this smart pointer does require m_mutex protection while
	 *       using the pointee doesn't. In other words, you can make a copy while
	 *       being protected by m_mutex and then use the copy without the mutex.
	 */
	std::shared_ptr<AcceleratableOperations> m_ptrAccelOps;

	BackgroundLoader m_backgroundLoader;
	Container m_items;
	ItemsByKey& m_itemsByKey; /**< ThumbId => Item mapping */
	
	/**
	 * An "std::list"-like view of QUEUED items in the order they are
	 * going to be loaded.  Actually the list contains all kinds of items,
	 * but all QUEUED ones precede any others.  New QUEUED items are added
	 * to the front of this list for purposes of request expiration.
	 * \see ThumbnailLoadResult::REQUEST_EXPIRED
	 */
	LoadQueue& m_loadQueue;
	
	/**
	 * An "std::list"-like view of LOADED items in the order they are
	 * going to be removed. Actually the list contains all kinds of items,
	 * but all LOADED ones precede any others.  Note that we don't bother
	 * removing items without a pixmap, which would be all except LOADED
	 * items.  New LOADED items are added after the last LOADED item
	 * already present in the list.
	 */
	RemoveQueue& m_removeQueue;
	
	/**
	 * An iterator of m_removeQueue that marks the end of LOADED items.
	 */
	RemoveQueue::iterator m_endOfLoadedItems;

	/** @see Item::retirementIdentity */
	uint64_t m_nextRetirementIdentity = 1;
	
	QString m_thumbDir;
	QSize m_maxThumbSize;
	int m_maxCachedPixmaps;
	
	/**
	 * \see ThumbnailPixmapCache::ThumbnailPixmapCache()
	 */
	int m_expirationThreshold;
	
	int m_numQueuedItems;
	int m_numLoadedItems;
	
	/**
	 * Total image loading attempts so far.  Used for request expiration.
	 * \see ThumbnailLoadResult::REQUEST_EXPIRED
	 */
	int m_totalLoadAttempts;
	
	bool m_threadStarted;
	bool m_shuttingDown;
};


class ThumbnailPixmapCache::Impl::LoadResultEvent : public QEvent
{
public:
	std::weak_ptr<Item const*> item;
	boost::optional<AffineTransformedImage> thumb;
	ThumbnailLoadResult::Status status;

	LoadResultEvent(
		ThumbnailLoadResult::Status status, std::weak_ptr<Item const*> const& item);

	LoadResultEvent(
		ThumbnailLoadResult::Status status,
		std::weak_ptr<Item const*> const& item,
		AffineTransformedImage const& thumb);
	
	virtual ~LoadResultEvent();
};


/*========================== ThumbnailPixmapCache ===========================*/

ThumbnailPixmapCache::ThumbnailPixmapCache(
	QString const& thumb_dir,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QSize const& max_thumb_size,
	int const max_cached_pixmaps, int const expiration_threshold)
:	m_ptrImpl(
		new Impl(
			RelinkablePath::normalize(thumb_dir), accel_ops,
			max_thumb_size, max_cached_pixmaps, expiration_threshold
		)
	)
{
}

ThumbnailPixmapCache::~ThumbnailPixmapCache()
{
}

void
ThumbnailPixmapCache::setThumbDir(QString const& thumb_dir)
{
	m_ptrImpl->setThumbDir(RelinkablePath::normalize(thumb_dir));
}

void
ThumbnailPixmapCache::setAccelOps(std::shared_ptr<AcceleratableOperations> const& accel_ops)
{
	m_ptrImpl->setAccelOps(accel_ops);
}

ThumbnailLoadResult
ThumbnailPixmapCache::loadRequest(PageId const& page_id,
	AbstractImageTransform const& full_size_image_transform,
	boost::weak_ptr<CompletionHandler> const& completion_handler)
{
	return m_ptrImpl->request(
		ThumbId(page_id, full_size_image_transform.isAffine()),
		full_size_image_transform, completion_handler
	);
}

void
ThumbnailPixmapCache::ensureThumbnailExists(
	PageId const& page_id, QImage const& full_size_image,
	AbstractImageTransform const& full_size_image_transform)
{
	m_ptrImpl->ensureThumbnailExists(
		ThumbId(page_id, full_size_image_transform.isAffine()),
		full_size_image, full_size_image_transform,
		/*force_recreate=*/false
	);
}

void
ThumbnailPixmapCache::recreateThumbnail(
	PageId const& page_id, QImage const& full_size_image,
	AbstractImageTransform const& full_size_image_transform)
{
	m_ptrImpl->ensureThumbnailExists(
		ThumbId(page_id, full_size_image_transform.isAffine()),
		full_size_image, full_size_image_transform,
		/*force_recreate=*/true
	);
}

/*======================= ThumbnailPixmapCache::Impl ========================*/

ThumbnailPixmapCache::Impl::Impl(
	QString const& thumb_dir,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QSize const& max_thumb_size,
	int const max_cached_pixmaps, int const expiration_threshold)
:	m_ptrAccelOps(accel_ops),
	m_backgroundLoader(*this),
	m_items(),
	m_itemsByKey(m_items.get<ItemsByKeyTag>()),
	m_loadQueue(m_items.get<LoadQueueTag>()),
	m_removeQueue(m_items.get<RemoveQueueTag>()),
	m_endOfLoadedItems(m_removeQueue.end()),
	m_thumbDir(thumb_dir),
	m_maxThumbSize(max_thumb_size),
	m_maxCachedPixmaps(max_cached_pixmaps),
	m_expirationThreshold(expiration_threshold),
	m_numQueuedItems(0),
	m_numLoadedItems(0),
	m_totalLoadAttempts(0),
	m_threadStarted(false),
	m_shuttingDown(false)
{
	// Note that QDir::mkdir() will fail if the parent directory,
	// that is $OUT/cache doesn't exist. We want that behaviour,
	// as otherwise when loading a project from a different machine,
	// a whole bunch of bogus directories would be created.
	QDir().mkdir(m_thumbDir);

	m_backgroundLoader.moveToThread(this);
}

ThumbnailPixmapCache::Impl::~Impl()
{
	{
		QMutexLocker const locker(&m_mutex);
		
		if (!m_threadStarted) {
			return;
		}
		
		m_shuttingDown = true;
	}
	
	quit();
	wait();
}

void
ThumbnailPixmapCache::Impl::setThumbDir(QString const& thumb_dir)
{
	QMutexLocker locker(&m_mutex);

	if (thumb_dir == m_thumbDir) {
		return;
	}

	m_thumbDir = thumb_dir;

	BOOST_FOREACH(Item const& item, m_loadQueue) {
		// This trick will make all queued tasks to expire.
		m_totalLoadAttempts = std::max(
			m_totalLoadAttempts,
			item.precedingLoadAttempts + m_expirationThreshold + 1
		);
	}
}

void
ThumbnailPixmapCache::Impl::setAccelOps(std::shared_ptr<AcceleratableOperations> const& accel_ops)
{
	QMutexLocker locker(&m_mutex);
	m_ptrAccelOps = accel_ops;
}

ThumbnailLoadResult
ThumbnailPixmapCache::Impl::request(ThumbId const& thumb_id,
	AbstractImageTransform const& full_size_image_transform,
	boost::weak_ptr<CompletionHandler> const& completion_handler)
{
	assert(QCoreApplication::instance()->thread() == QThread::currentThread());
	
	QMutexLocker locker(&m_mutex);
	
	if (m_shuttingDown) {
		return ThumbnailLoadResult::LOAD_FAILED;
	}
	
	ItemsByKey::iterator const k_it(m_itemsByKey.find(thumb_id));

	do { // while (false) -> just to be able to break from it.

		if (k_it == m_itemsByKey.end()) {
			break;
		}

		Item const& item = *k_it;

		if (!item.thumbId.isAffineTransform &&
				item.fullSizeImageTransform->fingerprint() !=
				full_size_image_transform.fingerprint()) {

			// Start the retirement process of the old version
			// followed by creation of a new one.
			m_itemsByKey.modify(k_it, [this](Item& item) {
				item.thumbId.retirementIdentity = m_nextRetirementIdentity++;
			});

			// This will result in removing the whole item.
			postRequestExpiredResult(item.thisPtr);
			break;
		}

		if (item.status == Item::LOADED) {
			assert(!item.pixmap.isNull());
			assert(item.pixmapTransform);

			// Move it after all other candidates for removal.
			RemoveQueue::iterator const rq_it(
				m_items.project<RemoveQueueTag>(k_it)
			);
			m_removeQueue.relocate(m_endOfLoadedItems, rq_it);

			if (!item.thumbId.isAffineTransform) {
				return ThumbnailLoadResult(
					ThumbnailLoadResult::LOADED, item.pixmap,
					item.pixmapTransform
				);
			} else {
				/* In affine case, some extra work is required to compensate
				 * for forcing Item::fullSizeImageTransform to an identity one. */
				AffineImageTransform pixmap_transform(
					full_size_image_transform.toAffine()
				);
				pixmap_transform.setOrigCropArea(
					item.pixmapTransform->transform().inverted().map(
						pixmap_transform.origCropArea()
					)
				);
				pixmap_transform.setTransform(
					item.pixmapTransform->transform() * pixmap_transform.transform()
				);
				return ThumbnailLoadResult(
					ThumbnailLoadResult::LOADED, item.pixmap,
					pixmap_transform
				);
			}
		} else if (item.status == Item::LOAD_FAILED) {
			return ThumbnailLoadResult::LOAD_FAILED;
		}

		assert(item.status == Item::QUEUED || item.status == Item::IN_PROGRESS);

		if (!completion_handler.expired()) {
			item.completionHandlers.push_back(completion_handler);
		}
		
		if (item.status == Item::QUEUED) {
			// Because we've got a new request for this item,
			// we move it to the beginning of the load queue.
			// Note that we don't do it for IN_PROGRESS items,
			// because all QUEUED items must precede any other
			// items in the load queue.
			LoadQueue::iterator const lq_it(
				m_items.project<LoadQueueTag>(k_it)
			);
			m_loadQueue.relocate(m_loadQueue.begin(), lq_it);
		}
		
		return ThumbnailLoadResult::QUEUED;

	} while (false);
	
	// Create a new item.
	LoadQueue::iterator const lq_it(
		m_loadQueue.push_front(
			Item(
				thumb_id, full_size_image_transform,
				m_totalLoadAttempts, Item::QUEUED
			)
		).first
	);
	// Now our new item is at the beginning of the load queue and at the
	// end of the remove queue.
	
	assert(lq_it->status == Item::QUEUED);
	assert(lq_it->completionHandlers.empty());
	
	if (m_endOfLoadedItems == m_removeQueue.end()) {
		m_endOfLoadedItems = m_items.project<RemoveQueueTag>(lq_it);
	}
	if (!completion_handler.expired())
	{
		lq_it->completionHandlers.push_back(completion_handler);
	}
	
	if (m_numQueuedItems++ == 0) {
		if (m_threadStarted) {
			// Wake the background thread up.
			QCoreApplication::postEvent(
				&m_backgroundLoader, new QEvent(QEvent::User)
			);
		} else {
			// Start the background thread.
			start();
			m_threadStarted = true;
		}
	}
	
	return ThumbnailLoadResult::QUEUED;
}

void
ThumbnailPixmapCache::Impl::ensureThumbnailExists(
	ThumbId const& thumb_id, QImage const& full_size_image,
	AbstractImageTransform const& full_size_image_transform,
	bool force_recreate)
{
	if (full_size_image.isNull()) {
		return;
	}
	
	QMutexLocker locker(&m_mutex);

	if (m_shuttingDown) {
		return;
	}

	QString const thumb_dir(m_thumbDir);
	QSize const max_thumb_size(m_maxThumbSize);
	std::shared_ptr<AcceleratableOperations> const accel_ops(m_ptrAccelOps);

	locker.unlock();
	
	QString const thumb_file_path(getThumbFilePath(thumb_id, thumb_dir));

	if (!force_recreate) {
		if (validateThumbnailOnDisk(thumb_file_path, full_size_image_transform)) {
			return;
		}
	}

	boost::optional<AffineTransformedImage> const thumb(
		makeThumbnail(max_thumb_size, full_size_image, full_size_image_transform, accel_ops)
	);
	if (!thumb) {
		return;
	}

	saveThumbnail(thumb->origImage(), thumb_file_path);

	locker.relock();

	// If the corresponding Item already exists, we need to update it.
	ItemsByKey::iterator const k_it(m_itemsByKey.find(thumb_id));
	if (k_it == m_itemsByKey.end()) {
		return;
	}

	Item const& item = *k_it;

	// This may not be a GUI thread, so instead of converting
	// QImage to QPixmap here, we mark this item IN_PROGRESS
	// and send a LoadResultEvent.
	transitionToInProgressStatusLocked(item);

	item.fullSizeImageTransform = normalizedFullSizeImageTransform(
		full_size_image_transform
	);

	postLoadedResult(item.thisPtr, *thumb);
}

void
ThumbnailPixmapCache::Impl::run()
{
	backgroundProcessing();
	exec(); // Wait for further processing requests (via custom events).
}

void
ThumbnailPixmapCache::Impl::customEvent(QEvent* e)
{
	processLoadResult(dynamic_cast<LoadResultEvent*>(e));
}

void
ThumbnailPixmapCache::Impl::backgroundProcessing()
{
	// This method is called from a background thread.
	assert(QCoreApplication::instance()->thread() != QThread::currentThread());
	
	for (;;) {
		try {
			// We are going to initialize these while holding the mutex.
			std::weak_ptr<Item const*> item_weak_ptr;
			ThumbId thumb_id;
			std::shared_ptr<AbstractImageTransform const> full_size_image_transform;
			QString thumb_dir;
			QSize max_thumb_size;
			std::shared_ptr<AcceleratableOperations> accel_ops;

			{
				QMutexLocker const locker(&m_mutex);

				if (m_shuttingDown || m_items.empty()) {
					break;
				}

				Item const& item = m_loadQueue.front();
				thumb_id = item.thumbId;
				full_size_image_transform = item.fullSizeImageTransform;

				if (item.status != Item::QUEUED) {
					// All QUEUED items precede any other items
					// in the load queue, so it means there are no
					// QUEUED items at all.
					assert(m_numQueuedItems == 0);
					break;
				}

				// By marking the item as IN_PROGRESS, we prevent it
				// from being processed again before the GUI thread
				// receives our LoadResultEvent.
				transitionToInProgressStatusLocked(item);

				// This has to be done after transitionToInProgressStatusLocked(),
				// as Item::thisPtr is only meant to be set in IN_PROGRESS state.
				item_weak_ptr = item.thisPtr;

				if (m_totalLoadAttempts - item.precedingLoadAttempts
					> m_expirationThreshold) {

					// Expire this request.  The reasoning behind
					// request expiration is described in
					// ThumbnailLoadResult::REQUEST_EXPIRED
					// documentation.

					postRequestExpiredResult(item_weak_ptr);
					continue;
				}

				// Expired requests don't count as load attempts.
				++m_totalLoadAttempts;

				// Copy those while holding the mutex.
				thumb_dir = m_thumbDir;
				max_thumb_size = m_maxThumbSize;
				accel_ops = m_ptrAccelOps;
			} // mutex scope

			assert(full_size_image_transform);

			boost::optional<AffineTransformedImage> const thumb(
				loadSaveThumbnail(
					thumb_id, *full_size_image_transform, accel_ops,
					thumb_dir, max_thumb_size
				)
			);
			if (thumb) {
				postLoadedResult(item_weak_ptr, *thumb);
			} else {
				postLoadFailedResult(item_weak_ptr);
			}
		} catch (std::bad_alloc const&) {
			OutOfMemoryHandler::instance().handleOutOfMemorySituation();
		}
	}
}

QString
ThumbnailPixmapCache::Impl::getThumbFilePath(
	ThumbId const& thumb_id, QString const& thumb_dir)
{
	// Because a project may have several files with the same name (from
	// different directories), we add a hash of the original image path
	// to the thumbnail file name.

	QByteArray const orig_path_hash(
		QCryptographicHash::hash(
			thumb_id.pageId.imageId().filePath().toUtf8(),
			QCryptographicHash::Md5
		).toHex()
	);
	QString const orig_path_hash_str(
		QString::fromLatin1(orig_path_hash.data(), orig_path_hash.size())
	);

	QFileInfo const orig_img_path(thumb_id.pageId.imageId().filePath());
	QString thumb_file_path(thumb_dir);
	thumb_file_path += QChar('/');
	thumb_file_path += orig_img_path.baseName();
	thumb_file_path += QChar('_');
	thumb_file_path += QString::number(thumb_id.pageId.imageId().zeroBasedPage());
	if (!thumb_id.isAffineTransform) {
		QChar sub_page_mark = QChar('S');
		switch (thumb_id.pageId.subPage()) {
			case PageId::SINGLE_PAGE:
				break;
			case PageId::LEFT_PAGE:
				sub_page_mark = QChar('L');
				break;
			case PageId::RIGHT_PAGE:
				sub_page_mark = QChar('R');
				break;
		}
		thumb_file_path += sub_page_mark;
	}
	thumb_file_path += QChar('_');
	thumb_file_path += orig_path_hash_str;
	thumb_file_path += QString::fromLatin1(".png");

	return thumb_file_path;
}

std::unique_ptr<QImageReader>
ThumbnailPixmapCache::Impl::validateThumbnailOnDisk(
	QString const& thumb_file_path,
	AbstractImageTransform const& full_size_image_transform)
{
	std::unique_ptr<QImageReader> thumb_reader(
		new QImageReader(thumb_file_path, "PNG")
	);

	if (!thumb_reader->canRead()) {
		thumb_reader.reset();
		return thumb_reader;
	}

	if (full_size_image_transform.isAffine()) {
		// We reuse the same thumbnail for all affine transforms,
		// so no further validation is required.
		return thumb_reader;
	}

	QString const expected_fingerprint(full_size_image_transform.fingerprint());
	QString const actual_fingerprint(thumb_reader->text(TRANSFORM_FINGERPRINT_KEY));
	if (expected_fingerprint != actual_fingerprint) {
		// The thumbnail on disk is stale and needs to be re-generated.
		thumb_reader.reset();
	}

	return thumb_reader;
}

boost::optional<AffineTransformedImage>
ThumbnailPixmapCache::Impl::loadSaveThumbnail(ThumbId const& thumb_id,
	AbstractImageTransform const& full_size_image_transform,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QString const& thumb_dir, QSize const& max_thumb_size)
{
	QString const thumb_file_path(getThumbFilePath(thumb_id, thumb_dir));
	std::unique_ptr<QImageReader> thumb_reader(
		validateThumbnailOnDisk(thumb_file_path, full_size_image_transform)
	);
	if (thumb_reader) { // Validation succeeded.
		QImage thumb_image(thumb_reader->read());
		if (thumb_image.isNull()) {
			return boost::optional<AffineTransformedImage>();
		}

		AffineImageTransform thumb_transform(full_size_image_transform.toAffine());
		thumb_transform.adjustForScaledOrigImage(thumb_image.size());
		return AffineTransformedImage(thumb_image, thumb_transform);
	}
	
	// Load full size image.
	QImage full_size_image(ImageLoader::load(thumb_id.pageId.imageId()));
	if (full_size_image.isNull()) {
		return boost::optional<AffineTransformedImage>();
	}

	boost::optional<AffineTransformedImage> const thumb(
		makeThumbnail(max_thumb_size, full_size_image, full_size_image_transform, accel_ops)
	);

	// Save thumbnail image.
	if (thumb) {
		saveThumbnail(thumb->origImage(), thumb_file_path);
	}

	return thumb;
}

boost::optional<AffineTransformedImage>
ThumbnailPixmapCache::Impl::makeThumbnail(
	QSize const& max_thumb_size, QImage const& full_size_image,
	AbstractImageTransform const& full_size_image_transform,
	std::shared_ptr<AcceleratableOperations> const& accel_ops)
{
	AffineImageTransform thumb_transform(full_size_image_transform.toAffine());

	if (full_size_image_transform.isAffine()) {
		QImage thumb_image(downscaleImage(full_size_image, max_thumb_size));
		thumb_transform.adjustForScaledOrigImage(thumb_image.size());
		return AffineTransformedImage(thumb_image, thumb_transform);
	}
	
	QSize const orig_affine_size(thumb_transform.origSize());
	QSize new_affine_size(orig_affine_size);
	new_affine_size.scale(max_thumb_size, Qt::KeepAspectRatio);
	new_affine_size = new_affine_size.expandedTo(QSize(1, 1));

	// Scale down, to get a thumbnail-size dewarped image.
	std::unique_ptr<AbstractImageTransform> downscaling_transform(
		full_size_image_transform.clone()
	);
	downscaling_transform->scale(
		double(new_affine_size.width()) / orig_affine_size.width(),
		double(new_affine_size.height()) / orig_affine_size.height()
	);

	AffineTransformedImage thumb(
		downscaling_transform->toAffine(
			full_size_image, Qt::transparent, accel_ops
		)
	);

	// Compensate the downscaling we did above.
	thumb_transform = thumb.xform();
	thumb_transform.scale(
		double(orig_affine_size.width()) / thumb_transform.origSize().width(),
		double(orig_affine_size.height()) / thumb_transform.origSize().height()
	);

	/* Set the transformation fingerprint. If a mismatch happens in the future,
	 * it would indicate the thumbnail on disk is stale. We don't do this for
	 * affine transforms, as in that case the thumbnail is merely a downscaled
	 * version of the original image, which we want to reuse for all kinds of
	 * affine transforms. */
	QImage thumb_image(thumb.origImage());
	thumb_image.setText(
		TRANSFORM_FINGERPRINT_KEY,
		full_size_image_transform.fingerprint()
	);

	return AffineTransformedImage(thumb_image, thumb_transform);
}

void
ThumbnailPixmapCache::Impl::saveThumbnail(
	QImage const& thumb, QString const& thumb_file_path)
{
	AtomicFileOverwriter overwriter;
	QIODevice* iodev = overwriter.startWriting(thumb_file_path);
	if (iodev && thumb.save(iodev, "PNG")) {
		overwriter.commit();
	}
}

QImage
ThumbnailPixmapCache::Impl::downscaleImage(
	QImage const& image, QSize const& max_size)
{
	if (image.width() < max_size.width() && image.height() < max_size.height()) {
		return image;
	}
	
	QSize to_size(image.size());
	to_size.scale(max_size, Qt::KeepAspectRatio);
	
	if (image.format() == QImage::Format_Indexed8 && image.isGrayscale()) {
		// This will be faster than QImage::scale().
		return scaleToGray(GrayImage(image), to_size);
	}
	
	return image.scaled(to_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void
ThumbnailPixmapCache::Impl::postRequestExpiredResult(
	std::weak_ptr<Item const*> const& item)
{
	LoadResultEvent* e = new LoadResultEvent(
		ThumbnailLoadResult::REQUEST_EXPIRED, item
	);
	QCoreApplication::postEvent(this, e);
}

void
ThumbnailPixmapCache::Impl::postLoadFailedResult(
	std::weak_ptr<Item const*> const& item)
{
	LoadResultEvent* e = new LoadResultEvent(
		ThumbnailLoadResult::LOAD_FAILED, item
	);
	QCoreApplication::postEvent(this, e);
}

void
ThumbnailPixmapCache::Impl::postLoadedResult(
	std::weak_ptr<Item const*> const& item, AffineTransformedImage const& thumb)
{
	LoadResultEvent* e = new LoadResultEvent(
		ThumbnailLoadResult::LOADED, item, thumb
	);
	QCoreApplication::postEvent(this, e);
}

void
ThumbnailPixmapCache::Impl::processLoadResult(LoadResultEvent* result)
{
	assert(QCoreApplication::instance()->thread() == QThread::currentThread());
	
	std::shared_ptr<Item const*> item_ptr(result->item.lock());
	if (!item_ptr) {
		// This means someone re-assigned Item::thisPtr, specifically
		// for the purpose to make the item inaccessible to any pending
		// operations.
		return;
	}
	assert(*item_ptr);

	QPixmap pixmap;
	if (result->thumb) {
		pixmap = QPixmap::fromImage(result->thumb->origImage());
	}
	
	std::vector<boost::weak_ptr<CompletionHandler> > completion_handlers;
	
	{
		QMutexLocker const locker(&m_mutex);
		
		if (m_shuttingDown) {
			return;
		}
		
		Item const& item = **item_ptr;
		item.completionHandlers.swap(completion_handlers);
		
		if (result->status == ThumbnailLoadResult::LOADED) {
			removeExcessGuiThreadLocked(); // Maybe remove an older item.
			transitionToLoadedStatusGuiThreadLocked(
				item, pixmap, result->thumb->xform()
			);
		} else if (result->status == ThumbnailLoadResult::LOAD_FAILED) {
			// We keep items that failed to load, as they are cheap
			// to keep and helps us avoid trying to load them
			// again and again.
			transitionToLoadFailedStatusLocked(item);
		} else {
			assert(result->status == ThumbnailLoadResult::REQUEST_EXPIRED);

			// Remove the whole item. We rely on this behaviour when retiring items.
			removeItemGuiThreadLocked(item);
		}
	} // mutex scope
	
	// Notify listeners.
	for (boost::weak_ptr<CompletionHandler> const& wh : completion_handlers) {
		boost::shared_ptr<CompletionHandler> const sh(wh.lock());
		if (sh.get()) {
			(*sh)(result->status);
		}
	}
}

void
ThumbnailPixmapCache::Impl::removeExcessGuiThreadLocked()
{
	assert(QCoreApplication::instance()->thread() == QThread::currentThread());

	if (m_numLoadedItems >= m_maxCachedPixmaps) {
		assert(m_numLoadedItems > 0);
		assert(!m_removeQueue.empty());
		assert(m_removeQueue.front().status == Item::LOADED);
		removeItemGuiThreadLocked(m_removeQueue.front());
	}
}

/**
 * @note Marked GuiThreadLocked because Item::pixmap can only be destroyed
 *       by the GUI thread.
 */
void
ThumbnailPixmapCache::Impl::removeItemGuiThreadLocked(Item const& item)
{
	assert(QCoreApplication::instance()->thread() == QThread::currentThread());

	switch (item.status) {
		case Item::QUEUED:
			assert(m_numQueuedItems > 0);
			--m_numQueuedItems;
			break;
		case Item::LOADED:
			assert(m_numLoadedItems > 0);
			--m_numLoadedItems;
			break;
		default:;
	}
	
	RemoveQueue::iterator const rq_it(m_removeQueue.iterator_to(item));
	if (m_endOfLoadedItems == rq_it) {
		++m_endOfLoadedItems;
	}
	m_removeQueue.erase(rq_it);
}

void
ThumbnailPixmapCache::Impl::transitionToInProgressStatusLocked(Item const& item)
{
	if (item.status != Item::IN_PROGRESS) {
		// In this case we can't let transitionFromCurrentStatusLocked()
		// to be called for IN_PROGRESS -> IN_PROGRESS transitions,
		// as it would clear Item::completionHandlers, which is not
		// what we want here.
		transitionFromCurrentStatusLocked(item);
	}

	// Item::thisPtr must be set in IN_PROGRESS state. If we are transitioning
	// from IN_PROGRESS to IN_PROGRESS, we still need to re-set it, as that
	// makes this item inaccessible to any pending operations that may be
	// in progress.
	item.thisPtr = std::make_shared<Item const*>(&item);

	item.status = Item::IN_PROGRESS;
}

void
ThumbnailPixmapCache::Impl::transitionToLoadedStatusGuiThreadLocked(
	Item const& item, QPixmap const& pixmap,
	AffineImageTransform const& pixmap_transform)
{
	assert(QCoreApplication::instance()->thread() == QThread::currentThread());

	// Note that in this case transitionFromCurrentStatusLocked()
	// is required even when transitioning from LOADED to LOADED status.
	transitionFromCurrentStatusLocked(item);

	item.pixmapTransform = pixmap_transform;
	item.pixmap = pixmap;

	// Move this item after all other LOADED items in
	// the remove queue.
	RemoveQueue::iterator const rq_it(m_removeQueue.iterator_to(item));
	m_removeQueue.relocate(m_endOfLoadedItems, rq_it);

	// Move to the end of load queue.
	LoadQueue::iterator const lq_it(m_loadQueue.iterator_to(item));
	m_loadQueue.relocate(m_loadQueue.end(), lq_it);

	++m_numLoadedItems;
	item.status = Item::LOADED;
}

void
ThumbnailPixmapCache::Impl::transitionToLoadFailedStatusLocked(Item const& item)
{
	if (item.status != Item::LOAD_FAILED) {
		transitionFromCurrentStatusLocked(item);
		item.status = Item::LOAD_FAILED;
	}
}

void
ThumbnailPixmapCache::Impl::transitionFromCurrentStatusLocked(Item const& item)
{
	switch (item.status) {
		case Item::QUEUED:
			prepareTransitionFromQueuedStatusLocked(item);
			break;
		case Item::IN_PROGRESS:
			prepareTransitionFromInProgressStatusLocked(item);
			break;
		case Item::LOADED:
			prepareTransitionFromLoadedStatusLocked(item);
			break;
		case Item::LOAD_FAILED:
			// Nothing special is required in this case.
			break;
	}
}

void
ThumbnailPixmapCache::Impl::prepareTransitionFromQueuedStatusLocked(Item const& item)
{
	assert(item.status == Item::QUEUED);

	// Move the item to the end of load queue.
	// The point is to keep QUEUED items before any others.
	LoadQueue::iterator const lq_it(m_loadQueue.iterator_to(item));
	m_loadQueue.relocate(m_loadQueue.end(), lq_it);

	--m_numQueuedItems;
	assert(m_numQueuedItems >= 0);
}

void
ThumbnailPixmapCache::Impl::prepareTransitionFromInProgressStatusLocked(Item const& item)
{
	assert(item.status == Item::IN_PROGRESS);

	// That's necessary to lock this item away from any pending operations.
	item.thisPtr.reset();

	// The assumption is the caller already did something meaningful
	// with completion handlers.
	item.completionHandlers.clear();
}

void
ThumbnailPixmapCache::Impl::prepareTransitionFromLoadedStatusLocked(Item const& item)
{
	assert(item.status == Item::LOADED);

	// Unfortunately we can't reset item.pixmap from here, as we may
	// not necessarily be in the GUI thread. We can reset pixmapTransform though.
	item.pixmapTransform.reset();

	// Move the item past m_endOfLoadedItems in remove queue.
	RemoveQueue::iterator const rq_it(m_removeQueue.iterator_to(item));
	m_removeQueue.relocate(m_endOfLoadedItems, rq_it);
	--m_endOfLoadedItems;

	--m_numLoadedItems;
	assert(m_numLoadedItems >= 0);
}

std::shared_ptr<AbstractImageTransform const> normalizedFullSizeImageTransform(
	AbstractImageTransform const& full_size_image_transform)
{
	if (!full_size_image_transform.isAffine()) {
		return full_size_image_transform.clone();
	} else {
		/* Turn it to identity transform. That's done to be able to use the same
		 * thumbnail image for any affine transforms the client may request. */
		std::shared_ptr<AffineImageTransform> identity_transform(
			std::make_shared<AffineImageTransform>(
				full_size_image_transform.toAffine()
			)
		);
		identity_transform->setTransform(QTransform());
		return identity_transform;
	}
}

/*====================== ThumbnailPixmapCache::Item =========================*/

ThumbnailPixmapCache::Item::Item(ThumbId const& thumb_id,
	AbstractImageTransform const& full_size_image_transform,
	int const preceding_load_attempts, Status const st)
:	thumbId(thumb_id),
	fullSizeImageTransform(
		normalizedFullSizeImageTransform(full_size_image_transform)
	),
	precedingLoadAttempts(preceding_load_attempts),
	status(st)
{
	if (st == IN_PROGRESS) {
		thisPtr = std::make_shared<Item const*>(this);
	}
}


/*=============== ThumbnailPixmapCache::Impl::LoadResultEvent ===============*/

ThumbnailPixmapCache::Impl::LoadResultEvent::LoadResultEvent(
	ThumbnailLoadResult::Status status, std::weak_ptr<Item const*> const& item)
:	QEvent(QEvent::User)
,	item(item)
,	status(status)
{
}

ThumbnailPixmapCache::Impl::LoadResultEvent::LoadResultEvent(
	ThumbnailLoadResult::Status status, std::weak_ptr<Item const*> const& item,
	AffineTransformedImage const& thumb)
:	QEvent(QEvent::User)
,	item(item)
,	thumb(thumb)
,	status(status)
{
}

ThumbnailPixmapCache::Impl::LoadResultEvent::~LoadResultEvent()
{
}


/*================== ThumbnailPixmapCache::BackgroundLoader =================*/

ThumbnailPixmapCache::Impl::BackgroundLoader::BackgroundLoader(Impl& owner)
:	m_rOwner(owner)
{
}

void
ThumbnailPixmapCache::Impl::BackgroundLoader::customEvent(QEvent*)
{
	m_rOwner.backgroundProcessing();
}
