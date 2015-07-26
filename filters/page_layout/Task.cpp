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

#include "Task.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "RelativeMargins.h"
#include "Params.h"
#include "Utils.h"
#include "FilterUiInterface.h"
#include "TaskStatus.h"
#include "ImageView.h"
#include "BasicImageView.h"
#include "ContentBox.h"
#include "PageLayout.h"
#include "filters/output/Task.h"
#include "imageproc/AffineTransformedImage.h"
#include <QSizeF>
#include <QRectF>
#include <QLineF>
#include <QPolygonF>
#include <QTransform>
#include <QObject>

#include "CommandLine.h"

using namespace imageproc;

namespace page_layout
{

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		IntrusivePtr<Settings> const& settings,
		PageId const& page_id,
		std::shared_ptr<AbstractImageTransform const> const& orig_transform,
		AffineTransformedImage const& affine_transformed_image,
		ContentBox const& content_box,
		bool agg_size_changed, bool batch);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	std::shared_ptr<AcceleratableOperations> m_ptrAccelOps;
	IntrusivePtr<Settings> m_ptrSettings;
	PageId m_pageId;
	std::shared_ptr<AbstractImageTransform const> m_ptrOrigTransform;
	AffineTransformedImage m_affineTransformedImage;
	QImage m_downscaledImage;
	ContentBox m_contentBox;
	bool m_aggSizeChanged;
	bool m_batchProcessing;
};


Task::Task(IntrusivePtr<Filter> const& filter,
	IntrusivePtr<output::Task> const& next_task,
	IntrusivePtr<Settings> const& settings,
	PageId const& page_id, bool batch, bool debug)
:	m_ptrFilter(filter),
	m_ptrNextTask(next_task),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_batchProcessing(batch)
{
}

Task::~Task()
{
}

FilterResultPtr
Task::process(
	TaskStatus const& status,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& orig_image,
	CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
	std::shared_ptr<AbstractImageTransform const> const& orig_image_transform,
	boost::optional<AffineTransformedImage> pre_transformed_image,
	ContentBox const& content_box)
{
	status.throwIfCancelled();

	QSizeF agg_hard_size_before;
	QSizeF agg_hard_size_after;
	Params const params(
		m_ptrSettings->updateContentSizeAndGetParams(
			m_pageId, content_box.toTransformedRect(*orig_image_transform).size(),
			&agg_hard_size_before, &agg_hard_size_after
		)
	);

	if (m_ptrNextTask) {
		QRectF const unscaled_content_rect(
			content_box.toTransformedRect(*orig_image_transform)
		);

		std::shared_ptr<AbstractImageTransform> adjusted_transform(
			orig_image_transform->clone()
		);

		PageLayout const page_layout(
			unscaled_content_rect, agg_hard_size_after,
			params.matchSizeMode(), params.alignment(), params.hardMargins()
		);
		page_layout.absorbScalingIntoTransform(*adjusted_transform);

		return m_ptrNextTask->process(
			status, accel_ops, orig_image, gray_orig_image_factory, adjusted_transform,
			page_layout.innerRect(), page_layout.outerRect()
		);
	} else if (m_ptrFilter->optionsWidget()) {

		if (!pre_transformed_image)
		{
			pre_transformed_image = orig_image_transform->toAffine(
				orig_image, Qt::transparent, accel_ops
			);
		}

		return FilterResultPtr(
			new UiUpdater(
				m_ptrFilter, accel_ops, m_ptrSettings, m_pageId,
				orig_image_transform, *pre_transformed_image, content_box,
				agg_hard_size_before != agg_hard_size_after,
				m_batchProcessing
			)
		);
	} else {
		return FilterResultPtr();
	}
}


/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	IntrusivePtr<Settings> const& settings,
	PageId const& page_id,
	std::shared_ptr<AbstractImageTransform const> const& orig_transform,
	AffineTransformedImage const& affine_transformed_image,
	ContentBox const& content_box, bool agg_size_changed, bool batch)
:	m_ptrFilter(filter),
	m_ptrAccelOps(accel_ops),
	m_ptrSettings(settings),
	m_pageId(page_id),
	m_ptrOrigTransform(orig_transform),
	m_affineTransformedImage(affine_transformed_image),
	m_downscaledImage(
		ImageViewBase::createDownscaledImage(affine_transformed_image.origImage(), accel_ops)
	),
	m_contentBox(content_box),
	m_aggSizeChanged(agg_size_changed),
	m_batchProcessing(batch)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.

	bool const have_content_box = m_contentBox.isValid();
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();

	opt_widget->postUpdateUI(have_content_box);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);
	
	if (m_aggSizeChanged) {
		ui->invalidateAllThumbnails();
	} else {
		ui->invalidateThumbnail(m_pageId);
	}
	
	if (m_batchProcessing) {
		return;
	}
	
	ImageViewBase* view;

	if (!have_content_box) {
		view = new BasicImageView(m_ptrAccelOps, m_affineTransformedImage, m_downscaledImage);
	} else {
		view = new ImageView(
			m_ptrAccelOps, m_ptrSettings, m_pageId, m_ptrOrigTransform,
			m_affineTransformedImage, m_downscaledImage,
			m_contentBox, *opt_widget
		);
	}

	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
	
	if (have_content_box) {

		QObject::connect(
			view, SIGNAL(invalidateThumbnail(PageId const&)),
			opt_widget, SIGNAL(invalidateThumbnail(PageId const&))
		);
		QObject::connect(
			view, SIGNAL(invalidateAllThumbnails()),
			opt_widget, SIGNAL(invalidateAllThumbnails())
		);
		QObject::connect(
			view, SIGNAL(marginsSetLocally(RelativeMargins const&)),
			opt_widget, SLOT(marginsSetExternally(RelativeMargins const&))
		);
		QObject::connect(
			opt_widget, SIGNAL(marginsSetLocally(RelativeMargins const&)),
			view, SLOT(marginsSetExternally(RelativeMargins const&))
		);
		QObject::connect(
			opt_widget, SIGNAL(topBottomLinkToggled(bool)),
			view, SLOT(topBottomLinkToggled(bool))
		);
		QObject::connect(
			opt_widget, SIGNAL(leftRightLinkToggled(bool)),
			view, SLOT(leftRightLinkToggled(bool))
		);
		QObject::connect(
			opt_widget, SIGNAL(matchSizeModeChanged(MatchSizeMode const&)),
			view, SLOT(matchSizeModeChanged(MatchSizeMode const&))
		);
		QObject::connect(
			opt_widget, SIGNAL(alignmentChanged(Alignment const&)),
			view, SLOT(alignmentChanged(Alignment const&))
		);
		QObject::connect(
			opt_widget, SIGNAL(aggregateHardSizeChanged()),
			view, SLOT(aggregateHardSizeChanged())
		);
	}
}

} // namespace page_layout
