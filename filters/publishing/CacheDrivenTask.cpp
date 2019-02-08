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

#include "CacheDrivenTask.h"
#include "Settings.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "IncompleteThumbnail.h"
#include "ImageTransformation.h"
#include "ThumbnailBase.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"

namespace publishing
{

CacheDrivenTask::CacheDrivenTask(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

void
CacheDrivenTask::process(
    PageInfo const& page_info, AbstractFilterDataCollector* collector, QString const& outputFile, ImageTransformation const& xform)
{
    Params p = m_ptrSettings->getParams(page_info.id());
    Params::Regenerate val = p.getForceReprocess();
    bool need_reprocess = val & Params::RegenerateThumbnail;
    if (need_reprocess) {
        val = (Params::Regenerate) (val & ~Params::RegenerateThumbnail);
        p.setForceReprocess(val);
        m_ptrSettings->setParams(page_info.id(), p);
    }

    if (!need_reprocess) {
        need_reprocess = !QFile::exists(outputFile);
        if (!need_reprocess) {
            need_reprocess = !QFile::exists( p.djvuFilename() );
        }
    }


    QString thumbnail_source;

	if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {    
        if (need_reprocess) {

            thumbnail_source = outputFile;
            if (outputFile.isEmpty() || !QFileInfo(outputFile).exists()) {
                thumbnail_source = page_info.id().imageId().filePath();
            }

            thumb_col->processThumbnail(
                std::unique_ptr<QGraphicsItem>(
                    new IncompleteThumbnail(
                        thumb_col->thumbnailCache(),
                        thumb_col->maxLogicalThumbSize(),
                                ImageId(thumbnail_source), xform
                    )
                )
            );
        } else {
            thumb_col->processThumbnail(
                        std::unique_ptr<QGraphicsItem>(
                            new ThumbnailBase(
                                thumb_col->thumbnailCache(),
                                thumb_col->maxLogicalThumbSize(),
                                ImageId(p.djvuFilename()), xform
                                )
                            )
                        );
        }
    }
}

} // namespace publishing
