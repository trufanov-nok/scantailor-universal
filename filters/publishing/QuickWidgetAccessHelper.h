#ifndef QUICKWIDGETACCESSHELPER_H
#define QUICKWIDGETACCESSHELPER_H

#include <QQuickWidget>
#include <QJSValue>
#include <QVariantMap>
#include "AppDependency.h"

class QuickWidgetAccessHelper
{
public:

    QuickWidgetAccessHelper(QQuickWidget* wgt, const QString& fileName = "");
    QuickWidgetAccessHelper(QObject* obj, const QString& fileName = "");
    bool init();
    bool getMissingAppHint(QString& val) { return callFunc("getMissingAppHint", val); }
    bool getCommand(QString& val) { return callFunc("getCommand", val); }
    bool setState(const QVariantMap& map);
    bool getState(QVariantMap& map);
    bool setOutputDpi(int dpi) { return setProperty("_dpi_", dpi); }
    bool getSupportedInput(QString& val) { return getProperty("supportedInput", val); }
    bool getPrefferedInput(QString& val) { return getProperty("prefferedInput", val); }
    bool filterByRequiredInput(const QString& supported, const QString& preffered) {
        return callFunc("filterByRequiredInput", supported, preffered);
    }
    bool readAppDependencies(AppDependencies& deps, QStringList* apps_readed);
private:
    bool callFunc(const QString& func_name, QString& result);
    bool callFunc(const QString& func_name, const QString& arg1, const QString& arg2);
    bool setProperty(const QString& prop, int val);
    bool getProperty(const QString& prop, QString& val);
    QJSValue convertQVariantMap2QJSValue(const QVariantMap& map);
    QVariantMap convertQJSValue2QVariantMap(const QJSValue& val);
private:
    QObject* m_root;
    QString m_name;
};

#endif // QUICKWIDGETACCESSHELPER_H
