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

