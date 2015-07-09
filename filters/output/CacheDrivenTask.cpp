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
#include "OutputGenerator.h"
#include "PictureZoneComparator.h"
#include "FillZoneComparator.h"
#include "Settings.h"
#include "Params.h"
#include "Thumbnail.h"
#include "IncompleteThumbnail.h"
#include "PageInfo.h"
#include "PageId.h"
#include "ImageId.h"
#include "Utils.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include <QString>
#include <QFileInfo>
#include <QRect>
#include <QRectF>
#include <QTransform>

using namespace imageproc;

namespace output
{

CacheDrivenTask::CacheDrivenTask(
	IntrusivePtr<Settings> const& settings,
	OutputFileNameGenerator const& out_file_name_gen)
:	m_ptrSettings(settings),
	m_outFileNameGen(out_file_name_gen)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

void
CacheDrivenTask::process(
	PageInfo const& page_info,
	std::shared_ptr<AbstractImageTransform const> const& full_size_image_transform,
	QRectF const& content_rect, QRectF const& outer_rect,
	AbstractFilterDataCollector* collector)
{
	double const scaling_factor = m_ptrSettings->scalingFactor();
	std::shared_ptr<AbstractImageTransform> const scaled_transform(full_size_image_transform->clone());
	QTransform const post_scale_xform(scaled_transform->scale(scaling_factor, scaling_factor));

	return processScaled(
		page_info, scaled_transform, post_scale_xform.mapRect(content_rect),
		post_scale_xform.mapRect(outer_rect), collector
	);
}

void
CacheDrivenTask::processScaled(
	PageInfo const& page_info,
	std::shared_ptr<AbstractImageTransform const> const& full_size_image_transform,
	QRectF const& content_rect, QRectF const& outer_rect,
	AbstractFilterDataCollector* collector)
{
	if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
		
		QString const out_file_path(m_outFileNameGen.filePathFor(page_info.id()));
		Params const params(m_ptrSettings->getParams(page_info.id()));

		OutputGenerator const generator(
			full_size_image_transform, content_rect, outer_rect,
			params.colorParams(), params.despeckleLevel()
		);

		bool need_reprocess = false;

		do { // Just to be able to break from it.

			std::auto_ptr<OutputParams> stored_output_params(
				m_ptrSettings->getOutputParams(page_info.id())
			);

			if (!stored_output_params.get()) {
				need_reprocess = true;
				break;
			}

			OutputImageParams const new_output_image_params(
				full_size_image_transform->fingerprint(),
				generator.outputImageRect(), generator.outputContentRect(),
				params.colorParams(), params.despeckleLevel()
			);

			if (!stored_output_params->outputImageParams().matches(new_output_image_params)) {
				need_reprocess = true;
				break;
			}

			ZoneSet const new_picture_zones(m_ptrSettings->pictureZonesForPage(page_info.id()));
			if (!PictureZoneComparator::equal(stored_output_params->pictureZones(), new_picture_zones)) {
				need_reprocess = true;
				break;
			}

			ZoneSet const new_fill_zones(m_ptrSettings->fillZonesForPage(page_info.id()));
			if (!FillZoneComparator::equal(stored_output_params->fillZones(), new_fill_zones)) {
				need_reprocess = true;
				break;
			}
			
			QFileInfo const out_file_info(out_file_path);

			if (!out_file_info.exists()) {
				need_reprocess = true;
				break;
			}
			
			if (!stored_output_params->outputFileParams().matches(OutputFileParams(out_file_info))) {
				need_reprocess = true;
				break;
			}
		} while (false);

		if (need_reprocess) {
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new IncompleteThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.id(), *full_size_image_transform
					)
				)
			);
		} else {
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new Thumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						PageId(ImageId(out_file_path)),
						AffineImageTransform(generator.outputImageSize())
					)
				)
			);
		}
	}
}

} // namespace output
