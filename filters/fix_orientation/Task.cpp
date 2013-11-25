/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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
#include "AffineImageTransform.h"
#include "AffineTransformedImage.h"
#include "filters/page_split/Task.h"
#include "TaskStatus.h"
#include "ImageView.h"
#include "FilterUiInterface.h"
#include <QSize>
#include <iostream>

namespace fix_orientation
{

class Task::UiUpdater : public FilterResult
{
public:
	UiUpdater(IntrusivePtr<Filter> const& filter,
		AffineTransformedImage const& transformed_image,
		OrthogonalRotation const& rotation,
		ImageId const& image_id, bool batch_processing);
	
	virtual void updateUI(FilterUiInterface* wnd);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
	IntrusivePtr<Filter> m_ptrFilter;

	/**
	 * The transformation associated with m_transformedImage doesn't
	 * incorporate m_rotation.
	 */
	AffineTransformedImage m_transformedImage;

	OrthogonalRotation m_rotation;
	ImageId m_imageId;
	bool m_batchProcessing;
};


Task::Task(
	ImageId const& image_id,
	IntrusivePtr<Filter> const& filter,
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<page_split::Task> const& next_task,
	bool const batch_processing)
:	m_ptrFilter(filter),
	m_ptrNextTask(next_task),
	m_ptrSettings(settings),
	m_imageId(image_id),
	m_batchProcessing(batch_processing)
{
}

Task::~Task()
{
}

FilterResultPtr
Task::process(
	TaskStatus const& status, QImage const& orig_image,
	CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
	AffineImageTransform const& orig_image_transform)
{
	// This function is executed from the worker thread.
	
	status.throwIfCancelled();
	
	OrthogonalRotation const rotation(m_ptrSettings->getRotationFor(m_imageId));

	if (m_ptrNextTask) {
		AffineImageTransform const rotated_transform = orig_image_transform.adjusted(
			[rotation](AffineImageTransform& xform) {
				xform.rotate(rotation.toDegrees());
			}
		);
		return m_ptrNextTask->process(
			status, orig_image, gray_orig_image_factory,
			rotated_transform, rotation
		);
	} else {
		return FilterResultPtr(
			new UiUpdater(
				m_ptrFilter, AffineTransformedImage(orig_image, orig_image_transform),
				rotation, m_imageId, m_batchProcessing
			)
		);
	}
}


/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
	AffineTransformedImage const& transformed_image,
	OrthogonalRotation const& rotation,
	ImageId const& image_id,
	bool const batch_processing)
:	m_ptrFilter(filter),
	m_transformedImage(transformed_image),
	m_rotation(rotation),
	m_imageId(image_id),
	m_batchProcessing(batch_processing)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
	// This function is executed from the GUI thread.
	OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
	opt_widget->postUpdateUI(m_rotation);
	ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);
	
	ui->invalidateThumbnail(PageId(m_imageId));
	
	if (m_batchProcessing) {
		return;
	}
	
	ImageView* view = new ImageView(m_transformedImage);
	ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
	QObject::connect(
		opt_widget, SIGNAL(rotated(OrthogonalRotation)),
		view, SLOT(setPreRotation(OrthogonalRotation))
	);
}

} // namespace fix_orientation
