#include "djvuencoder.h"
#include "QuickWidgetAccessHelper.h"
#include <QVariant>
#include <QString>
#include <iostream>
#include <QJSValue>
#include <QProcess>

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

DjVuEncoder::DjVuEncoder(QObject * componentInstance, AppDependencies& dependencies):
    isValid(false)
{
    if (componentInstance->property("type").toString() == "encoder") {
        if (!getStrProperty(componentInstance, "name", this->m_name)) return;
        QString tmp;
        if (!getStrProperty(componentInstance, "supportedInput", tmp)) return;
        this->supportedInput = tmp.split(';');
        if (!getStrProperty(componentInstance, "prefferedInput", this->prefferedInput)) return;
        if (!getStrProperty(componentInstance, "supportedOutputMode", this->supportedOutputMode)) return;
        if (!getStrProperty(componentInstance, "description", this->description)) return;
        if (!getIntProperty(componentInstance, "priority", this->priority)) return;

        QuickWidgetAccessHelper reader(componentInstance, m_name);
        if (!reader.init()) {
            return;
        }

        if (!reader.getMissingAppHint(missingAppHint)) {
            return;
        }

        isValid = reader.getCommand(defaultCmd); // using default state of newly created instance

        ptrComponentInstance.reset(componentInstance);

        reader.readAppDependencies(dependencies, &requiredApps);
    }
}
