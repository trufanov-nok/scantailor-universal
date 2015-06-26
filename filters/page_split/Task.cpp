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
#include "TaskStatus.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "LayoutType.h"
#include "ProjectPages.h"
#include "PageInfo.h"
#include "PageId.h"
#include "PageLayoutEstimator.h"
#include "PageLayout.h"
#include "Dependencies.h"
#include "Params.h"
#include "ImageMetadata.h"
#include "AffineTransformedImage.h"
#include "OrthogonalRotation.h"
#include "ImageView.h"
#include "FilterUiInterface.h"
#include "DebugImages.h"
#include "imageproc/GrayImage.h"
#include "filters/deskew/Task.h"
#include <QImage>
#include <QObject>
#include <QDebug>
#include <memory>
#include <assert.h>

namespace page_split
{

using imageproc::BinaryThreshold;

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		IntrusivePtr<ProjectPages> const& pages,
		std::auto_ptr<DebugImages> dbg_img,
		AffineTransformedImage const& full_size_image,
		PageInfo const& page_info,
		Params const& params,
		bool layout_type_auto_detected,
		bool batch_processing);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;
	IntrusivePtr<ProjectPages> m_ptrPages;
	std::auto_ptr<DebugImages> m_ptrDbg;
	AffineTransformedImage m_fullSizeImage;
	PageInfo m_pageInfo;
	Params m_params;
	bool m_layoutTypeAutoDetected;
	bool m_batchProcessing;
};

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

Task::Task(
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<ProjectPages> const& pages,
	IntrusivePtr<deskew::Task> const& next_task,
	PageInfo const& page_info,
	bool const batch_processing, bool const debug)
:	m_ptrFilter(filter),
	m_ptrSettings(settings),
	m_ptrPages(pages),
	m_ptrNextTask(next_task),
	m_pageInfo(page_info),
	m_batchProcessing(batch_processing)
{
	if (debug) {
		m_ptrDbg.reset(new DebugImages);
	}
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
	AffineImageTransform const& orig_image_transform,
	OrthogonalRotation const& rotation)
{
	status.throwIfCancelled();
	
	Settings::Record record(m_ptrSettings->getPageRecord(m_pageInfo.imageId()));
	
	Dependencies const deps(
		orig_image.size(), rotation, record.combinedLayoutType()
	);

	for (;;) {
		Params const* const params = record.params();
		
		PageLayout new_layout;
		
		if (!params || !deps.compatibleWith(*params)) {
			new_layout = PageLayoutEstimator::estimatePageLayout(
				record.combinedLayoutType(),
				AffineTransformedImage(gray_orig_image_factory(), orig_image_transform),
				m_ptrDbg.get()
			);
			status.throwIfCancelled();
		} else if (params->pageLayout().uncutOutline().isEmpty()) {
			// Backwards compatibility with versions < 0.9.9
			new_layout = params->pageLayout();
			new_layout.setUncutOutline(orig_image_transform.transformedCropArea().boundingRect());
		} else {
			break;
		}
			
		Params const new_params(new_layout, deps, MODE_AUTO);
		Settings::UpdateAction update;
		update.setParams(new_params);

#ifndef NDEBUG
		{
			Settings::Record updated_record(record);
			updated_record.update(update);
			assert(!updated_record.hasLayoutTypeConflict());
			// This assert effectively verifies that PageLayoutEstimator::estimatePageLayout()
			// returned a layout of a type consistent with the requested one.
			// If it didn't, it's a bug which will in fact cause a dead loop.
		}
#endif

		bool conflict = false;
		record = m_ptrSettings->conditionalUpdate(
			m_pageInfo.imageId(), update, &conflict
		);
		if (conflict && !record.params()) {
			// If there was a conflict, it means
			// the record was updated by another
			// thread somewhere between getPageRecord()
			// and conditionalUpdate().  If that
			// external update didn't leave page
			// parameters clear, we are just going
			// to use it's data, otherwise we need
			// to process this page again for the
			// new layout type.
			continue;
		}
		
		break;
	}
	
	PageLayout const& layout = record.params()->pageLayout();
	m_ptrPages->setLayoutTypeFor(m_pageInfo.imageId(), toPageLayoutType(layout));
	
	if (m_ptrNextTask) {
		AffineImageTransform cropping_transform(orig_image_transform);
		cropping_transform.setOrigCropArea(
			orig_image_transform.transform().inverted().map(
				layout.pageOutline(m_pageInfo.id().subPage())
			)
		);
		return m_ptrNextTask->process(
			status, accel_ops, orig_image, gray_orig_image_factory,
			cropping_transform, rotation
		);
	} else {
		return FilterResultPtr(
			new UiUpdater(
				m_ptrFilter, m_ptrPages, m_ptrDbg,
				AffineTransformedImage(orig_image, orig_image_transform),
				m_pageInfo, *record.params(),
				record.combinedLayoutType() == AUTO_LAYOUT_TYPE,
				m_batchProcessing
			)
		);
	}
}


/*============================ Task::UiUpdater =========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<ProjectPages> const& pages,
	std::auto_ptr<DebugImages> dbg_img,
	AffineTransformedImage const& full_size_image,
	PageInfo const& page_info,
	Params const& params,
	bool const layout_type_auto_detected,
	bool const batch_processing)
:	m_ptrFilter(filter),
	m_ptrPages(pages),
	m_ptrDbg(dbg_img),
	m_fullSizeImage(full_size_image),
	m_pageInfo(page_info),
	m_params(params),
	m_layoutTypeAutoDetected(layout_type_auto_detected),
	m_batchProcessing(batch_processing)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.
	
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI(m_params, m_layoutTypeAutoDetected);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);
	
	ui->invalidateThumbnail(m_pageInfo.id());
	
	if (m_batchProcessing) {
		return;
	}
	
	ImageView* view = new ImageView(
		m_fullSizeImage, m_params.pageLayout(), m_ptrPages, m_pageInfo.imageId(),
		m_pageInfo.leftHalfRemoved(), m_pageInfo.rightHalfRemoved()
	);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_ptrDbg.get());
	
	QObject::connect(
		view, SIGNAL(invalidateThumbnail(PageInfo const&)),
		opt_widget, SIGNAL(invalidateThumbnail(PageInfo const&))
	);
	QObject::connect(
		view, SIGNAL(pageLayoutSetLocally(PageLayout const&)),
		opt_widget, SLOT(pageLayoutSetExternally(PageLayout const&))
	);
	QObject::connect(
		opt_widget, SIGNAL(pageLayoutSetLocally(PageLayout const&)),
		view, SLOT(pageLayoutSetExternally(PageLayout const&))
	);
}

} // namespace page_split
