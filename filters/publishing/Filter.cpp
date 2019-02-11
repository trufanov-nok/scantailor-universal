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

#include "Filter.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "Settings.h"
#include "Task.h"
#include "CacheDrivenTask.h"
#include "PageId.h"
#include "ImageId.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#ifndef Q_MOC_RUN
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <QString>
#include <QObject>
#include <QCoreApplication>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <iostream>
#include <QMessageBox>
#include <QFileDialog>
#include <libdjvu/ddjvuapi.h>
#include "CommandLine.h"
#include "QMLLoader.h"
#include "StatusBarProvider.h"
#include "OrderByFileSize.h"

// required by resource system for static libs
// must be declared in default namespace
inline void initStaticLibResources() { Q_INIT_RESOURCE(qdjvuwidget); }

namespace publishing
{

enum DisplayMode {
  DISPLAY_COLOR,              //!< Default dislplay mode
  DISPLAY_STENCIL,            //!< Only display the b&w mask layer
  DISPLAY_BG,                 //!< Only display the background layer
  DISPLAY_FG,                 //!< Only display the foregroud layer
  DISPLAY_TEXT,               //!< Overprint the text layer
};

inline QDjVuWidget::DisplayMode idx2displayMode(int idx) {
    switch (idx) {
    case 0: return QDjVuWidget::DISPLAY_COLOR;
    case 1: return QDjVuWidget::DISPLAY_BG;
    case 2: return QDjVuWidget::DISPLAY_FG;
    case 3: return QDjVuWidget::DISPLAY_STENCIL;
    }
    return QDjVuWidget::DISPLAY_COLOR;
}

void Filter::setupImageViewer()
{
    QWidget* wgt = new QWidget();
    m_ptrImageViewer.reset(wgt);
    QVBoxLayout* lt = new QVBoxLayout(wgt);
    lt->setSpacing(0);

    QTabBar* tab = new QTabBar();
    tab->addTab(tr("Main"));
    tab->addTab(tr("Background"));
    tab->addTab(tr("Foregroud"));
    tab->addTab(tr("B&W Mask"));

    lt->addWidget(tab, 0, Qt::AlignTop);
    m_ptrDjVuWidget.reset( new QDjVuWidget() );
    lt->addWidget(m_ptrDjVuWidget.get(), 100);

    connect(tab, &QTabBar::currentChanged, [=](int idx) {
        m_ptrDjVuWidget->setDisplayMode(idx2displayMode(idx));
    });

}

Filter::Filter(IntrusivePtr<ProjectPages> const& pages,
               PageSelectionAccessor const& page_selection_accessor)
    :	m_ptrPages(pages), m_ptrSettings(new Settings), m_DjVuContext("scan_tailor_universal"), m_QMLLoader(nullptr),
      m_selectedPageOrder(0), m_suppressDjVuDisplay(false)
{
    initStaticLibResources();

    m_isGUI = CommandLine::get().isGui();

    if (m_isGUI) {

        setupImageViewer();

        m_ptrOptionsWidget.reset(
                    new OptionsWidget(m_ptrSettings, page_selection_accessor)
                    );
        // GUI thread owns loader
        m_QMLLoader = m_ptrOptionsWidget->getQMLLoader();
    } else {
        m_QMLLoader = new QMLLoader();
    }

    typedef PageOrderOption::ProviderPtr ProviderPtr;
    ProviderPtr const default_order;
    ProviderPtr const order_by_filesize(new OrderByFileSize(m_ptrSettings));
    m_pageOrderOptions.push_back(PageOrderOption(tr("Natural order"), default_order));
    m_pageOrderOptions.push_back(PageOrderOption(tr("Order by file size"), order_by_filesize,
                                                 tr("Orders the pages by the DjVu page file size")));

}

void
Filter::updateDjVuDocument(const QString& djvu_filename)
{
    int file_size = 0;
    QFileInfo info(djvu_filename);

    if (info.exists()) {
        file_size = (int)info.size();
    }

    StatusBarProvider::setFileSize(file_size);

    if (!m_suppressDjVuDisplay) {
        if (info.exists()) {

            ddjvu_cache_clear(m_DjVuContext);
            QDjVuDocument* doc = new QDjVuDocument(true);
            doc->setFileName(&m_DjVuContext, djvu_filename, false);

            if (!doc->isValid()) {
                delete doc;
                QMessageBox::critical(qApp->activeWindow(), tr("Cannot open file '%1'.").arg(djvu_filename), tr("Opening DjVu file"));
            } else {
                // technically takes doc ownership as it was created with autoDel==true
                connect(doc, &QDjVuDocument::error, [](QString err, QString fname, int line_no) {
                    qDebug() << err << fname << line_no;
                });
                m_ptrDjVuWidget->setDocument(doc);
                m_ptrImageViewer->show();
            }
        } else {
            m_ptrImageViewer->hide();
        }
    }

    emit m_ptrOptionsWidget->invalidateThumbnail(m_pageId);
}

void
Filter::setSuppressDjVuDisplay(bool val)
{
    bool update_display = !val && m_suppressDjVuDisplay;
    m_suppressDjVuDisplay = val;

    if (update_display) {
        Params param = m_ptrSettings->getParams(m_pageId);
        updateDjVuDocument(param.djvuFilename());
    }
}

Filter::~Filter()
{
    if (!m_isGUI) {
        // we own loader pointer
        delete m_QMLLoader;
    }
}

QString
Filter::getName() const
{
    return QCoreApplication::translate(
                "publishing::Filter", "Make a book"
                );
}

PageView
Filter::getView() const
{
    return PAGE_VIEW;
}

int
Filter::selectedPageOrder() const
{
    return m_selectedPageOrder;
}

void
Filter::selectPageOrder(int option)
{
    assert((unsigned)option < m_pageOrderOptions.size());
    m_selectedPageOrder = option;
}

std::vector<PageOrderOption>
Filter::pageOrderOptions() const
{
    return m_pageOrderOptions;
}

void
Filter::performRelinking(AbstractRelinker const& relinker)
{
    m_ptrSettings->performRelinking(relinker);
}

void
Filter::preUpdateUI(FilterUiInterface* ui, PageId const& page_id)
{
    if (m_ptrOptionsWidget.get()) {
        m_ptrOptionsWidget->preUpdateUI(page_id);
        ui->setOptionsWidget(m_ptrOptionsWidget.get(), ui->KEEP_OWNERSHIP);
    }
}

QDomElement
Filter::saveSettings(
        ProjectWriter const& writer, QDomDocument& doc) const
{
    QDomElement filter_el(doc.createElement("publishing"));
    writer.enumPages(
                boost::lambda::bind(
                    &Filter::writePageSettings,
                    this, boost::ref(doc), boost::lambda::var(filter_el), boost::lambda::_1, boost::lambda::_2
                    )
                );

    return filter_el;
}

void
Filter::loadSettings(ProjectReader const& reader, QDomElement const& filters_el)
{
    m_ptrSettings->clear();

    QDomElement filter_el(
                filters_el.namedItem("publishing").toElement()
                );
    QString const page_tag_name("page");

    QDomNode node(filter_el.firstChild());
    for (; !node.isNull(); node = node.nextSibling()) {
        if (!node.isElement()) {
            continue;
        }
        if (node.nodeName() != page_tag_name) {
            continue;
        }
        QDomElement const el(node.toElement());

        bool ok = true;
        int const id = el.attribute("id").toInt(&ok);
        if (!ok) {
            continue;
        }

        PageId const page_id(reader.pageId(id));
        if (page_id.isNull()) {
            continue;
        }

        QDomElement const params_el(el.namedItem("params").toElement());
        if (!params_el.isNull()) {
            Params const params(params_el);
            m_ptrSettings->setParams(page_id, params);
        }
    }

}

void
Filter::invalidateSetting(PageId const& page)
{
    Params p = m_ptrSettings->getParams(page);
    p.setForceReprocess(Params::RegenerateAll);
    m_ptrSettings->setParams(page, p);
}


IntrusivePtr<Task>
Filter::createTask(
        PageId const& page_id,
        bool const batch_processing)
{
    m_pageId = page_id;
    return IntrusivePtr<Task>(
                new Task(
                    IntrusivePtr<Filter>(this),
                    m_ptrSettings, page_id,
                    batch_processing
                    )
                );
}

IntrusivePtr<CacheDrivenTask>
Filter::createCacheDrivenTask()
{
    return IntrusivePtr<CacheDrivenTask>(
                new CacheDrivenTask(m_ptrSettings)
                );
}

void
Filter::writePageSettings(
        QDomDocument& doc, QDomElement& filter_el,
        PageId const& page_id, int numeric_id) const
{

    Params const params(m_ptrSettings->getParams(page_id));

    QDomElement page_el(doc.createElement("page"));
    page_el.setAttribute("id", numeric_id);
    page_el.appendChild(params.toXml(doc, "params"));

    filter_el.appendChild(page_el);
}

void
Filter::composeDjVuDocument(const QString& target_fname)
{
    QStringList done;
    QStringList missing;
    if (m_ptrSettings->allDjVusGenerated(done, missing)) {
        QFileDialog dlg(qApp->activeWindow(), tr("Create multipage DjVu document"),
                        target_fname, tr("DjVu documents (*.djvu;*.djv);All files (*.*)"));
        dlg.setDefaultSuffix(".djvu");
        if (dlg.exec() == QDialog::Accepted) {
            QString cmd("djvum %1 %2");
            cmd = cmd.arg(dlg.selectedFiles()[0]).arg(done.join(" "));
//            QSingleShotExec* executer = new QSingleShotExec(QStringList(cmd));
//            executer->start();
        }

    } else {
        while (missing.count() > 10) {
            missing.removeLast();
        }
        QMessageBox::critical(qApp->activeWindow(), tr("Compose DjVu document"),
                              tr("Folowing pages aren't yet encoded to DjVu format:/n%1").arg(missing.join("/n")));
    }
}

} // namespace publishing
