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

#ifndef PAGE_LAYOUT_TASK_H_
#define PAGE_LAYOUT_TASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "FilterResult.h"
#include "PageId.h"
#include "CachingFactory.h"
#include "imageproc/AffineTransformedImage.h"
#include "acceleration/AcceleratableOperations.h"
#include <boost/optional.hpp>
#include <memory>

class TaskStatus;
class ContentBox;
class QImage;

namespace imageproc
{
	class GrayImage;
	class AbstractImageTransform;
}

namespace output
{
	class Task;
}

namespace page_layout
{

class Filter;
class Settings;

class Task : public RefCountable
{
	DECLARE_NON_COPYABLE(Task)
public:
	Task(IntrusivePtr<Filter> const& filter,
		IntrusivePtr<output::Task> const& next_task,
		IntrusivePtr<Settings> const& settings,
		PageId const& page_id, bool batch, bool debug);
	
	virtual ~Task();
	
	FilterResultPtr process(
		TaskStatus const& status,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		QImage const& orig_image,
		CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
		std::shared_ptr<imageproc::AbstractImageTransform const> const& orig_image_transform,
		boost::optional<imageproc::AffineTransformedImage> pre_transformed_image,
		ContentBox const& content_box);
private:
	class UiUpdater;
	
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<output::Task> m_ptrNextTask;
	IntrusivePtr<Settings> m_ptrSettings;
	PageId m_pageId;
	bool m_batchProcessing;
};

} // namespace page_layout

#endif
