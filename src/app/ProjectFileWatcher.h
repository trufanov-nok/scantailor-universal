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

#ifndef PROJECTFILEWATCHER_H_
#define PROJECTFILEWATCHER_H_

#include <QByteArray>
#include <QDateTime>
#include <QFileSystemWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

// Tells when another program, for example the CLI, writes the project file that the GUI has open.
class ProjectFileWatcher : public QObject
{
    Q_OBJECT
public:
    explicit ProjectFileWatcher(QObject* parent = nullptr);

    void watch(QString const& file_path);

    void stop();

    void rememberFileContent(QString const& file_path);

    QByteArray const& knownContent() const
    {
        return m_knownContent;
    }

    bool isChangedOnDisk() const;
signals:
    void changedOnDisk();
private slots:
    void fileEvent();

    void folderEvent();

    void check();
private:
    void addWatchPaths();

    QFileSystemWatcher m_watcher;
    QTimer m_delay;
    QString m_filePath;
    QByteArray m_knownContent;
    bool m_fileEventSeen;
    qint64 m_knownSize;
    QDateTime m_knownModified;
};

#endif
