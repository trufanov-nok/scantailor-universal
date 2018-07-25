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
        PageSelectionAccessor const& page_selection_accessor)
    :	m_ptrSettings(settings),
      m_pageSelectionAccessor(page_selection_accessor)
{
    setupUi(this);
    1==3;

    initTiffConvertor();
    initEncodersSelector();
    checkDependencies(m_dependencies);
}

OptionsWidget::~OptionsWidget()
{
    for (int i = 0; i < m_encoders.size(); i++) {
        delete m_encoders[i];
    }
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
OptionsWidget::preUpdateUI(QString const& filename, PageId const& page_id)
{
    m_filename = filename;
    Params const params(m_ptrSettings->getParams(page_id));
    m_pageId = page_id;
    QString dpi_label = tr("%1 dpi").arg(QString::number(params.outputDpi().horizontal()));
    dpiValue->setText(Utils::richTextForLink(dpi_label));

    displayWidgets();

    if (QQuickItem* root = conversionWidget->rootObject()) {
        const QJSValue& val = params.converterState();
        if (!val.isUndefined()) {
            bool res = QMetaObject::invokeMethod(root, "setState", Qt::DirectConnection,
                                                 Q_ARG(QJSValue, val));
        }
    }

    if (QQuickItem* root = encoderWidget->rootObject()) {
        const QJSValue& val = params.encoderState();
        if (!val.isUndefined()) {
            bool res = QMetaObject::invokeMethod(root, "setState", Qt::DirectConnection,
                                                 Q_ARG(QJSValue, val));
        }
    }

    m_pageGenerator.reset(new DjVuPageGenerator(m_filename, QStringList() << m_encoderCmd << m_convertorCmd, this));
    connect(m_pageGenerator.get(), &DjVuPageGenerator::executionComplete, [this](){

    });
    m_pageGenerator->execute();
}

void
OptionsWidget::postUpdateUI()
{
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


bool
OptionsWidget::initTiffConvertor()
{
    setupQuickWidget(conversionWidget);

    QStringList sl = _qml_path.split(';');
    foreach (QString d, sl) {
        const QDir dir(QFileInfo(d).absoluteFilePath());
        QString qml_file = dir.absoluteFilePath("TiffPostprocessors.qml");
        if (QFileInfo(qml_file).exists()) {
            conversionWidget->setSource(qml_file);
            if (QObject *item = conversionWidget->rootObject()) {
                QVariant retVal;
                bool res = QMetaObject::invokeMethod(item, "init", Qt::DirectConnection,
                                                     Q_RETURN_ARG(QVariant, retVal),
                                                     Q_ARG(QVariant, _platform));
                if (!res || !retVal.isValid() || !retVal.toBool()) {
                    std::cerr << QString("TiffPostprocessors.qml::init(\"%1\") failed").arg(_platform).toStdString();
                } else {
                    AppDependenciesReader::fromJSObject(item, m_dependencies, &m_convertorRequiredApps);
                    return true;
                }
            }

        }
    }
    return false;
}


void
OptionsWidget::initEncodersSelector()
{
    setupQuickWidget(encoderWidget);

    QStringList sl = _qml_path.split(';');
    foreach (QString d, sl) {
        const QDir dir(QFileInfo(d).absoluteFilePath());
        const QStringList files = dir.entryList(QStringList("*.qml"), QDir::Filter::Files);
        if (!files.isEmpty()) {
            QQmlEngine* engine = new QQmlEngine(this);

            if (QQmlContext* cntx = engine->rootContext()) {
                // This is a temporary qml load but set callback link to supress warnings
                cntx->setContextProperty("mainApp", this);
            }

            foreach (QString qml_file, files) {
                if (qml_file.contains("ui.qml")) {
                    continue;
                }
                QString ff = dir.absoluteFilePath(qml_file);
                QQmlComponent component(engine, ff);

                //engine.load(ff);
                QObject *item = component.create();  //dynamic_cast<QObject *>(engine.rootObjects().at(0));
                if (item) {
                    DjVuEncoder* encoder = new DjVuEncoder(item, m_dependencies);
                    if (encoder->isValid) {
                        encoder->filename = ff;
                        m_encoders.append(encoder);
                    } else {
                        delete encoder;
                    }

                    delete item;
                }
            }
            qSort(m_encoders.begin(), m_encoders.end(), &DjVuEncoder::lessThan);
            delete engine;
        }
    }


    cbEncodersSelector->clear();

    for (const DjVuEncoder* enc: m_encoders) {
        cbEncodersSelector->addItem(enc->m_name, enc->filename);
    }
}


void OptionsWidget::checkDependencies(const AppDependency& dep)
{
    AppDependencies tmp;
    tmp[dep.app_name] = dep;
    checkDependencies(tmp);
}

void OptionsWidget::checkDependencies(const AppDependencies& deps)
{
    DependencyChecker* checker =  new DependencyChecker(deps);
    // we'll delete self at completion
    connect(checker, &DependencyChecker::dependenciesChecked, [checker](){checker->deleteLater();});

    connect(checker, &DependencyChecker::dependencyStateChanged, this, [this](const QString app_name, const AppDependencyState state) {
        m_dependencies[app_name].state = state;
        displayWidgets();
    }, Qt::QueuedConnection); // QueuedConnection is required to create MissingAppDeps widgets in Ui thread.

    checker->start();
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

bool OptionsWidget::allDependenciesFound(const QStringList& req_apps, QStringList* missing_apps)
{
    bool all_is_ok = true;
    for (const QString& app_name : req_apps) {
        if (m_dependencies[app_name].state != AppDependencyState::Found) {
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
        MissingApplicationsWidget * mw = new MissingApplicationsWidget(app, m_dependencies[app].missing_app_hint, wgt);
        wgt->layout()->addWidget(mw);
        connect(mw, &MissingApplicationsWidget::newPathToExecutable, [this](QString app, QString filename) {
            m_dependencies[app].working_cmd = filename;
            checkDependencies(m_dependencies[app]);
        });
        child_cnt++;
    }

    wgt->setVisible(child_cnt > 0);
    //    wgt->updateGeometry();
}

void OptionsWidget::displayWidgets()
{
    QStringList missing_deps;
    encoderWidget->setVisible(allDependenciesFound(m_encoders[cbEncodersSelector->currentIndex()]->requiredApps,&missing_deps));
    showMissingAppSelectors(missing_deps, true);

    conversionWidget->setVisible(allDependenciesFound(m_convertorRequiredApps,&missing_deps));
    showMissingAppSelectors(missing_deps, false);
}

void OptionsWidget::requestParamUpdate(const QVariant requestor)
{
    bool is_encoder = requestor.toString() == "encoder";
    QQuickWidget* wgt = (is_encoder)? encoderWidget : conversionWidget;
    if (QObject* root = wgt->rootObject()) {
        QVariant retVal;
        bool res = QMetaObject::invokeMethod(root, "getCommand", Qt::DirectConnection,
                                             Q_RETURN_ARG(QVariant, retVal));
        if (res && retVal.isValid()) {
            if (is_encoder) {
                m_encoderCmd = retVal.toString();
            } else {
                m_convertorCmd = retVal.toString();
            }
            if (m_pageGenerator) {
                m_pageGenerator->setComands(QStringList() << m_encoderCmd << m_convertorCmd);
                m_pageGenerator->execute();
            }
        }

        res = QMetaObject::invokeMethod(root, "getState", Qt::DirectConnection,
                                             Q_RETURN_ARG(QVariant, retVal));
        if (res && retVal.isValid()) {
            const QJSValue jsVal = retVal.value<QJSValue>();
            m_ptrSettings->setState(m_pageId, jsVal, is_encoder);
        }
    }
}

void setDpi(QQuickWidget* wgt, uint val) {
    if (QObject* root = wgt->rootObject()) {
        root->setProperty("_dpi_", val);
    }
}

void
OptionsWidget::setOutputDpi(uint dpi)
{
    setDpi(encoderWidget, dpi);
//    setDpi(conversionWidget, dpi);
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

    if (QObject* enc_root = encoderWidget->rootObject()) {
        QString supported_input = enc_root->property("supportedInput").toString();
        QString preferred_input = enc_root->property("prefferedInput").toString();
        if (QObject* conv_root = conversionWidget->rootObject()) {
            bool res = QMetaObject::invokeMethod(conv_root, "filterByRequiredInput", Qt::DirectConnection,
                                                 Q_ARG(QVariant, supported_input),
                                                 Q_ARG(QVariant, preferred_input));
            Q_ASSERT(res);
        }

    }
}

void publishing::OptionsWidget::on_dpiValue_linkActivated(const QString &/*link*/)
{

    Dpi dpi(600,600);
    ChangeDpiDialog dlg(this, dpi);
    if (dlg.exec() == QDialog::Accepted) {
        m_ptrSettings->setDpi(m_pageId, dpi);
        setOutputDpi((uint)dpi.horizontal());
    };
}
