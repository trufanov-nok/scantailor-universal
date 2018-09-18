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

#ifndef APPDEPENDENCY_H
#define APPDEPENDENCY_H

#include <QThread>
#include <QProcess>
#include <QMap>


namespace publishing
{

enum AppDependencyState {
    Unknown = 0,
    Missing,
    Found
};

struct AppDependency
{
    AppDependency(): state(AppDependencyState::Unknown) {}

    QString app_name;
    QString working_cmd;
    QString check_cmd;
    QStringList required_params;
    QString missing_app_hint;
    AppDependencyState state;

    QString getExecutable() const { return working_cmd.isEmpty() ? app_name : working_cmd; }
    QString getCheckCmd() const { return check_cmd.arg(getExecutable()); }
};

typedef QMap<QString, AppDependency> AppDependencies;

class AppDependenciesReader {
public:
    static bool fromJSObject(QObject* const obj, AppDependencies& deps, QStringList *apps_readed = nullptr);

};

class DependencyChecker: public QThread
{
    Q_OBJECT
public:
    DependencyChecker(const AppDependencies &deps);

    void run() override;
private:
    void checkDependencies(bool force_recheck = false);
    void changeAppDependencyState(AppDependency& dep, AppDependencyState new_state);
signals:
    void dependenciesChecked();
    void dependencyStateChanged(const QString app_name, const AppDependencyState new_state);
private:
    AppDependencies m_dependencies;
    QProcess m_proc;
};

class DependencyManager: public QObject
{
    Q_OBJECT
public:
    AppDependency dependency(const QString& app_name) const { return m_dependencies[app_name]; }
    void addDependency(const AppDependency& dep) { m_dependencies[dep.app_name] = dep; }
    void addDependencies(const AppDependencies& deps);

    void checkDependencies(const AppDependency& dep);
    void checkDependencies(const QStringList& apps);
    void checkDependencies(const AppDependencies& deps);
    void checkDependencies() { checkDependencies(m_dependencies); }

    bool isDependenciesFound(const QStringList& req_apps, QStringList* missing_apps) const;

signals:
    void dependencyStateChanged();
private:

    AppDependencies m_dependencies;
};

}

Q_DECLARE_METATYPE(publishing::AppDependencyState)



#endif // APPDEPENDENCY_H
