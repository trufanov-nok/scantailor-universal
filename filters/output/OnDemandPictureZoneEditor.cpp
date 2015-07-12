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

#include "OnDemandPictureZoneEditor.h"
#include "PictureZoneEditor.h"
#include "AbstractCommand.h"
#include "BackgroundExecutor.h"
#include "BackgroundTask.h"
#include "ImageViewBase.h"
#include "OutputMargins.h"
#include "ProcessingIndicationWidget.h"
#include "imageproc/BinaryImage.h"
#include <QPointer>
#include <QDebug>
#include <memory>

using namespace imageproc;

namespace output
{

class OnDemandPictureZoneEditor::ImageTransformationTask :
	public AbstractCommand0<BackgroundExecutor::TaskResultPtr>
{
public:
	ImageTransformationTask(
		OnDemandPictureZoneEditor* owner,
		CachingFactory<QImage> const& cached_transformed_orig_image,
		CachingFactory<QImage> const& cached_downscaled_transformed_orig_image);

	virtual BackgroundExecutor::TaskResultPtr operator()();
private:
	QPointer<OnDemandPictureZoneEditor> m_ptrOwner;
	CachingFactory<QImage> m_cachedTransformedOrigImage;
	CachingFactory<QImage> m_cachedDownscaledTransformedOrigImage;
};


class OnDemandPictureZoneEditor::ImageTransformationResult : public AbstractCommand0<void>
{
public:
	ImageTransformationResult(QPointer<OnDemandPictureZoneEditor> const& owner);

	virtual void operator()();
private:
	QPointer<OnDemandPictureZoneEditor> m_ptrOwner;
};


/*========================== OnDemandPictureZoneEditor ==========================*/

OnDemandPictureZoneEditor::OnDemandPictureZoneEditor(
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	CachingFactory<QImage> const& cached_transformed_orig_image,
	CachingFactory<QImage> const& cached_downscaled_transformed_orig_image,
	imageproc::BinaryImage const& output_picture_mask,
	PageId const& page_id, IntrusivePtr<Settings> const& settings,
	std::function<QPointF(QPointF const&)> const& orig_to_output,
	std::function<QPointF(QPointF const&)> const& output_to_orig)
:	m_ptrAccelOps(accel_ops)
,	m_cachedTransformedOrigImage(cached_transformed_orig_image)
,	m_cachedDownscaledTransformedOrigImage(cached_downscaled_transformed_orig_image)
,	m_outputPictureMask(output_picture_mask)
,	m_pageId(page_id)
,	m_ptrSettings(settings)
,	m_origToOutput(orig_to_output)
,	m_outputToOrig(output_to_orig)
,	m_pProcessingIndicator(new ProcessingIndicationWidget(this))
,	m_backgroundTaskSubmitted(false)
{
	addWidget(m_pProcessingIndicator);

	if (cached_transformed_orig_image.isCached()) {
		buildRealPictureZoneEditor();
	}
}

void
OnDemandPictureZoneEditor::showEvent(QShowEvent* evt)
{
	QStackedWidget::showEvent(evt);

	if (!m_backgroundTaskSubmitted && currentWidget() == m_pProcessingIndicator) {
		BackgroundExecutor::TaskPtr const task(
			new ImageTransformationTask(
				this, m_cachedTransformedOrigImage,
				m_cachedDownscaledTransformedOrigImage
			)
		);
		ImageViewBase::backgroundExecutor().enqueueTask(task);
		m_backgroundTaskSubmitted = true;
	}
}

void
OnDemandPictureZoneEditor::buildRealPictureZoneEditor()
{
	std::unique_ptr<QWidget> widget(
		new PictureZoneEditor(
			m_ptrAccelOps,
			m_cachedTransformedOrigImage(),
			m_cachedDownscaledTransformedOrigImage(),
			m_outputPictureMask, m_pageId, m_ptrSettings,
			m_origToOutput, m_outputToOrig
		)
	);

	connect(
		widget.get(), SIGNAL(invalidateThumbnail(PageId const&)),
		this, SIGNAL(invalidateThumbnail(PageId const&))
	);

	setCurrentIndex(addWidget(widget.release()));
}


/*========================= ImageTransformationTask =======================*/

OnDemandPictureZoneEditor::ImageTransformationTask::ImageTransformationTask(
	OnDemandPictureZoneEditor* owner,
	CachingFactory<QImage> const& cached_transformed_orig_image,
	CachingFactory<QImage> const& cached_downscaled_transformed_orig_image)
:	m_ptrOwner(owner)
,	m_cachedTransformedOrigImage(cached_transformed_orig_image)
,	m_cachedDownscaledTransformedOrigImage(cached_downscaled_transformed_orig_image)
{
}

/** @note This method is called on a background thread. */
BackgroundExecutor::TaskResultPtr
OnDemandPictureZoneEditor::ImageTransformationTask::operator()()
{
	// Create images and populate the cache.
	m_cachedTransformedOrigImage();
	m_cachedDownscaledTransformedOrigImage();

	return BackgroundExecutor::TaskResultPtr(
		new ImageTransformationResult(m_ptrOwner)
	);
}


/*===================== ImageTransformationResult ======================*/

OnDemandPictureZoneEditor::ImageTransformationResult::ImageTransformationResult(
	QPointer<OnDemandPictureZoneEditor> const& owner)
:	m_ptrOwner(owner)
{
}

/** @note This method is called on the GUI thread. */
void
OnDemandPictureZoneEditor::ImageTransformationResult::operator()()
{
	if (OnDemandPictureZoneEditor* owner = m_ptrOwner) {
		owner->buildRealPictureZoneEditor();
	}
}

} // namespace output
