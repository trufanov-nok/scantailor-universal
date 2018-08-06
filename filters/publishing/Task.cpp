/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2018 Alexander Trufanov <trufanovan@gmail.com>

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
#include "TaskStatus.h"
#include "ImageView.h"
#include "FilterUiInterface.h"
#include "DjVuPageGenerator.h"
#include <QImage>
#include <iostream>
#include <QFileInfo>
#include "djview4/qdjvuwidget.h"

namespace publishing
{

class Task::UiUpdater : public FilterResult
{
public:
    UiUpdater(IntrusivePtr<Filter> const& filter, /*QString const& filename,*/
              bool batch_processing);

    virtual void updateUI(FilterUiInterface* wnd);

    virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
    IntrusivePtr<Filter> m_ptrFilter;
    bool m_batchProcessing;
};


Task::Task(
        IntrusivePtr<Filter> const& filter,
        IntrusivePtr<Settings> const& settings,
        PageId const& page_id,
        bool const batch_processing)
    :	m_ptrFilter(filter),
      m_ptrSettings(settings),
      m_pageId(page_id),
      m_batchProcessing(batch_processing)
{
}

Task::~Task()
{
}

FilterResultPtr
Task::process(TaskStatus const& status, QString const& image_file, qint64 image_hash)
{
    // This function is executed from the worker thread.

    status.throwIfCancelled();
    Params param = m_ptrSettings->getParams(m_pageId);
    if (param.inNull()) {
        QImage img;
        if (img.load(image_file)) {
            publishing::ImageInfo info(image_file, img);
            param.setImageInfo(info);
        } else {
            std::cerr << "Can't load image file " << image_file.toStdString().c_str() << "\n";
        }
    }

    if (param.executedCommand().isEmpty()) {
        // need default commands

    }

    m_ptrSettings->setParams(m_pageId, param);

    if (image_file != param.inputFilename() ||
            image_hash != param.inputImageHash() ||
            param.getForceReprocess() & RegenParams::RegenParams::RegeneratePage) {
        // djvu file recreation is needed
        DjVuPageGenerator& generator = m_ptrFilter->getPageGenerator();
        generator.setFilename(param.inputFilename());
        generator.setComands(param.executedCommand().split('\n', QString::SkipEmptyParts));
        generator.execute();
    }

    return FilterResultPtr(
                new UiUpdater(
                    m_ptrFilter, m_batchProcessing
                    )
                );
}


/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(
        IntrusivePtr<Filter> const& filter,
        bool const batch_processing)
    :	m_ptrFilter(filter),
      m_batchProcessing(batch_processing)
{
}

void
Task::UiUpdater::updateUI(FilterUiInterface* ui)
{
    // This function is executed from the GUI thread.
    OptionsWidget* const opt_widget = m_ptrFilter->optionsWidget();
    opt_widget->postUpdateUI();
    ui->setOptionsWidget(opt_widget, ui->KEEP_OWNERSHIP);

    //    ui->invalidateThumbnail(m_filename);

    if (m_batchProcessing) {
        return;
    }

    QDjVuWidget* const widget = m_ptrFilter->getDjVuWidget();
    ui->setImageWidget(widget, ui->KEEP_OWNERSHIP);

    //ImageView* view = new ImageView(image, downscaled_image, xform);
    //ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
}

} // namespace publishing
