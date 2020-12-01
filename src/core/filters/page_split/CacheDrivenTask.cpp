/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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
#include "Thumbnail.h"
#include "IncompleteThumbnail.h"
#include "Settings.h"
#include "ProjectPages.h"
#include "PageInfo.h"
#include "ImageTransformation.h"
#include "AbstractFilterDataCollector.h"
#include "ThumbnailCollector.h"
#include "filters/deskew/CacheDrivenTask.h"

namespace page_split
{

CacheDrivenTask::CacheDrivenTask(IntrusivePtr<Settings> const& settings, IntrusivePtr<ProjectPages> projectPages,
                                 IntrusivePtr<deskew::CacheDrivenTask> const& next_task)
    :   m_ptrNextTask(next_task),
        m_ptrSettings(settings),
        m_projectPages(projectPages)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

static ProjectPages::LayoutType toPageLayoutType(PageLayout const& layout)
{
    switch (layout.type()) {
    case PageLayout::SINGLE_PAGE_UNCUT:
    case PageLayout::SINGLE_PAGE_CUT:
        return ProjectPages::ONE_PAGE_LAYOUT;
    case PageLayout::TWO_PAGES:
        return ProjectPages::TWO_PAGE_LAYOUT;
    }

    assert(!"Unreachable");
    return ProjectPages::ONE_PAGE_LAYOUT;
}

void
CacheDrivenTask::process(
    PageInfo const& page_info, AbstractFilterDataCollector* collector,
    ImageTransformation const& xform)
{
    Settings::Record const record(
        m_ptrSettings->getPageRecord(page_info.imageId())
    );

    OrthogonalRotation const pre_rotation(xform.preRotation());
    Dependencies const deps(
        page_info.metadata().size(), pre_rotation,
        record.combinedLayoutType()
    );

    Params const* params = record.params();

    bool compatibleWith = true;
    if (!params || !(compatibleWith = deps.compatibleWith(*params))) {
        if (!compatibleWith) {
            Params new_params = *params;

            compatibleWith = deps.fixCompatibility(new_params);

            if (compatibleWith) {
                Settings::UpdateAction update_params;
                update_params.setParams(new_params);
                m_ptrSettings->updatePage(page_info.imageId(), update_params);
            }
        }

        bool need_reprocess(!params || !compatibleWith);
        if (!need_reprocess) {
            Params p(*params);
            Params::Regenerate val = p.getForceReprocess();
            need_reprocess = val & Params::RegenerateThumbnail;
            if (need_reprocess && !m_ptrNextTask) {
                val = (Params::Regenerate)(val & ~Params::RegenerateThumbnail);
                p.setForceReprocess(val);
                Settings::UpdateAction update_params;
                update_params.setParams(p);
                m_ptrSettings->updatePage(page_info.imageId(), update_params);
            }
        }

        if (need_reprocess) {
            if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
                thumb_col->processThumbnail(
                    std::unique_ptr<QGraphicsItem>(
                        new IncompleteThumbnail(
                            thumb_col->thumbnailCache(),
                            thumb_col->maxLogicalThumbSize(),
                            page_info.imageId(), xform
                        )
                    )
                );
            }

            return;
        }
    }

    PageLayout layout(params->pageLayout());
    if (layout.uncutOutline().isEmpty()) {
        // Backwards compatibility with versions < 0.9.9
        layout.setUncutOutline(xform.resultingRect());
    }

    // m_projectPages controls number of pages displayed in thumbnail list
    // usually this is set in Task, but if user changed layout with Apply To..
    // and just jumped to next stage - the Task::process isn't invoked for all pages
    // so we must additionally ensure here that we display right number of pages.

    m_projectPages->setLayoutTypeFor(page_info.id().imageId(), toPageLayoutType(layout));

    if (m_ptrNextTask) {
        ImageTransformation new_xform(xform);
        new_xform.setPreCropArea(layout.pageOutline(page_info.id().subPage()));
        m_ptrNextTask->process(page_info, collector, new_xform);
        return;
    }

    if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
        thumb_col->processThumbnail(
            std::unique_ptr<QGraphicsItem>(
                new Thumbnail(
                    thumb_col->thumbnailCache(),
                    thumb_col->maxLogicalThumbSize(),
                    page_info.imageId(), xform, layout,
                    page_info.leftHalfRemoved(),
                    page_info.rightHalfRemoved()
                )
            )
        );
    }
}

} // namespace page_split
