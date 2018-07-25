#include "AppDependency.h"
#include <iostream>
#include <QJSValue>
#include <QVariant>

DependencyChecker::DependencyChecker(const AppDependencies &deps): m_dependencies(deps)
{
}

void
DependencyChecker::run()
{
    checkDependencies();
    emit dependenciesChecked();
}

void
DependencyChecker::changeAppDependencyState(AppDependency& dep, AppDependencyState new_state)
{
    if (dep.state != new_state) {
        dep.state = new_state;
        emit dependencyStateChanged(dep.app_name, new_state);
    }
}

void
DependencyChecker::checkDependencies(bool force_recheck)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);

    for (AppDependency dep: m_dependencies) {
        if (force_recheck || dep.state != AppDependencyState::Found) {

            bool failed = false;
            const QString cmd = dep.getCheckCmd();
            proc.start(cmd, QProcess::OpenModeFlag::ReadOnly);

            const int max_msecs = 30000;
            const int iterations = 30;
            int cnt = 0;
            while (!proc.waitForFinished(max_msecs/iterations) && !isInterruptionRequested() && cnt++ < iterations)
                if (cnt >= iterations && !isInterruptionRequested()) {
                    std::cerr << QString("App dependency check: timeout while executing command \"%2\"\n").arg(cmd).toStdString();
                    changeAppDependencyState(dep, AppDependencyState::Missing);
                    failed = true;
                    break;
                }


            if (dep.required_params.count() > 0) {
                const QString output = QString::fromStdString(proc.readAll().toStdString());
                for (const QString& param: dep.required_params) {
                    if (!output.contains(param)) {
                        std::cerr << QString("App dependency missing: %2\n").arg(param).toStdString()
                                  << "//cmd: " << cmd.toStdString();
                        changeAppDependencyState(dep, AppDependencyState::Missing);
                        failed = true;
                        break;
                    } else {
                        std::cerr << "found " << param.toStdString() << "\n";
                    }
                }
            }
            if (!failed) {
                changeAppDependencyState(dep, AppDependencyState::Found);
            }
        }

    }
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
AppDependenciesReader::fromJSObject(QObject* const obj, AppDependencies& deps, QStringList* apps_readed)
{
    QVariant retVal;
    bool res = QMetaObject::invokeMethod(obj, "getDependencies", Qt::DirectConnection,
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
