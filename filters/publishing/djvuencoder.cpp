#include "djvuencoder.h"
#include <QVariant>
#include <QString>
#include <iostream>
#include <QJSValue>
#include <QProcess>


#ifdef Q_OS_UNIX
#ifdef Q_OS_OSX
QString _platform = "macos";
#else
QString _platform = "linux";
#endif
#else
QString _platform = "windows";
#endif



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

DjVuEncoder::DjVuEncoder(QObject * obj): QObject(),
    isMissing(true), isValid(false)
{
    if (obj->property("type").toString() == "encoder") {
        if (!getStrProperty(obj, "name", this->name)) return;
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
            std::cerr << QString("init(\"%1\") failed for %2").arg(_platform).arg(name).toStdString();
            return;
        }

        res = QMetaObject::invokeMethod(obj, "getMissingAppHint", Qt::DirectConnection,
                                        Q_RETURN_ARG(QVariant, retVal));
        if (!res || !retVal.isValid() || !retVal.canConvert(QMetaType::QString)) {
            std::cerr << QString("getMissingAppHint() failed for %1.").arg(name).toStdString();
            return;
        }
        missingAppHint = retVal.toString();

        res = QMetaObject::invokeMethod(obj, "getDependencies", Qt::DirectConnection,
                                        Q_RETURN_ARG(QVariant, retVal));
        if (!res || !retVal.isValid()) {
            std::cerr << QString("getDependencies() failed for %1.").arg(name).toStdString();
            return;
        }

        isValid = true;

        QJSValue deps = retVal.value<QJSValue>();

        EncoderDependency dep;
        dep.app_name = deps.property("app").toString();
        dep.check_cmd = deps.property("check_cmd").toString();
        dep.required_params = deps.property("search_params").toString().split(';');
        dependencies.append(dep);


        m_deps_checker.reset(new DependencyChecker(this));

        connect(m_deps_checker.get(), &DependencyChecker::finished, [this](){
            m_deps_checker.release()->deleteLater();
        });

        connect(m_deps_checker.get(), &DependencyChecker::dependenciesChecked, [this](bool res) {
            isMissing = !res;
        });

        m_deps_checker->start();
    }
}

DjVuEncoder::~DjVuEncoder()
{
    if (m_deps_checker) {
        m_deps_checker->requestInterruption();
        m_deps_checker->wait(-1);
    }
}


/*
 * DependencyChecker
 *
 */

DependencyChecker::DependencyChecker(DjVuEncoder* enc):
    m_name(enc->name), m_dependencies(enc->dependencies)
{
    QObject::connect(this, &DependencyChecker::dependenciesChecked,
                     enc, &DjVuEncoder::dependenciesChecked);
}

bool DependencyChecker::checkDependencies()
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    foreach (EncoderDependency dep, m_dependencies) {
        const QString cmd = dep.check_cmd.arg(dep.app_name);

        proc.start(cmd, QProcess::OpenModeFlag::ReadOnly);

        const int max_msecs = 30000;
        const int iterations = 30;
        int cnt = 0;
        while (!proc.waitForFinished(max_msecs/iterations) && !isInterruptionRequested() && cnt++ < iterations)
        if (cnt >= iterations && !isInterruptionRequested()) {
            std::cerr << QString("DjVu encoder %1: timeout for execution cmd %2\n").arg(m_name).arg(cmd).toStdString();
            return false;
        }

        if (dep.required_params.count() > 0) {
            const QString output = QString::fromStdString(proc.readAll().toStdString());
            foreach (QString param, dep.required_params) {
                if (!output.contains(param)) {
                    std::cerr << QString("DjVu encoder %1 dependency missing: %2\n").arg(m_name).arg(param).toStdString()
                              << "//cmd: " << cmd.toStdString();
                    return false;
                } else {
                    std::cerr << "found " << param.toStdString() << "\n";
                }
            }
        }

    }

    return true;
}
