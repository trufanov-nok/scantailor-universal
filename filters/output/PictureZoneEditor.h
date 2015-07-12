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


#ifndef OUTPUT_PICTURE_ZONE_EDITOR_H_
#define OUTPUT_PICTURE_ZONE_EDITOR_H_

#include "ImageViewBase.h"
#include "NonCopyable.h"
#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include "ZoneInteractionContext.h"
#include "EditableSpline.h"
#include "EditableZoneSet.h"
#include "ZoomHandler.h"
#include "DragHandler.h"
#include "ImagePixmapUnion.h"
#include "imageproc/BinaryImage.h"
#include "acceleration/AcceleratableOperations.h"
#include <QTransform>
#include <QPoint>
#include <QTimer>
#include <QPixmap>
#include <functional>
#include <memory>

class InteractionState;
class QPainter;
class QMenu;

namespace output
{

class Settings;


class PictureZoneEditor : public ImageViewBase, private InteractionHandler
{
	Q_OBJECT
public:
	/**
	 * @param accel_ops OpenCL-acceleratable operations.
	 * @param transformed_orig_image The original image transformed
	 *        into output image coordinates.
	 * @param downscaled_transformed_orig_image A downscaled version of
	 *        @p transformed_orig_image or a default-constructed ImagePixmapUnion.
	 *        Use ImageViewBase::createDownscaledImage() to create a downscaled image.
	 * @param output_picture_mask A binary image in output image coordinates
	 *        where white pixels correspond to auto-detected picture areas.
	 * @param page_id Identifies the page.
	 * @param settings Output stage settings.
	 * @param orig_to_output Mapper from original to output image coordinates.
	 * @param output_to_orig Mapper from output to original image coordinates.
	 */
	PictureZoneEditor(
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		QImage const& transformed_orig_image,
		ImagePixmapUnion const& downscaled_transformed_orig_image,
		imageproc::BinaryImage const& output_picture_mask,
		PageId const& page_id, IntrusivePtr<Settings> const& settings,
		std::function<QPointF(QPointF const&)> const& orig_to_output,
		std::function<QPointF(QPointF const&)> const& output_to_orig);
	
	virtual ~PictureZoneEditor();
signals:
	void invalidateThumbnail(PageId const& page_id);
protected:
	virtual void onPaint(QPainter& painter, InteractionState const& interaction);
private slots:
	void advancePictureMaskAnimation();

	void initiateBuildingScreenPictureMask();

	void commitZones();

	void updateRequested();
private:
	class MaskTransformTask;
	
	bool validateScreenPictureMask() const;

	void schedulePictureMaskRebuild();

	void screenPictureMaskBuilt(QPoint const& origin, QImage const& mask);

	void paintOverPictureMask(QPainter& painter);

	void showPropertiesDialog(EditableZoneSet::Zone const& zone);

	std::shared_ptr<AcceleratableOperations> m_ptrAccelOps;

	EditableZoneSet m_zones;

	// Must go after m_zones.
	ZoneInteractionContext m_context;

	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;

	imageproc::BinaryImage m_outputPictureMask;
	QPixmap m_screenPictureMask;
	QPoint m_screenPictureMaskOrigin;
	QTransform m_screenPictureMaskXform;
	QTransform m_potentialPictureMaskXform;
	QTimer m_pictureMaskRebuildTimer;
	QTimer m_pictureMaskAnimateTimer;
	int m_pictureMaskAnimationPhase; // degrees
	IntrusivePtr<MaskTransformTask> m_ptrMaskTransformTask;

	PageId m_pageId;
	IntrusivePtr<Settings> m_ptrSettings;
	std::function<QPointF(QPointF const&)> m_origToOutput;
	std::function<QPointF(QPointF const&)> m_outputToOrig;
};

} // namespace output

#endif
