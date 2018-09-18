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

#include "djvuencoder.h"
#include "QuickWidgetAccessHelper.h"
#include <QVariant>
#include <QString>
#include <iostream>
#include <QJSValue>
#include <QProcess>

namespace publishing
{

void printErr(const char* prop, QVariant const& var) {
    QString err = "Error while reading QML encoder property %1. Value is: %2\n";
    if (!var.isValid()) {
        std::cerr << err.arg(prop).arg("not valid.").toStdString();
    } else if (var.isNull()) {
        std::cerr << err.arg(prop).arg("null.").toStdString();
    } else {
        std::cerr << err.arg(prop).arg(var.toString()).toStdString();
    }
}

bool getStrProperty(QObject const* obj, const char* prop, QString& res) {
    QVariant ret = obj->property(prop);
    if (ret.isValid() && ret.canConvert(QMetaType::QString)) {
        res = ret.toString();
        return true;
    }
    printErr(prop, ret);
    return false;
}

bool getIntProperty(QObject const* obj, const char* prop, int& res) {
    QVariant ret = obj->property(prop);
    if (ret.isValid() && ret.canConvert(QMetaType::Int)) {
        res = ret.toInt();
        return true;
    }
    printErr(prop, ret);
    return false;
}

DjVuEncoder::DjVuEncoder(QQmlEngine *engine, const QString &fname, AppDependencies& dependencies):
    isValid(false)
{
    filename = fname;
    ptrComponent.reset(new QQmlComponent(engine, filename));
    ptrInstance.reset(ptrComponent->create());
    QObject* instance = ptrInstance.get();

    if (instance->property("type").toString() == "encoder") {
        if (!getStrProperty(instance, "name", name)) return;
        if (!getStrProperty(instance, "supportedInput", supportedInput)) return;
        if (!getStrProperty(instance, "prefferedInput", prefferedInput)) return;
        if (!getStrProperty(instance, "supportedOutputMode", supportedOutputMode)) return;
        if (!getStrProperty(instance, "description", description)) return;
        if (!getIntProperty(instance, "priority", priority)) return;

        QuickWidgetAccessHelper reader(instance, name);
        if (!reader.init()) {
            return;
        }

        if (!reader.getMissingAppHint(missingAppHint)) {
            return;
        }

        if (reader.getState(defaultState)) {
            isValid = reader.getCommand(defaultCmd); // using default state of newly created instance
        }

        reader.readAppDependencies(dependencies, &requiredApps);
    }
}

}
