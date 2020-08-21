#include "AutoSaveTimer.h"
#include "MainWindow.h"
#include <QFile>
#include <QDir>
#include "settings/ini_keys.h"
#include <QDebug>

QAutoSaveTimer::QAutoSaveTimer(MainWindow* obj): QTimer(obj), m_MW(obj)
{
    connect(this, SIGNAL(timeout()), this, SLOT(autoSaveProject()));
}

bool
QAutoSaveTimer::copyFileTo(const QString& sFromPath, const QString& sToPath)
{
    if (QFile::exists(sFromPath)) {
        QDir d;
        QFile f(sFromPath);
        if (QFile::exists(sToPath)) {
            QFileInfo fi(sToPath);
            if (fi.isHidden() || !fi.isWritable()) {
                QFile::setPermissions(sToPath, QFile::WriteUser | QFile::ExeUser);
            }
            if (QFile::remove(sToPath)) {
                d.mkpath(QDir::toNativeSeparators(QFileInfo(sToPath).path()));
                return f.copy(sToPath);
            }
        } else {
            d.mkpath(QDir::toNativeSeparators(QFileInfo(sToPath).path()));
            return f.copy(sToPath);
        }
    }
    return false;
}

const QString
QAutoSaveTimer::getAutoSaveInputDir()
{
    return QSettings().value(_key_autosave_inputdir, "").toString();
}

void
QAutoSaveTimer::autoSaveProject()
{
    blockSignals(true);

    QString const unnamed_autosave_projectFile(
        QDir::toNativeSeparators(getAutoSaveInputDir() + "/UnnamedAutoSave.Scantailor")
    );

    if (m_MW->numImages() != 0) {
        if (m_MW->projectFile().isEmpty()) {
            m_MW->saveProjectWithFeedback(unnamed_autosave_projectFile);
        } else {
            QString const project_filename = m_MW->projectFile();
            QFileInfo const project_file(project_filename);
            QFileInfo const file_as(
                project_file.absoluteDir(),
                project_file.fileName() + ".as"
            );
            QString const file_as_path(file_as.absoluteFilePath());
            if (m_MW->saveProjectWithFeedback(file_as_path)) {
                QFile::remove(unnamed_autosave_projectFile);
                if (copyFileTo(project_filename, project_filename + ".bak")) {
                    QFile::remove(project_filename);
                }
                if (copyFileTo(file_as_path, project_filename)) {
                    QFile::remove(file_as_path);
                }
                //QFile::remove(project_filename+".bak");
            }
        }
    }

    blockSignals(false);
}
