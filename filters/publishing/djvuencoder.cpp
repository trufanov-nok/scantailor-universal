#include "djvuencoder.h"
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

DjVuEncoder::DjVuEncoder(QObject * obj, AppDependencies& dependencies):
    isValid(false)
{
    if (obj->property("type").toString() == "encoder") {
        if (!getStrProperty(obj, "name", this->m_name)) return;
        QString tmp;
        if (!getStrProperty(obj, "supportedInput", tmp)) return;
        this->supportedInput = tmp.split(';');
        if (!getStrProperty(obj, "prefferedInput", this->prefferedInput)) return;
        if (!getStrProperty(obj, "supportedOutputMode", this->supportedOutputMode)) return;
        if (!getStrProperty(obj, "description", this->description)) return;
        if (!getIntProperty(obj, "priority", this->priority)) return;

        QVariant retVal;
        bool res = QMetaObject::invokeMethod(obj, "init", Qt::DirectConnection,
                                             Q_RETURN_ARG(QVariant, retVal),
                                             Q_ARG(QVariant, _platform));
        if (!res || !retVal.isValid() || !retVal.toBool()) {
            std::cerr << QString("init(\"%1\") failed for %2").arg(_platform).arg(m_name).toStdString();
            return;
        }

        res = QMetaObject::invokeMethod(obj, "getMissingAppHint", Qt::DirectConnection,
                                        Q_RETURN_ARG(QVariant, retVal));
        if (!res || !retVal.isValid() || !retVal.canConvert(QMetaType::QString)) {
            std::cerr << QString("getMissingAppHint() failed for %1.").arg(m_name).toStdString();
            return;
        }
        missingAppHint = retVal.toString();

        res = QMetaObject::invokeMethod(obj, "getDependencies", Qt::DirectConnection,
                                        Q_RETURN_ARG(QVariant, retVal));
        if (!res || !retVal.isValid()) {
            std::cerr << QString("getDependencies() failed for %1.").arg(m_name).toStdString();
            return;
        }

        isValid = true;

        AppDependenciesReader::fromJSObject(obj, dependencies, &requiredApps);
    }
}
