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
#include "FilterData.h"
#include "ImageTransformation.h"
#include "filters/page_split/Task.h"
#include "TaskStatus.h"
#include "ImageView.h"
#include "FilterUiInterface.h"
#include "imageproc/OrientationDetector.h"
#include "ImageLoader.h"
#include <QImage>
#include <iostream>

namespace fix_orientation
{

using imageproc::BinaryThreshold;

class Task::UiUpdater : public FilterResult
{
public:
    UiUpdater(IntrusivePtr<Filter> const& filter,
              QImage const& image, ImageId const& image_id,
              ImageTransformation const& xform,
              bool batch_processing);

    virtual void updateUI(FilterUiInterface* wnd);

    virtual IntrusivePtr<AbstractFilter> filter()
    {
        return m_ptrFilter;
    }
private:
    IntrusivePtr<Filter> m_ptrFilter;
    QImage m_image;
    QImage m_downscaledImage;
    ImageId m_imageId;
    ImageTransformation m_xform;
    bool m_batchProcessing;
};

Task::Task(
    ImageId const& image_id,
    IntrusivePtr<Filter> const& filter,
    IntrusivePtr<Settings> const& settings,
    IntrusivePtr<page_split::Task> const& next_task,
    bool const batch_processing)
    :   m_ptrFilter(filter),
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
Task::process(TaskStatus const& status, FilterData const& data)
{
    // This function is executed from the worker thread.

    status.throwIfCancelled();

    // The automatic rotation runs here, so that the stage itself finds the rotation of each requested scan.
    if (m_ptrSettings->autoState(m_imageId) == Settings::AUTO_PENDING) {
        using imageproc::OrientationDetector;
        imageproc::OrientationResult result;
        if (!m_ptrSettings->cachedDetection(m_imageId, &result)) {
            // The file image has the same DPI as in the look-ahead and the CLI, and the detector scale depends on it.
            QImage const file_image(ImageLoader::load(m_imageId));
            result = OrientationDetector::detect(
                file_image.isNull() ? data.origImage() : file_image,
                OrientationDetector::exifRotation(m_imageId.filePath())
            );
            m_ptrSettings->cacheDetection(m_imageId, result);
        }
        status.throwIfCancelled();
        // Only a batch looks ahead, so that the result does not depend on the page order and an open page does not wait.
        if (m_batchProcessing && !m_ptrSettings->hasBookRotation()) {
            lookAheadForBookRotation(status);
        }
        m_ptrSettings->applyAutoResult(m_imageId, result);
    }

    ImageTransformation xform(data.xform());
    xform.setPreRotation(m_ptrSettings->getRotationFor(m_imageId));

    if (m_ptrNextTask) {
        return m_ptrNextTask->process(status, FilterData(data, xform));
    } else {
        return FilterResultPtr(
                   new UiUpdater(
                       m_ptrFilter, data.origImage(), m_imageId, xform,
                       m_batchProcessing
                   )
               );
    }
}

void
Task::lookAheadForBookRotation(TaskStatus const& status)
{
    using imageproc::OrientationDetector;
    // A scan without text, such as a cover, needs the rotation of the book, so other requested scans give their votes first.
    std::vector<ImageId> const pending(m_ptrSettings->pendingImages());
    int checked = 0;
    for (ImageId const& image_id : pending) {
        if (m_ptrSettings->hasBookRotation() || checked >= 30) {
            break;
        }
        status.throwIfCancelled();
        QImage const image(ImageLoader::load(image_id));
        if (image.isNull()) {
            continue;
        }
        m_ptrSettings->cacheDetection(
            image_id, OrientationDetector::detect(image, OrientationDetector::exifRotation(image_id.filePath()))
        );
        ++checked;
    }
}

/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(
    IntrusivePtr<Filter> const& filter,
    QImage const& image, ImageId const& image_id,
    ImageTransformation const& xform,
    bool const batch_processing)
    :   m_ptrFilter(filter),
        m_image(image),
        m_downscaledImage(ImageView::createDownscaledImage(image)),
        m_imageId(image_id),
        m_xform(xform),
        m_batchProcessing(batch_processing)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
    // This function is executed from the GUI thread.
    OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
    if (!m_batchProcessing) {
        opt_widget->postUpdateUI(m_xform.preRotation());
    }
    ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

    ui->invalidateThumbnail(PageId(m_imageId));

    if (m_batchProcessing) {
        return;
    }

    ImageView* view = new ImageView(m_image, m_downscaledImage, m_xform);
    ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
    QObject::connect(
        opt_widget, SIGNAL(rotated(OrthogonalRotation)),
        view, SLOT(setPreRotation(OrthogonalRotation))
    );
}

} // namespace fix_orientation
