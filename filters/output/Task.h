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

#ifndef OUTPUT_TASK_H_
#define OUTPUT_TASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "FilterResult.h"
#include "PageId.h"
#include "ImageViewTab.h"
#include "OutputFileNameGenerator.h"
#include "CachingFactory.h"
#include "acceleration/AcceleratableOperations.h"
#include <QColor>
#include <memory>

class DebugImagesImpl;
class TaskStatus;
class ThumbnailPixmapCache;
class QPolygonF;
class QSize;
class QRectF;
class QImage;

namespace imageproc
{
	class BinaryImage;
	class GrayImage;
	class AbstractImageTransform;
}

namespace output
{

class Filter;
class Settings;

class Task : public RefCountable
{
	DECLARE_NON_COPYABLE(Task)
public:
	Task(IntrusivePtr<Filter> const& filter,
		IntrusivePtr<Settings> const& settings,
		IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
		PageId const& page_id, OutputFileNameGenerator const& out_file_name_gen,
		ImageViewTab last_tab, bool batch, bool debug);
	
	virtual ~Task();
	
	FilterResultPtr process(
		TaskStatus const& status,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		QImage const& orig_image,
		CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
		std::shared_ptr<imageproc::AbstractImageTransform const> const& orig_image_transform,
		QRectF const& content_rect, QRectF const& outer_rect);
private:
	class UiUpdater;
	
	FilterResultPtr processScaled(
		TaskStatus const& status, QImage const& orig_image,
		CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
		std::shared_ptr<imageproc::AbstractImageTransform const> const& orig_image_transform,
		QRectF const& content_rect, QRectF const& outer_rect);

	void deleteMutuallyExclusiveOutputFiles();

	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
	IntrusivePtr<ThumbnailPixmapCache> m_ptrThumbnailCache;
	std::auto_ptr<DebugImagesImpl> m_ptrDbg;
	PageId m_pageId;
	OutputFileNameGenerator m_outFileNameGen;
	ImageViewTab m_lastTab;
	bool m_batchProcessing;
	bool m_debug;
};

} // namespace output

#endif
