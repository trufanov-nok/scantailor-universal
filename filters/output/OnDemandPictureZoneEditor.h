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


#ifndef OUTPUT_ON_DEMAND_PICTURE_ZONE_EDITOR_H_
#define OUTPUT_ON_DEMAND_PICTURE_ZONE_EDITOR_H_

#include "IntrusivePtr.h"
#include "PageId.h"
#include "Settings.h"
#include "CachingFactory.h"
#include "imageproc/BinaryImage.h"
#include <QStackedWidget>
#include <QImage>
#include <functional>

class ProcessingIndicationWidget;

namespace output
{

/**
 * @brief Constructs an instance of PictureZoneEditor on demand,
 *        serving as its proxy while it's being constructed.
 */
class OnDemandPictureZoneEditor : public QStackedWidget
{
	Q_OBJECT
public:
	/**
	 * @see PictureZoneEditor::PictureZoneEditor()
	 */
	OnDemandPictureZoneEditor(
		CachingFactory<QImage> const& cached_transformed_orig_image,
		CachingFactory<QImage> const& cached_downscaled_transformed_orig_image,
		imageproc::BinaryImage const& output_picture_mask,
		PageId const& page_id, IntrusivePtr<Settings> const& settings,
		std::function<QPointF(QPointF const&)> const& orig_to_output,
		std::function<QPointF(QPointF const&)> const& output_to_orig);
signals:
	void invalidateThumbnail(PageId const& page_id);
protected:
	virtual void showEvent(QShowEvent* evt);
private:
	class ImageTransformationTask;
	class ImageTransformationResult;

	void buildRealPictureZoneEditor();

	CachingFactory<QImage> m_cachedTransformedOrigImage;
	CachingFactory<QImage> m_cachedDownscaledTransformedOrigImage;
	imageproc::BinaryImage m_outputPictureMask;
	PageId m_pageId;
	IntrusivePtr<Settings> m_ptrSettings;
	std::function<QPointF(QPointF const&)> m_origToOutput;
	std::function<QPointF(QPointF const&)> m_outputToOrig;
	ProcessingIndicationWidget* m_pProcessingIndicator;
	bool m_backgroundTaskSubmitted;
};

} // namespace output

#endif
