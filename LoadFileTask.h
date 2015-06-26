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

#ifndef LOADFILETASK_H_
#define LOADFILETASK_H_

#include "NonCopyable.h"
#include "BackgroundTask.h"
#include "FilterResult.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include "ImageMetadata.h"
#include "acceleration/AcceleratableOperations.h"
#include <memory>

class ThumbnailPixmapCache;
class PageInfo;
class ProjectPages;
class QImage;

namespace fix_orientation
{
	class Task;
}

class LoadFileTask : public BackgroundTask
{
	DECLARE_NON_COPYABLE(LoadFileTask)
public:
	LoadFileTask(Type type, PageInfo const& page,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
		IntrusivePtr<ProjectPages> const& pages,
		IntrusivePtr<fix_orientation::Task> const& next_task);
	
	virtual ~LoadFileTask();
	
	virtual FilterResultPtr operator()();
private:
	class ErrorResult;
	
	void updateImageSizeIfChanged(QImage const& image);
	
	std::shared_ptr<AcceleratableOperations> m_ptrAccelOps;
	IntrusivePtr<ThumbnailPixmapCache> m_ptrThumbnailCache;
	PageId m_pageId;
	ImageMetadata m_imageMetadata;
	IntrusivePtr<ProjectPages> const m_ptrPages;
	IntrusivePtr<fix_orientation::Task> const m_ptrNextTask;
};

#endif
