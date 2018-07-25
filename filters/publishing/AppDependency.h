#ifndef APPDEPENDENCY_H
#define APPDEPENDENCY_H

#include <QThread>
#include <QProcess>
#include <QMap>

enum AppDependencyState {
    Unknown = 0,
    Missing,
    Found
};

Q_DECLARE_METATYPE(AppDependencyState)

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



#endif // APPDEPENDENCY_H
