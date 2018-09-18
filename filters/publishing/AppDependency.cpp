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

#include "AppDependency.h"
#include <iostream>
#include <QJSValue>
#include <QVariant>

namespace publishing
{

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
DependencyManager::addDependencies(const AppDependencies& deps)
{
    for (const AppDependency& dep: deps) {
        addDependency(dep);
    }
}

void
DependencyManager::checkDependencies(const AppDependency& dep)
{
    AppDependencies tmp;
    tmp[dep.app_name] = dep;
    checkDependencies(tmp);
}

void
DependencyManager::checkDependencies(const QStringList& apps)
{
    AppDependencies tmp;
    for (const QString& s: apps) {
        tmp[s] = m_dependencies[s];
    }
    checkDependencies(tmp);
}

void
DependencyManager::checkDependencies(const AppDependencies& deps)
{
    DependencyChecker* checker =  new DependencyChecker(deps);
    // we'll delete self at completion
    QObject::connect(checker, &DependencyChecker::dependenciesChecked, [checker](){checker->deleteLater();});

    QObject::connect(checker, &DependencyChecker::dependencyStateChanged, this, [this](const QString app_name, const AppDependencyState state) {
        m_dependencies[app_name].state = state;
        emit dependencyStateChanged();
    }, Qt::QueuedConnection); // QueuedConnection is required to create MissingAppDeps widgets in Ui thread.

    checker->start();
}

bool
DependencyManager::isDependenciesFound(const QStringList& req_apps, QStringList* missing_apps) const
{
    bool all_is_ok = true;
    for (const QString& app_name : req_apps) {
        if (m_dependencies[app_name].state != AppDependencyState::Found) {
            all_is_ok = false;
            if (missing_apps) {
                missing_apps->append(app_name);
            } else {
                break;
            }
        }
    }
    return all_is_ok;
}

}
