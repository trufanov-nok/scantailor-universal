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

#include "CommandLine.h"
#include "Task.h"
#include "Filter.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "FilterData.h"
#include "ImageTransformation.h"
#include "TaskStatus.h"
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
    UiUpdater(IntrusivePtr<Filter> const& filter, PageId const& page_id,
              bool batch_processing);

    virtual void updateUI(FilterUiInterface* wnd);

    virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
    IntrusivePtr<Filter> m_ptrFilter;
    PageId m_pageId;
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
    connect(this, &Task::displayDjVu, m_ptrFilter.get(), &Filter::updateDjVuDocument); //must be done via slots as both objects are in diff threads
}

Task::~Task()
{
}

FilterResultPtr
Task::process(TaskStatus const& status, QString const& image_file, const QImage& out_image)
{
    // This function is executed from the worker thread.    
    status.throwIfCancelled();

    m_ptrFilter->imageViewer()->hide();
    m_ptrFilter->getDjVuWidget()->setDocument(nullptr);

    Params param = m_ptrSettings->getParams(m_pageId);

    Params::Regenerate val = param.getForceReprocess();
    bool need_reprocess = val & Params::RegeneratePage;
    if (need_reprocess) {
        val = (Params::Regenerate) (val & ~Params::RegeneratePage);
        param.setForceReprocess(val);
        m_ptrSettings->setParams(m_pageId, param);
    }

    publishing::ImageInfo info;
    if (param.isNull()) {
        info = publishing::ImageInfo(image_file, out_image);
        param.setImageInfo(info);
    } else {
        info = param.imageInfo();
    }

    if (param.commandToExecute().isEmpty()) {
        // need default commands
        QMLLoader::CachedState cached_state =  m_ptrFilter->getQMLLoader()->getDefaultCommand(param.inputImageColorMode());
        param.setEncoderId(cached_state.encoder_id);
        param.setEncoderState(cached_state.encoder_state);
        param.setConverterState(cached_state.converter_state);
        param.setCommandToExecute(cached_state.commands.join('\n'));
        need_reprocess = true;
    }

    if (!need_reprocess) {
        need_reprocess = param.commandToExecute() != param.executedCommand();
    }
    if (!need_reprocess) {
        need_reprocess = image_file != param.imageFilename() || info.imageHash != param.inputImageHash();
    }
    if (!need_reprocess) {
        need_reprocess = !QFile::exists(param.djvuFilename());
    }



    if (need_reprocess) {

        m_ptrSettings->setParams(m_pageId, param);

        DjVuPageGenerator& generator = m_ptrFilter->getPageGenerator();
        generator.setInputImageFile(image_file, info.imageHash);
        generator.setOutputFile(param.djvuFilename());

        generator.setComands(param.commandToExecute().split('\n', QString::SkipEmptyParts));
        if (!generator.execute()) {
            QMessageBox::warning(nullptr, QObject::tr("DjVu creation"), QObject::tr("Can't create output folder for %1").arg(param.djvuFilename()));
        }
    } else {        
        emit displayDjVu();  // need to be done via slots as djvuwidget belongs to another thread
    }

    if (!CommandLine::get().isGui()) {
        return FilterResultPtr(nullptr);
    }

    return FilterResultPtr(
                new UiUpdater(
                    m_ptrFilter, m_pageId, m_batchProcessing
                    )
                );
}


/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(
        IntrusivePtr<Filter> const& filter,
        PageId const& page_id,
        bool const batch_processing)
    :	m_ptrFilter(filter), m_pageId(page_id),
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

    m_ptrFilter->setSuppressDjVuDisplay(m_batchProcessing);

    ui->invalidateThumbnail(m_pageId);

    if (m_batchProcessing) {
        return;
    }

    ui->setImageWidget(m_ptrFilter->imageViewer(), ui->KEEP_OWNERSHIP);
}

} // namespace publishing
