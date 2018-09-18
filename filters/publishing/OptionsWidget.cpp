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
#include <QtQuickControls2/QQuickStyle>
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
      m_QMLLoader(this),
      m_pageGenerator(pageGenerator)
{
    QQuickStyle::setStyle("Material");

    setupUi(this);

    connect(&m_QMLLoader, &QMLLoader::dependencyStateChanged, this, [this]() {
        displayWidgets();
    });

    for (const DjVuEncoder* enc: m_QMLLoader.encoders()->allEncoders()) {
        cbEncodersSelector->addItem(enc->name, enc->filename);
    }

    setupQuickWidget(conversionWidget, m_QMLLoader.converter()->component(), m_QMLLoader.converter()->instance());

}

void deleteQQuickWidget(QQmlEngine* engine, QQuickWidget* wgt)
{
    // setContent(nullptr) is required before destruction
    // as widget owns these objects and tries to destroy assigned component
    // but this triggers warnings in std::cerr so we replace data with dummy objects' pointers instead of nullptr

    wgt->setVisible(false);
    QQmlComponent* cmpt = new QQmlComponent(engine);
    cmpt->setData(QByteArray("import QtQuick 2.0; Item { }"), QUrl());
    wgt->setContent(cmpt->url(), cmpt, cmpt->create());
    //wgt->setContent(QUrl(), nullptr, nullptr);
    wgt->deleteLater();
}

OptionsWidget::~OptionsWidget()
{
    // destroy QQuickWidgets manually without its content
    // as content will be destroyed in QMLLOader
    deleteQQuickWidget(m_QMLLoader.engine(), conversionWidget);
    deleteQQuickWidget(m_QMLLoader.engine(), encoderWidget);
}

void
updateQuickWidgetHeight(QQuickWidget* w)
{
    if (w) {
        QQuickItem* obj = w->rootObject();
        if (obj && obj->height() > 0.) {
            int h = (int)obj->height()+200;
            w->setFixedHeight(h);
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
    m_QMLLoader.encoders()->setState(params.encoderState());
    //    QuickWidgetAccessHelper c_help(conversionWidget);
    //    QuickWidgetAccessHelper e_help(encoderWidget);

    launchDjVuGenerator();
}

void
OptionsWidget::setupQuickWidget(QQuickWidget* &wgt, QQmlComponent* component, QObject* instance)
{
    if (component && component->isReady()) {
        if (instance && (!wgt || instance != wgt->rootObject())) {

            int prev_idx = 0;
            if (wgt) {
                prev_idx = verticalLayout_4->indexOf(wgt);
                deleteQQuickWidget(m_QMLLoader.engine(), wgt);
            }
            wgt = new QQuickWidget(m_QMLLoader.engine(), this);
            wgt->setAttribute(Qt::WA_AlwaysStackOnTop);
            wgt->setAttribute(Qt::WA_TranslucentBackground);
            wgt->setClearColor(Qt::transparent);

            // trick to set qml contents actual height but resizable panel's width
            wgt->setResizeMode(QQuickWidget::ResizeMode::SizeViewToRootObject);
            wgt->setContent(component->url(), component, instance);
            wgt->setFixedHeight(wgt->rootObject()->height());
            wgt->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);

            verticalLayout_4->insertWidget(prev_idx, wgt);

            qDebug() << "set: " << component->url();
            //            updateGeometry();

            QObject::connect(wgt, &QQuickWidget::statusChanged, this, &OptionsWidget::on_statusChanged);
        }
    }

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
    if (!m_pageId.isNull()) {
        QQuickWidget* wgt = (arg.toString() == "encoder")? encoderWidget
                                                         : conversionWidget;
        if (wgt /*&& wgt->engine()*/) {
            if (QQuickItem* qi = wgt->rootObject()) {
                const int hght = (int) qi->height();
                //                wgt->setMinimumSize(10, hght);
                //                wgt->setMaximumSize(5000, hght);
                wgt->setFixedHeight(hght);
                //                if (wgt->resizeMode() != QQuickWidget::ResizeMode::SizeRootObjectToView) {
                ////                    wgt->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
                ////                     wgt->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
                ////                }
                //                wgt->updateGeometry();
                //            }

                updateGeometry();
            }
        }
    }
}

bool
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
        MissingApplicationsWidget * mw = new MissingApplicationsWidget(app, m_QMLLoader.dependencyManager().dependency(app).missing_app_hint, wgt);
        wgt->layout()->addWidget(mw);
        connect(mw, &MissingApplicationsWidget::newPathToExecutable, [this](QString app, QString filename) {
            DependencyManager& dm = m_QMLLoader.dependencyManager();
            AppDependency dep = dm.dependency(app);
            dep.working_cmd = filename;
            dm.addDependency(dep);
            dm.checkDependencies(dep);
        });
        child_cnt++;
    }

    wgt->setVisible(child_cnt > 0);
    return wgt->isVisible();
}

void OptionsWidget::displayWidgets()
{
    QStringList missing_deps;
    if (m_QMLLoader.encoders()) {
        bool shown = m_QMLLoader.dependencyManager().isDependenciesFound(m_QMLLoader.encoders()->requiredApps(), &missing_deps);
        if (shown) {
            setupQuickWidget(encoderWidget, m_QMLLoader.encoders()->component(), m_QMLLoader.encoders()->instance());
        }
        encoderWidget->setVisible(shown);
        showMissingAppSelectors(missing_deps, true);
    }

    if (m_QMLLoader.converter()) {
        conversionWidget->setVisible(m_QMLLoader.dependencyManager().isDependenciesFound(m_QMLLoader.converter()->requiredApps(), &missing_deps));
        showMissingAppSelectors(missing_deps, false);
    }
}

void OptionsWidget::requestParamUpdate(const QVariant requestor)
{
    bool is_encoder = requestor.toString() == "encoder";
    QMLPluginBase* plg = m_QMLLoader.getPlugin(is_encoder);
    if (plg && plg->update()) {
        launchDjVuGenerator();
    }

    QVariantMap val;
    if (plg && plg->state(val)) {
        m_ptrSettings->setState(m_pageId, val, is_encoder);
    }

}

} // namespace publishing


void publishing::OptionsWidget::on_cbEncodersSelector_currentIndexChanged(int index)
{
    DJVUEncoders* encoders = m_QMLLoader.encoders();
    encoders->switchActiveEncoder(index);
    if (const DjVuEncoder* enc = encoders->encoder()) {
        m_QMLLoader.converter()->filterByRequiredInput(enc->supportedInput, enc->prefferedInput);
    }
    displayWidgets();
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
