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
#include "DistortionType.h"
#include "RotationThumbnail.h"
#include "DewarpingThumbnail.h"
#include "IncompleteThumbnail.h"
#include "Settings.h"
#include "PageInfo.h"
#include "AbstractImageTransform.h"
#include "AffineImageTransform.h"
#include "DewarpingImageTransform.h"
#include "filter_dc/AbstractFilterDataCollector.h"
#include "filter_dc/ThumbnailCollector.h"
#include "filters/select_content/CacheDrivenTask.h"
#include <QPointF>
#include <memory>
#include <cassert>
#include <vector>

namespace deskew
{

CacheDrivenTask::CacheDrivenTask(
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<select_content::CacheDrivenTask> const& next_task)
:	m_ptrNextTask(next_task),
	m_ptrSettings(settings)
{
}

CacheDrivenTask::~CacheDrivenTask()
{
}

void
CacheDrivenTask::process(
	PageInfo const& page_info, OrthogonalRotation const& pre_rotation,
	AffineImageTransform const& image_transform,
	AbstractFilterDataCollector* collector)
{
	Dependencies const deps(image_transform.origCropArea(), pre_rotation);

	std::auto_ptr<Params> params(m_ptrSettings->getPageParams(page_info.id()));
	if (!params.get() || !deps.matches(params->dependencies()) ||
			!params->validForDistortionType(params->distortionType())) {
		
		if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {
			thumb_col->processThumbnail(
				std::auto_ptr<QGraphicsItem>(
					new IncompleteThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.id(), image_transform
					)
				)
			);
		}
		
		return;
	}
	
	if (m_ptrNextTask) {

		std::shared_ptr<AbstractImageTransform const> new_transform;

		switch (params->distortionType()) {
			case DistortionType::NONE: {
				new_transform = image_transform.clone();
				break;
			}
			case DistortionType::ROTATION: {
				auto rotated = std::make_shared<AffineImageTransform>(image_transform);
				rotated->rotate(params->rotationParams().compensationAngleDeg());
				new_transform = std::move(rotated);
				break;
			}
			case DistortionType::PERSPECTIVE: {
				std::vector<QPointF> const top_curve{
					params->perspectiveParams().corner(PerspectiveParams::TOP_LEFT),
					params->perspectiveParams().corner(PerspectiveParams::TOP_RIGHT)
				};
				std::vector<QPointF> const bottom_curve{
					params->perspectiveParams().corner(PerspectiveParams::BOTTOM_LEFT),
					params->perspectiveParams().corner(PerspectiveParams::BOTTOM_RIGHT)
				};
				new_transform = std::make_shared<DewarpingImageTransform>(
					image_transform.origSize(), image_transform.origCropArea(),
					top_curve, bottom_curve, dewarping::DepthPerception()
				);
				break;
			}
			case DistortionType::WARP: {
				new_transform = std::make_shared<DewarpingImageTransform>(
					image_transform.origSize(), image_transform.origCropArea(),
					params->dewarpingParams().distortionModel().topCurve().polyline(),
					params->dewarpingParams().distortionModel().bottomCurve().polyline(),
					params->dewarpingParams().depthPerception()
				);
				break;
			}
		}

		assert(new_transform);
		m_ptrNextTask->process(page_info, new_transform, collector);
		return;
	}
	
	if (ThumbnailCollector* thumb_col = dynamic_cast<ThumbnailCollector*>(collector)) {

		std::auto_ptr<QGraphicsItem> thumb;

		switch (params->distortionType()) {
			case DistortionType::NONE: {
				thumb.reset(
					new RotationThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.id(), image_transform,
						/*compensation_angle_deg=*/0.0,
						/*grid_visible=*/false
					)
				);
				break;
			}
			case DistortionType::ROTATION: {
				thumb.reset(
					new RotationThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.id(), image_transform,
						params->rotationParams().compensationAngleDeg(),
						/*grid_visible=*/true
					)
				);
				break;
			}
			case DistortionType::PERSPECTIVE: {
				std::vector<QPointF> const top_curve{
					params->perspectiveParams().corner(PerspectiveParams::TOP_LEFT),
					params->perspectiveParams().corner(PerspectiveParams::TOP_RIGHT)
				};
				std::vector<QPointF> const bottom_curve{
					params->perspectiveParams().corner(PerspectiveParams::BOTTOM_LEFT),
					params->perspectiveParams().corner(PerspectiveParams::BOTTOM_RIGHT)
				};
				thumb.reset(
					new DewarpingThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.id(), image_transform,
						top_curve, bottom_curve,
						dewarping::DepthPerception()
					)
				);
				break;
			}
			case DistortionType::WARP: {
				thumb.reset(
					new DewarpingThumbnail(
						thumb_col->thumbnailCache(),
						thumb_col->maxLogicalThumbSize(),
						page_info.id(), image_transform,
						params->dewarpingParams().distortionModel().topCurve().polyline(),
						params->dewarpingParams().distortionModel().bottomCurve().polyline(),
						params->dewarpingParams().depthPerception()
					)
				);
				break;
			}
		}

		assert(thumb.get());
		thumb_col->processThumbnail(thumb);
	}
}

} // namespace deskew
