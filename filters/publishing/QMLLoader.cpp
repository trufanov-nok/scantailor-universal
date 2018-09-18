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

#include "QMLLoader.h"
#include <QDir>
#include <QQmlComponent>
#include <QQmlContext>

namespace publishing
{

#ifdef Q_OS_UNIX
#ifdef Q_OS_OSX
QString _qml_path;
#else
static QString _qml_path = "./filters/publishing/";
#endif
#else
QString _qml_path = qApp->applicationDirPath()+"djvu/";
#endif

QMLPluginBase::~QMLPluginBase()
{
}

TiffConverters::TiffConverters(QQmlEngine* engine, DependencyManager& dep_manager, QObject *parent):
    QMLPluginBase(engine), m_dep_manager(dep_manager)
{
    const QString _convertor_qml_("TiffPostprocessors.qml");

    QStringList sl = _qml_path.split(';');
    for (const QString& d : sl) {
        const QDir dir(QFileInfo(d).absoluteFilePath());
        QString qml_file = dir.absoluteFilePath(_convertor_qml_);
        if (QFileInfo(qml_file).exists()) {

            m_component.reset(new QQmlComponent(m_engine, qml_file));
            if (QObject* instance = m_component->create()) {
                QuickWidgetAccessHelper ch(instance, _convertor_qml_);
                if (ch.init()) {
                    AppDependencies deps;
                    if (ch.readAppDependencies(deps, &m_convertorRequiredApps)) {
                        m_dep_manager.addDependencies(deps);
                        if (ch.getState(m_defaultState)) {
                            if (ch.getCommand(m_defaultCmd)) {
                                m_instance.reset(instance);
                                m_access_helper.reset(new QuickWidgetAccessHelper(instance, _convertor_qml_));
                            }
                        }
                        m_isValid = true;
                    }
                }
            }
        }
    }
}


DJVUEncoders::~DJVUEncoders()
{
    for (int i = 0; i < m_encoders.size(); i++) {
        delete m_encoders[i];
    }
    m_encoders.clear();
}

DJVUEncoders::DJVUEncoders(QQmlEngine* engine, DependencyManager& dep_manager, QObject *parent):
    QMLPluginBase(engine), m_dep_manager(dep_manager), m_currentEncoder(0)
{
    const QStringList sl = _qml_path.split(';');
    for (const QString& d : sl) {
        const QDir dir(QFileInfo(d).absoluteFilePath());
        const QStringList files = dir.entryList(QStringList("*.qml"), QDir::Filter::Files);
        if (!files.isEmpty()) {

            for (const QString& qml_file : files) {
                if (qml_file.contains("ui.qml")) {
                    continue;
                }
                QString ff = dir.absoluteFilePath(qml_file);

                AppDependencies deps;
                DjVuEncoder* encoder = new DjVuEncoder(m_engine, ff, deps);
                if (encoder->isValid) {
                    m_encoders.append(encoder);
                    m_dep_manager.addDependencies(deps);
                } else {
                    delete encoder;
                }

            }
            qSort(m_encoders.begin(), m_encoders.end(), &DjVuEncoder::lessThan);
        }
    }
}

bool
DJVUEncoders::switchActiveEncoder(int idx)
{
    if (m_currentEncoder != idx) {
        if (DjVuEncoder* enc = encoder(idx)) {
//            component()->setParent(nullptr);
//            instance()->setParent(nullptr);
            m_currentEncoder = idx;
            m_dep_manager.checkDependencies(enc->requiredApps);

            return true;
        }
    }
    return false;
}

bool
DJVUEncoders::switchActiveEncoder(const QString& name)
{
    for (int idx = 0; idx < m_encoders.size(); idx++) {
        if (m_encoders[idx]->name == name) {
            return switchActiveEncoder(idx);
        }
    }
    return false;
}

int
DJVUEncoders::findBestEncoder(ImageInfo::ColorMode clr)
{
    const QString clr_type = ImageInfo::ColorModeToStr(clr);
    for (int idx = 0; idx < m_encoders.size(); idx++) {
        if (m_encoders[idx]->supportedOutputMode.contains(clr_type)) {
            return idx;
        }
    }
    return -1;
}




QMLLoader::QMLLoader(QObject *parent): QObject (parent)
{

    m_engine.reset(new QQmlEngine());
    if (parent) {
        if (QQmlContext* cntx = m_engine->rootContext()) {
            cntx->setContextProperty("mainApp", parent);
        }
    }

    connect(&m_dep_manager, &DependencyManager::dependencyStateChanged, this, &QMLLoader::dependencyStateChanged);

    m_converters.reset(new TiffConverters(m_engine.get(), m_dep_manager, this));
    m_encoders.reset(new DJVUEncoders(m_engine.get(), m_dep_manager, this));

    m_dep_manager.checkDependencies();
}


}
