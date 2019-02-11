/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2018 Alexander Trufanov <trufanovan@gmail.com>

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

#ifndef PUBLISHING_TASK_H_
#define PUBLISHING_TASK_H_

#include "NonCopyable.h"
#include "RefCountable.h"
#include "FilterResult.h"
#include "IntrusivePtr.h"
#include "ImageId.h"
#include "PageId.h"
#include "djview4/qdjvu.h"
#include <memory>

class TaskStatus;
class FilterData;
class QImage;

namespace publishing
{

class Filter;
class Settings;

class Task : public QObject, public RefCountable
{
    Q_OBJECT
	DECLARE_NON_COPYABLE(Task)
public:
    Task(IntrusivePtr<Filter> const& filter,
        IntrusivePtr<Settings> const& settings,
        PageId const& page_id, bool batch_processing);
	
	virtual ~Task();
	
    FilterResultPtr process(TaskStatus const& status, const QString &image_file, const QImage &out_image);

signals:
    void displayDjVu(const QString& djvu_filename);

private:
	class UiUpdater;
	
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<Settings> m_ptrSettings;
    PageId m_pageId;
	bool m_batchProcessing;
};

} // namespace publishing

#endif
