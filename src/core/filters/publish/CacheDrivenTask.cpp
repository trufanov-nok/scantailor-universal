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

#include "CacheDrivenTask.h"
#include "Settings.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "ImageTransformation.h"
#include "ThumbnailBase.h"
#include "AbstractFilterDataCollector.h"
#include "ThumbnailCollector.h"

namespace publish
{

CacheDrivenTask::CacheDrivenTask(IntrusivePtr<Settings> const& settings)
    :   m_ptrSettings(settings)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

void
CacheDrivenTask::process(
    PageInfo const& page_info, AbstractFilterDataCollector* collector)
{
    QRectF const initial_rect(QPointF(0.0, 0.0), page_info.metadata().size());
    ImageTransformation xform(initial_rect, page_info.metadata().dpi());

    if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
        thumb_col->processThumbnail(
            std::unique_ptr<QGraphicsItem>(
                new ThumbnailBase(
                    thumb_col->thumbnailCache(),
                    thumb_col->maxLogicalThumbSize(),
                    page_info.imageId(), xform
                )
            )
        );
    }
}

} // namespace publishing
