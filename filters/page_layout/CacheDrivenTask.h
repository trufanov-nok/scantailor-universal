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

#ifndef PAGE_LAYOUT_CACHEDRIVENTASK_H_
#define PAGE_LAYOUT_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "ContentBox.h"
#include <memory>

class QRectF;
class PageInfo;
class AbstractFilterDataCollector;

namespace imageproc
{
	class AbstractImageTransform;
}

namespace output
{
	class CacheDrivenTask;
}

namespace page_layout
{

class Settings;

class CacheDrivenTask : public RefCountable
{
	DECLARE_NON_COPYABLE(CacheDrivenTask)
public:
	CacheDrivenTask(IntrusivePtr<output::CacheDrivenTask> const& next_task,
		IntrusivePtr<Settings> const& settings);
	
	virtual ~CacheDrivenTask();
	
	void process(
		PageInfo const& page_info, AbstractFilterDataCollector* collector,
		std::shared_ptr<imageproc::AbstractImageTransform const> const& full_size_image_transform,
		ContentBox const& content_box);
private:
	IntrusivePtr<output::CacheDrivenTask> m_ptrNextTask;
	IntrusivePtr<Settings> m_ptrSettings;
};

} // namespace page_layout

#endif
