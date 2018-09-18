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

#ifndef QMLLOADER_H
#define QMLLOADER_H

#include "AppDependency.h"
#include "djvuencoder.h"
#include "QuickWidgetAccessHelper.h"
#include "Params.h"

#include <QVector>
#include <qqmlengine.h>
#include <QtQuickWidgets>
#include <memory>

namespace publishing
{

class QMLPluginBase
{
public:
    QString cmd() const { return m_cmd; }

    bool setState(QVariantMap const& val) {
        bool res = access_helper().setState(val);
        if (res) {
            return update();
        }
        return res;
    }

    bool state(QVariantMap& res) {
        return access_helper().getState(res);
    }

    bool update() {
        return access_helper().getCommand(m_cmd);
    }

    bool isValid() const { return m_isValid; }

    virtual ~QMLPluginBase();
protected:
    virtual QuickWidgetAccessHelper& access_helper() = 0;
    QMLPluginBase(QQmlEngine* engine): m_engine(engine) {}
    bool m_isValid;
    QQmlEngine* m_engine;

private:
    QString m_cmd;

};

class TiffConverters: public QMLPluginBase
{
public:
    TiffConverters(QQmlEngine* engine, DependencyManager& dep_manager, QObject *parent = Q_NULLPTR);
    bool resetToDefaultState() { return setState(m_defaultState); }
    QString defaultCmd() const { return m_defaultCmd; }
    QStringList requiredApps() const { return m_convertorRequiredApps; }
    bool filterByRequiredInput(const QString& supported, const QString& preffered) {
        if (access_helper().filterByRequiredInput(supported, preffered)) {
            return update();
        } else {
            return false;
        }
    }
    QQmlComponent* component() const { return m_component.get(); }
    QObject* instance() const { return m_instance.get(); }
private:
    virtual QuickWidgetAccessHelper& access_helper() override { return *m_access_helper; }
private:
    DependencyManager& m_dep_manager;
    QStringList m_convertorRequiredApps;
    QVariantMap m_defaultState;
    QString m_defaultCmd;
    QString m_cmd;

    std::unique_ptr<QObject> m_instance;
    std::unique_ptr<QQmlComponent> m_component;
    std::unique_ptr<QuickWidgetAccessHelper> m_access_helper;

};

class DJVUEncoders: public QMLPluginBase
{
public:
    DJVUEncoders(QQmlEngine* engine, DependencyManager& dep_manager, QObject *parent = Q_NULLPTR);
    virtual ~DJVUEncoders() override;

    QString name() const {
        DjVuEncoder* enc = encoder(m_currentEncoder);
        return  enc ? enc->name : "";
    }

    QString defaultCmd() const {
        DjVuEncoder* enc = encoder(m_currentEncoder);
        return  enc ? enc->defaultCmd : "";
    }

    QStringList requiredApps() {
        DjVuEncoder* enc = encoder(m_currentEncoder);
        return  enc ? enc->requiredApps : QStringList();
    }

    QQmlComponent* component() {
        DjVuEncoder* enc = encoder(m_currentEncoder);
        return  enc ? enc->ptrComponent.get() : nullptr;
    }

    QObject* instance() {
        DjVuEncoder* enc = encoder(m_currentEncoder);
        return  enc ? enc->ptrInstance.get() : nullptr;
    }

    bool switchActiveEncoder(int idx);
    bool switchActiveEncoder(const QString& name);
    int findBestEncoder(ImageInfo::ColorMode clr);

    bool resetToDefaultState() {
        if (DjVuEncoder* enc = encoder(m_currentEncoder)) {
            return setState(enc->defaultState);
        }
        return false;
    }

    DjVuEncoder* encoder() const { return encoder(m_currentEncoder); }

//    bool getSupportedInput(QString& val) { return access_helper().getSupportedInput(val); }
//    bool getPrefferedInput(QString& val) { return access_helper().getPrefferedInput(val); }

    const QVector<DjVuEncoder*>& allEncoders() const { return  m_encoders; }

private:

    DjVuEncoder* encoder(int idx) const { return (idx >= 0 && idx < m_encoders.size()) ? m_encoders[idx] : nullptr; }

    QuickWidgetAccessHelper* createAccessHelperFor(int idx) const {
        DjVuEncoder* enc = encoder(idx);
        return (enc) ? new QuickWidgetAccessHelper(enc->ptrInstance.get(), enc->filename): nullptr;
    }

    virtual QuickWidgetAccessHelper& access_helper() override {
        DjVuEncoder* enc = encoder(m_currentEncoder);
        if (!enc || !m_access_helper || m_access_helper->name() != enc->filename) {
            m_access_helper.reset(createAccessHelperFor(m_currentEncoder));
        } else {
            return *m_access_helper;
        }
        return *m_access_helper;
    }

private:
    DependencyManager& m_dep_manager;
    QVector<DjVuEncoder*> m_encoders;
    int m_currentEncoder;
    std::unique_ptr<QuickWidgetAccessHelper> m_access_helper;
};


class QMLLoader: public QObject
{
    Q_OBJECT
public:
    QMLLoader(QObject *parent = Q_NULLPTR);

    ~QMLLoader() {}

    QQmlEngine* engine() const { return m_engine.get(); }
    DependencyManager& dependencyManager() { return m_dep_manager; }
    TiffConverters* converter() const { return m_converters.get(); }
    DJVUEncoders* encoders() const { return m_encoders.get(); }
    QMLPluginBase* getPlugin(bool is_encoder) { return is_encoder ? (QMLPluginBase*) m_encoders.get() : (QMLPluginBase*) m_converters.get(); }

    QStringList getCommands() const { return QStringList() << m_converters->defaultCmd() << m_encoders->defaultCmd(); }
signals:
    void dependencyStateChanged();

private:
    DependencyManager m_dep_manager;
    std::shared_ptr<QQmlEngine> m_engine;
    std::unique_ptr<TiffConverters> m_converters;
    std::unique_ptr<DJVUEncoders> m_encoders;
};

}

#endif // QMLLOADER_H
