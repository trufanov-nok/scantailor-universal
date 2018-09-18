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

#include "QuickWidgetAccessHelper.h"
#include <QQuickItem>
#include <iostream>
#include <QString>
#include <QMetaObject>
#include <QJSEngine>
#include <QJSValueIterator>

namespace publishing
{

#ifdef Q_OS_UNIX
#ifdef Q_OS_OSX
QString _platform = "macos";
#else
static QString _platform = "linux";
#endif
#else
QString _platform = "windows";
#endif

QuickWidgetAccessHelper::QuickWidgetAccessHelper(QQuickWidget* wgt, const QString& fileName):
    m_name(fileName)
{
    if (wgt) {
        m_root = wgt->rootObject();
    }
}

QuickWidgetAccessHelper::QuickWidgetAccessHelper(QObject* obj, const QString& fileName):
    m_root(obj), m_name(fileName)
{
}

bool
QuickWidgetAccessHelper::init()
{
    QVariant retVal;
    bool res = QMetaObject::invokeMethod(m_root, "init", Qt::DirectConnection,
                                         Q_RETURN_ARG(QVariant, retVal),
                                         Q_ARG(QVariant, _platform));
    if (!res || !retVal.isValid() || !retVal.toBool()) {
        std::cerr << QString("init(\"%1\") failed for %2\n").arg(_platform).arg(m_name).toStdString();
        return false;
    }
    return res;
}

bool
QuickWidgetAccessHelper::callFunc(const QString& func_name, QString& result) {
    QVariant retVal;
    bool res = QMetaObject::invokeMethod(m_root, func_name.toStdString().c_str(), Qt::DirectConnection,
                                         Q_RETURN_ARG(QVariant, retVal));
    if (!res || !retVal.isValid() || !retVal.canConvert(QMetaType::QString)) {
        std::cerr << QString("%1 failed for %2.\n").arg(func_name).arg(m_name).toStdString();
        return false;
    }
    result = retVal.toString();
    return true;
}

bool
QuickWidgetAccessHelper::callFunc(const QString& func_name, const QString& arg1, const QString& arg2)
{
    if (m_root) {
        bool res = QMetaObject::invokeMethod(m_root, func_name.toStdString().c_str(), Qt::DirectConnection,
                                             Q_ARG(QVariant, arg1),
                                             Q_ARG(QVariant, arg2));
        if (!res) {
            std::cerr << QString("%1 failed for %2.\n").arg(func_name).arg(m_name).toStdString();
        }
        return res;
    }
    return false;
}

bool
QuickWidgetAccessHelper::setProperty(const QString& prop, int val)
{
    if (m_root) {
        m_root->setProperty(prop.toStdString().c_str(), val);
        return true;
    }
    return false;
}
bool
QuickWidgetAccessHelper::getProperty(const QString& prop, QString& val)
{
    if (m_root) {
        QVariant res = m_root->property(prop.toStdString().c_str());
        if (res.isValid() && res.canConvert(QMetaType::QString)) {
            val = res.toString();
            return true;
        }
    }
    return false;
}

bool
QuickWidgetAccessHelper::getState(QVariantMap& map)
{
    if (m_root) {
        QVariant retVal;
        bool res = QMetaObject::invokeMethod(m_root, "getState", Qt::DirectConnection,
                                             Q_RETURN_ARG(QVariant, retVal));
        if (res) {
            map = convertQJSValue2QVariantMap(retVal.value<QJSValue>());
        }
        return res;
    }
    return false;
}

bool
QuickWidgetAccessHelper::setState(const QVariantMap& map)
{
    if (m_root) {
        const QJSValue& val = convertQVariantMap2QJSValue( map );
        if (!val.isUndefined()) {
            bool res = QMetaObject::invokeMethod(m_root, "setState", Qt::DirectConnection,
                                                 Q_ARG(QJSValue, val));
            return res;
        }
    }
    return false;
}



QJSValue
QuickWidgetAccessHelper::convertQVariantMap2QJSValue(const QVariantMap& map)
{
    QJSEngine js_engine;
    QJSValue val = js_engine.newObject();

    for (QVariantMap::const_iterator it = map.constBegin();
         it != map.constEnd(); ++it) {
        val.setProperty(it.key(), it->toString());
    }
    return val;
}

QVariantMap
QuickWidgetAccessHelper::convertQJSValue2QVariantMap(const QJSValue& val)
{
    QVariantMap map;
    if (val.isObject()) {
        QJSValueIterator it(val);
        while (it.hasNext()) {
            it.next();
            map[it.name()] = it.value().toString();
        }
    }
    return map;
}

void
readDep(const QJSValue& jsVal, AppDependencies& deps, QStringList* apps_readed)
{
    AppDependency app_dep;
    app_dep.app_name = jsVal.property("app").toString();
    app_dep.check_cmd = jsVal.property("check_cmd").toString();
    app_dep.required_params = jsVal.property("search_params").toString().split(';', QString::SkipEmptyParts);
    app_dep.missing_app_hint = jsVal.property("missing_app_hint").toString();
    deps[app_dep.app_name] = app_dep;

    if (apps_readed) {
        apps_readed->append(app_dep.app_name);
    }
}

bool
QuickWidgetAccessHelper::readAppDependencies(AppDependencies& deps, QStringList* apps_readed)
{
    if (m_root) {
        QVariant retVal;
        bool res = QMetaObject::invokeMethod(m_root, "getDependencies", Qt::DirectConnection,
                                             Q_RETURN_ARG(QVariant, retVal));
        if (res) {
            const QJSValue jsVal = retVal.value<QJSValue>();
            if (jsVal.isArray()) {
                const uint len = jsVal.property("length").toUInt();
                for (uint i = 0; i < len; ++i) {
                    readDep(jsVal.property(i), deps, apps_readed);
                }
            } else {
                readDep(jsVal, deps, apps_readed);
            }
        }
        return res;
    }
    return false;
}
}
