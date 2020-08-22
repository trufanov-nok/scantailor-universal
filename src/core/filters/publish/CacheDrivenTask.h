/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2020 Alexander Trufanov <trufanovan@gmail.com>

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

#ifndef PUBLISH_CACHEDRIVENTASK_H_
#define PUBLISH_CACHEDRIVENTASK_H_

#include "NonCopyable.h"
#include "CompositeCacheDrivenTask.h"
#include "IntrusivePtr.h"

class PageInfo;
class AbstractFilterDataCollector;

namespace publish
{

class Settings;

class CacheDrivenTask : public CompositeCacheDrivenTask
{
    DECLARE_NON_COPYABLE(CacheDrivenTask)
public:
    CacheDrivenTask(
        IntrusivePtr<Settings> const& settings);

    virtual ~CacheDrivenTask();

    virtual void process(
        PageInfo const& page_info, AbstractFilterDataCollector* collector);
private:
    IntrusivePtr<Settings> m_ptrSettings;
};

} // namespace publishing

#endif
