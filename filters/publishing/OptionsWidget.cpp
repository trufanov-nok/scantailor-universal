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

#include "OptionsWidget.h"
#include "OptionsWidget.moc"
#include "Filter.h"
#include "ApplyToDialog.h"
#include "Settings.h"

#include "MissingApplicationsWidget.h"
#include "../../Utils.h"
#include "ChangeDpiDialog.h"
#include "QuickWidgetAccessHelper.h"
#include <QFileInfo>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDir>
#include <iostream>

namespace publishing
{

OptionsWidget::OptionsWidget(
        IntrusivePtr<Settings> const& settings,
        PageSelectionAccessor const& page_selection_accessor,
        DjVuPageGenerator& pageGenerator)
    :	m_ptrSettings(settings),
      m_pageSelectionAccessor(page_selection_accessor),
      m_pageGenerator(pageGenerator)
{
    setupUi(this);

    connect(&m_pageGenerator, &DjVuPageGenerator::executionComplete, [this](){
        emit displayDjVu(m_pageGenerator.outputFileName());
    });

    setupQuickWidget(conversionWidget);
    setupQuickWidget(encoderWidget);

    cbEncodersSelector->clear();

    QObject::connect(&m_QMLLoader, &QMLLoader::dependencyStateChanged, this, [this]() {
        displayWidgets();
    });



    for (const DjVuEncoder* enc: m_QMLLoader.availableEncoders()) {
        cbEncodersSelector->addItem(enc->m_name, enc->filename);
    }

}

OptionsWidget::~OptionsWidget()
{
}

void
updateQuickWidgetHeight(QQuickWidget* w)
{
    if (w) {
        QQuickItem* obj = w->rootObject();
        if (obj && obj->height() > 0.) {
            w->setFixedHeight((int)obj->height());
            w->updateGeometry();
        }
    }
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
    m_pageId = page_id;
    Params const params(m_ptrSettings->getParams(m_pageId));

    QString dpi_label = tr("%1 dpi").arg(QString::number(params.outputDpi().horizontal()));
    dpiValue->setText(Utils::richTextForLink(dpi_label));
}

void
OptionsWidget::launchDjVuGenerator()
{
    if (!m_imageFilename.isNull()) {
        m_pageGenerator.setComands(m_QMLLoader.getCommands());
        m_pageGenerator.execute();
    }
}

void
OptionsWidget::postUpdateUI()
{
    Params const params(m_ptrSettings->getParams(m_pageId));
    m_imageFilename = params.inputFilename();
    m_pageGenerator.setFilename(m_imageFilename);

    displayWidgets();

    m_QMLLoader.converter()->setState(params.converterState());
    m_QMLLoader.encoder()->setState(params.encoderState());
//    QuickWidgetAccessHelper c_help(conversionWidget);
//    QuickWidgetAccessHelper e_help(encoderWidget);

    launchDjVuGenerator();
}

void
OptionsWidget::setupQuickWidget(QQuickWidget* wgt)
{
    wgt->setAttribute(Qt::WA_AlwaysStackOnTop);
    wgt->setAttribute(Qt::WA_TranslucentBackground);
    wgt->setClearColor(Qt::transparent);
    QObject::connect(wgt, &QQuickWidget::statusChanged, this, &OptionsWidget::on_statusChanged);
    setLinkToMainApp(wgt);
}


void
OptionsWidget::on_statusChanged(const QQuickWidget::Status &arg1)
{
    if (arg1 == QQuickWidget::Ready) {
        QQuickWidget* w = static_cast<QQuickWidget*>(sender());
        updateQuickWidgetHeight(w);
    }
}

void
OptionsWidget::resizeQuickWidget(QVariant arg)
{
    // QTBUG-69566
    QQuickWidget* wgt = (arg.toString() == "encoder")? encoderWidget
                                                     : conversionWidget;
    if (QQuickItem* qi = wgt->rootObject()) {
        wgt->setFixedHeight((int) qi->height());
    }

    updateGeometry();
}

void
OptionsWidget::setLinkToMainApp(QQuickWidget* wgt) {
    if (QQmlEngine* eng = wgt->engine()) {
        if (QQmlContext* cntx = eng->rootContext()) {
            cntx->setContextProperty("mainApp", this);
        }
    }
}

bool OptionsWidget::isAllDependenciesFound(const QStringList& req_apps, QStringList* missing_apps)
{
    bool all_is_ok = true;
    for (const QString& app_name : req_apps) {
        if (m_QMLLoader.dependencies()[app_name].state != AppDependencyState::Found) {
            all_is_ok = false;
            if (missing_apps) {
                missing_apps->append(app_name);
            } else {
                break;
            }
        }
    }
    return all_is_ok;
}


void
OptionsWidget::showMissingAppSelectors(QStringList req_apps, bool isEncoderDep)
{
    QWidget* wgt = (isEncoderDep)? wgtMissingDeps4Encoder : wgtMissingDeps4Converter;
    QList<MissingApplicationsWidget *> childWidgets = wgt->findChildren<MissingApplicationsWidget *>(QString(), Qt::FindDirectChildrenOnly);
    int child_cnt = childWidgets.size();

    // Clear missing apps already found
    for (MissingApplicationsWidget * mw : childWidgets) {
        const QString req_app_shown = mw->getApplication();
        if (!req_apps.contains(req_app_shown)) {
            mw->setVisible(false);
            mw->deleteLater();
            child_cnt--;
        } else {
            req_apps.removeAll(req_app_shown);
        }
    }

    // Show missing apps selection dialogs
    for (const QString& app: req_apps) {
        MissingApplicationsWidget * mw = new MissingApplicationsWidget(app, m_QMLLoader.dependencies()[app].missing_app_hint, wgt);
        wgt->layout()->addWidget(mw);
        connect(mw, &MissingApplicationsWidget::newPathToExecutable, [this](QString app, QString filename) {
            AppDependency& dep = m_QMLLoader.dependencies()[app];
            dep.working_cmd = filename;
            checkDependencies(dep);
        });
        child_cnt++;
    }

    wgt->setVisible(child_cnt > 0);
    //    wgt->updateGeometry();
}

void OptionsWidget::displayWidgets()
{
    QStringList missing_deps;
    if (m_QMLLoader.encoder()) {
        encoderWidget->setVisible(isAllDependenciesFound(m_QMLLoader.encoder()->requiredApps(), &missing_deps));
        showMissingAppSelectors(missing_deps, true);
    }

    if (m_QMLLoader.converter()) {
        conversionWidget->setVisible(isAllDependenciesFound(m_QMLLoader.converter()->requiredApps(), &missing_deps));
        showMissingAppSelectors(missing_deps, false);
    }
}

void OptionsWidget::requestParamUpdate(const QVariant requestor)
{
    bool is_encoder = requestor.toString() == "encoder";
    QQuickWidget* wgt = (is_encoder)? encoderWidget : conversionWidget;
    QuickWidgetAccessHelper helper(wgt);
    QString& cmd = (is_encoder)? m_encoderCmd : m_convertorCmd;
    if (helper.getCommand(cmd)) {
        launchDjVuGenerator();
    }

    QVariantMap val;
    if (helper.getState(val)) {
        m_ptrSettings->setState(m_pageId, val, is_encoder);
    }

}

} // namespace publishing


void publishing::OptionsWidget::on_cbEncodersSelector_currentIndexChanged(int index)
{
    const QString qml_file = cbEncodersSelector->itemData(index).toString();
    encoderWidget->setResizeMode(QQuickWidget::ResizeMode::SizeViewToRootObject);
    //    setLinkToMainApp(encoderWidget);
    encoderWidget->setSource(qml_file);

    // trick to achieve qml contents actual height but resizable panel's width
    encoderWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);

    QuickWidgetAccessHelper eh(encoderWidget, qml_file);
    QString supported_input; QString preferred_input;
    eh.getSupportedInput(supported_input);
    eh.getPrefferedInput(preferred_input);

    QuickWidgetAccessHelper ch(conversionWidget, _convertor_qml_);
    bool res = ch.filterByRequiredInput(supported_input, preferred_input);
    Q_ASSERT(res);
}

void publishing::OptionsWidget::on_dpiValue_linkActivated(const QString &/*link*/)
{

    Dpi dpi(600,600);
    ChangeDpiDialog dlg(this, dpi);
    if (dlg.exec() == QDialog::Accepted) {
        m_ptrSettings->setDpi(m_pageId, dpi);
        QuickWidgetAccessHelper(encoderWidget).setOutputDpi(dpi.horizontal());
    };
}
