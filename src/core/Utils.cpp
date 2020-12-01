/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "Utils.h"
#include <QString>
#include <QByteArray>
#include <QFile>
#include <QDir>
#include <QtGlobal>
#include "settings/ini_keys.h"
#include "imageproc/Constants.h" // DPI2DPM

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <stdio.h>
#endif

bool
Utils::overwritingRename(QString const& from, QString const& to)
{
#ifdef Q_OS_WIN
    return MoveFileExW(
               (WCHAR*)from.utf16(), (WCHAR*)to.utf16(),
               MOVEFILE_REPLACE_EXISTING
           ) != 0;
#else
    return rename(
               QFile::encodeName(from).data(),
               QFile::encodeName(to).data()
           ) == 0;
#endif
}

QString
Utils::richTextForLink(
    QString const& label, QString const& target)
{
    return QString::fromLatin1(
               "<a href=\"%1\">%2</a>"
           ).arg(target.toHtmlEscaped(), label.toHtmlEscaped());
}

void
Utils::maybeCreateCacheDir(QString const& output_dir)
{
    QDir(output_dir).mkdir(QLatin1String("cache"));

    // QDir::mkdir() returns false if the directory already exists,
    // so to prevent confusion this function return void.
}

QString
Utils::outputDirToThumbDir(QString const& output_dir)
{
    return output_dir + QLatin1String("/cache/thumbs");
}

IntrusivePtr<ThumbnailPixmapCache>
Utils::createThumbnailCache(QString const& output_dir)
{
    QSize const max_pixmap_size = QSettings().value(_key_thumbnails_max_cache_pixmap_size, _key_thumbnails_max_cache_pixmap_size_def).toSize();
    QString const thumbs_cache_path(outputDirToThumbDir(output_dir));

    return IntrusivePtr<ThumbnailPixmapCache>(
               new ThumbnailPixmapCache(thumbs_cache_path, max_pixmap_size, 40, 5)
           );
}

qreal
Utils::adjustByDpiAndUnits(qreal val, qreal const dpi,
                           StatusLabelPhysSizeDisplayMode mode)
{
    switch (mode) {
    case StatusLabelPhysSizeDisplayMode::Pixels:
    case StatusLabelPhysSizeDisplayMode::Inch: return val / dpi;
    case StatusLabelPhysSizeDisplayMode::MM: return val / qRound(dpi * imageproc::constants::DPI2DPM) * 1000.;
    case StatusLabelPhysSizeDisplayMode::SM: return val / qRound(dpi * imageproc::constants::DPI2DPM) * 100.;
    }
    return val;
}
