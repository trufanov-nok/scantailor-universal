#ifndef QMLLOADER_H
#define QMLLOADER_H

#include "AppDependency.h"
#include "djvuencoder.h"
#include "QuickWidgetAccessHelper.h"
#include <QVector>
#include <QQmlEngine>
#include <QtQuickWidgets>
#include <memory>

class QMLPluginBase: public QObject
{
    Q_OBJECT
public:

    QMLPluginBase(QQmlEngine* engine, QObject *parent = Q_NULLPTR):
        QObject(parent), m_engine(engine) {}
    void checkDependencies(const AppDependency& dep);
    void checkDependencies(const QStringList& apps);
    void checkDependencies(const AppDependencies& deps);
signals:
    void dependencyStateChanged();
protected:
    QQmlEngine* m_engine;
    AppDependencies m_dependencies;
    bool m_isValid;
};

class TiffConverters: public QMLPluginBase
{
public:
    TiffConverters(QQmlEngine* engine, QObject *parent = Q_NULLPTR);
    QString defaultCmd() const { return m_defaultCmd; }
    QStringList requiredApps() const { return m_convertorRequiredApps; }

    bool isValid() const { return m_isValid; }

    bool setState(QVariantMap const& val) {
        bool res = m_access_helper->setState(val);
        if (res) {
            res = m_access_helper->getCommand(m_cmd);
        }
        return res;
    }

    bool state(QVariantMap& res) const {
        return m_access_helper->getState(res);
    }

private:
    QStringList m_convertorRequiredApps;
    QString m_defaultCmd;
    QString m_cmd;

    std::unique_ptr<QObject> m_instance;
    std::unique_ptr<QQmlComponent> m_component;
    std::unique_ptr<QuickWidgetAccessHelper> m_access_helper;

};

class DJVUEncoders: public QMLPluginBase
{
public:
    DJVUEncoders(QQmlEngine* engine, QObject *parent = Q_NULLPTR);
    ~DJVUEncoders();

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
        return  enc ? static_cast<QQmlComponent*>(enc->ptrComponentInstance.get()) : nullptr;
    }

    bool setState(QVariantMap const& val) {
        std::unique_ptr<QuickWidgetAccessHelper> access_helper(createAccessHelperFor(m_currentEncoder));
        bool res = access_helper != nullptr;
        if (res) {
            res = access_helper->setState(val);
            if (res) {
                res = access_helper->getCommand(m_cmd);
            }
        }
        return res;
    }

    bool state(QVariantMap& res) const {
        std::unique_ptr<QuickWidgetAccessHelper> access_helper(createAccessHelperFor(m_currentEncoder));
        if (access_helper) {
            return access_helper->getState(res);
        } else {
            return false;
        }
    }

    bool switchActiveEncoder(int idx) {
        DjVuEncoder* enc = encoder(idx);
        if (enc != nullptr) {
            m_currentEncoder = idx;
            checkDependencies(enc->requiredApps);
            return true;
        }
        return false;
    }

    bool switchActiveEncoder(const QString& name) {
        for (int idx = 0; idx < m_encoders.size(); idx++) {
            if (m_encoders[idx]->name == name) {
                return switchActiveEncoder(idx);
            }
        }
        return false;
    }

    const QVector<DjVuEncoder*>& allEncoders() const { return  m_encoders; }

private:
    void clearEncoders();
    DjVuEncoder* encoder(int idx) const { return (idx >= 0 && idx < m_encoders.size()) ? m_encoders[idx] : nullptr; }

    QuickWidgetAccessHelper* createAccessHelperFor(int idx) const {
        DjVuEncoder* enc = encoder(idx);
        if (enc) {
            return new QuickWidgetAccessHelper(enc->ptrComponentInstance.get(), enc->filename);
        } else return nullptr;
    }

private:
    QVector<DjVuEncoder*> m_encoders;
    int m_currentEncoder;
    QString m_cmd;
};


class QMLLoader: public QObject
{
    Q_OBJECT
public:
    QMLLoader(QObject *parent = Q_NULLPTR);
    ~QMLLoader();

    TiffConverters* converter() const { return m_converters.get(); }
    DJVUEncoders* encoder() const { return m_encoders.get(); }
    const QVector<DjVuEncoder*>& availableEncoders() const { return  m_encoders->allEncoders(); }

    QStringList getCommands() const { return QStringList() << m_converters->defaultCmd() << m_encoders->defaultCmd(); }
signals:
    void dependencyStateChanged();

private:
    std::unique_ptr<QQmlEngine> m_engine;
    std::unique_ptr<TiffConverters> m_converters;
    std::unique_ptr<DJVUEncoders> m_encoders;

    std::unique_ptr<QObject> m_converterInstance;

    QString m_encoderCmd;
    std::unique_ptr<QQmlComponent> m_encoderComponent;

    QStringList m_convertorRequiredApps;
    QString m_convertorCmd;
    QString m_converterQMLfile;
    std::unique_ptr<QQmlComponent> m_converterComponent;
};

#endif // QMLLOADER_H
