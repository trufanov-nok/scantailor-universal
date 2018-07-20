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
#include <QImage>
#include <iostream>
#include <QFileInfo>
#include "djview4/qdjvuwidget.h"

namespace publishing
{

class Task::UiUpdater : public FilterResult
{
public:
    UiUpdater(IntrusivePtr<Filter> const& filter, QString const& filename,
        bool batch_processing);
	
	virtual void updateUI(FilterUiInterface* wnd);
	
	virtual IntrusivePtr<AbstractFilter> filter() { return m_ptrFilter; }
private:
    QString const& m_filename;
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
Task::process(TaskStatus const& status, QString const& image_file, quint64 image_hash)
{
	// This function is executed from the worker thread.
	
	status.throwIfCancelled();

//    QFileInfo fi(image_file);
//    QString djv_name;
//    if (fi.exists()) {
//        djv_name = fi.completeBaseName() + ".djvu";
//    }
//    fi.setFile(djv_name);
//    if (!fi.exists()) {

//    }
	
    return FilterResultPtr(
                new UiUpdater(
                    m_ptrFilter, image_file, m_batchProcessing
                    )
                );
}


/*============================ Task::UiUpdater ========================*/

Task::UiUpdater::UiUpdater(
	IntrusivePtr<Filter> const& filter,
    QString const& filename,
	bool const batch_processing)
:	m_ptrFilter(filter),
    m_filename(filename),
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
	
    QDjVuWidget* widget = new QDjVuWidget();
    widget->setDocument(m_ptrFilter->getDjVuDocument());
    ui->setImageWidget(widget, ui->TRANSFER_OWNERSHIP);

    //ImageView* view = new ImageView(image, downscaled_image, xform);
    //ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP);
}

} // namespace publishing
