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

#include "CacheDrivenTask.h"
#include "Settings.h"
#include "Params.h"
#include "Thumbnail.h"
#include "IncompleteThumbnail.h"
#include "PageLayout.h"
#include "PageInfo.h"
#include "PageId.h"
#include "Utils.h"
#include "filters/output/CacheDrivenTask.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include <QSizeF>
#include <QRectF>
#include <QPolygonF>
#include <memory>

using namespace imageproc;

namespace page_layout
{

CacheDrivenTask::CacheDrivenTask(
	IntrusivePtr<output::CacheDrivenTask> const& next_task,
	IntrusivePtr<Settings> const& settings)
:	m_ptrNextTask(next_task)
,	m_ptrSettings(settings)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

void
CacheDrivenTask::process(
	PageInfo const& page_info, AbstractFilterDataCollector* collector,
	std::shared_ptr<imageproc::AbstractImageTransform const> const& full_size_image_transform,
	ContentBox const& content_box)
{
	std::auto_ptr<Params> const params(
		m_ptrSettings->getPageParams(page_info.id())
	);
	if (!params.get() || !params->contentSize().isValid()) {
		if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new IncompleteThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.id(), *full_size_image_transform
					)
				)
			);
		}
		return;
	}

	QRectF const unscaled_content_rect(
		content_box.toTransformedRect(*full_size_image_transform)
	);

	std::shared_ptr<AbstractImageTransform> adjusted_transform(
		full_size_image_transform->clone()
	);

	PageLayout const page_layout(
		unscaled_content_rect, m_ptrSettings->getAggregateHardSize(),
		params->matchSizeMode(), params->alignment(), params->hardMargins()
	);
	page_layout.absorbScalingIntoTransform(*adjusted_transform);

	if (m_ptrNextTask) {

		m_ptrNextTask->process(
			page_info, adjusted_transform,
			page_layout.innerRect(), page_layout.outerRect(), collector
		);
		return;
	}
	
	if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
		
		std::auto_ptr<QGraphicsItem> thumb;

		if (content_box.isValid()) {
			thumb.reset(
				new Thumbnail(
					thumb_col->thumbnailCache(),
					thumb_col->maxLogicalThumbSize(),
					page_info.id(), *params,
					*adjusted_transform, page_layout
				)
			);
		} else {
			thumb.reset(
				new ThumbnailBase(
					thumb_col->thumbnailCache(),
					thumb_col->maxLogicalThumbSize(),
					page_info.id(),
					*full_size_image_transform
				)
			);
		}

		thumb_col->processThumbnail(thumb);
	}
}

} // namespace page_layout
