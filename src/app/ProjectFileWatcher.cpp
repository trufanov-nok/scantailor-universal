/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

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

#include "ProjectFileWatcher.h"
#include <QFile>
#include <QFileInfo>
#include <QStringList>

namespace
{

bool
readFile(QString const& file_path, QByteArray& content)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    content = file.readAll();
    return true;
}

} // namespace

ProjectFileWatcher::ProjectFileWatcher(QObject* parent)
    :   QObject(parent),
        m_fileEventSeen(false),
        m_knownSize(-1)
{
    // A writer can change the file in more than one step, so the check waits until the changes stop.
    m_delay.setSingleShot(true);
    m_delay.setInterval(500);
    connect(&m_delay, SIGNAL(timeout()), SLOT(check()));
    connect(&m_watcher, SIGNAL(fileChanged(QString)), SLOT(fileEvent()));
    connect(&m_watcher, SIGNAL(directoryChanged(QString)), SLOT(folderEvent()));
}

void
ProjectFileWatcher::watch(QString const& file_path)
{
    stop();
    m_filePath = file_path;
    rememberFileContent(file_path);
    addWatchPaths();
}

void
ProjectFileWatcher::stop()
{
    m_delay.stop();
    m_fileEventSeen = false;
    QStringList const paths(m_watcher.files() + m_watcher.directories());
    if (!paths.isEmpty()) {
        m_watcher.removePaths(paths);
    }
    m_filePath.clear();
    m_knownContent.clear();
    m_knownSize = -1;
    m_knownModified = QDateTime();
}

void
ProjectFileWatcher::rememberFileContent(QString const& file_path)
{
    QByteArray content;
    if (readFile(file_path, content)) {
        m_knownContent = content;
        QFileInfo const info(file_path);
        m_knownSize = info.size();
        m_knownModified = info.lastModified();
    }
}

bool
ProjectFileWatcher::isChangedOnDisk() const
{
    if (m_filePath.isEmpty()) {
        return false;
    }
    QByteArray content;
    // A file that is missing for a moment is in a rename by its writer, and the next check sees the new file.
    if (!readFile(m_filePath, content)) {
        return false;
    }
    return content != m_knownContent;
}

void
ProjectFileWatcher::fileEvent()
{
    m_fileEventSeen = true;
    folderEvent();
}

void
ProjectFileWatcher::folderEvent()
{
    if (!m_filePath.isEmpty()) {
        m_delay.start();
    }
}

void
ProjectFileWatcher::check()
{
    addWatchPaths();
    bool const file_event = m_fileEventSeen;
    m_fileEventSeen = false;
    // Most folder events come from other files, so the size and the time tell first if a full read is necessary.
    QFileInfo const info(m_filePath);
    if (!file_event && info.exists() && info.size() == m_knownSize && info.lastModified() == m_knownModified) {
        return;
    }
    if (isChangedOnDisk()) {
        emit changedOnDisk();
    }
}

void
ProjectFileWatcher::addWatchPaths()
{
    if (m_filePath.isEmpty()) {
        return;
    }
    // A writer that replaces the file with a rename ends the watch of the file, but the watch of its folder stays.
    QString const dir(QFileInfo(m_filePath).absolutePath());
    if (!m_watcher.directories().contains(dir)) {
        m_watcher.addPath(dir);
    }
    if (!m_watcher.files().contains(m_filePath) && QFileInfo::exists(m_filePath)) {
        m_watcher.addPath(m_filePath);
    }
}
