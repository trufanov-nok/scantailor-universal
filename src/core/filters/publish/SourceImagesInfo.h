/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2021 Alexander Trufanov <trufanovan@gmail.com>

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

#ifndef SOURCEIMAGESINFO_H
#define SOURCEIMAGESINFO_H

#include "ExportSuggestions.h"

class OutputFileNameGenerator;
class PageId;

namespace publish
{

class FileTracker
{
public:
    FileTracker();
    FileTracker(const QString& filename);
    void setFilename(const QString& filename) { m_filename = filename; }
    const QString& filename() const { return m_filename; }
    bool operator !=(const FileTracker &other) const;
    FileTracker(QDomElement const& el);
    QDomElement toXml(QDomDocument& doc, const QString &id) const;
    void update();
    bool isValid() const { return !m_filename.isEmpty(); }
    bool isCached() const;
private:
    QString m_filename;
    uint m_filesize;
    QDateTime m_last_modified;
};


class SourceImagesInfo
{
public:
    const ExportSuggestion& export_suggestion() const { return m_export_suggestion; }

    const QString& source_filename() const { return m_source.filename(); }
    const QString& export_foregroundFilename() const { return m_export_foreground.filename(); }
    const QString& export_backgroundFilename() const { return m_export_background.filename(); }
    const QString& export_bg44Filename() const { return m_export_bg44.filename(); }
    const QString& export_jb2Filename() const { return m_export_jb2.filename(); }

    bool operator !=(const SourceImagesInfo &other) const;
    bool operator ==(const SourceImagesInfo &other) const { return !(*this != other); }
    SourceImagesInfo();
    SourceImagesInfo(const PageId& page_id,
                     const OutputFileNameGenerator & fname_gen,
                     const ExportSuggestions& export_suggestions);
    SourceImagesInfo(QDomElement const& el);
    QDomElement toXml(QDomDocument& doc, QString const& name) const;
    void update();
    bool isValid() const { return !m_source.filename().isEmpty(); }

    bool isSourceCached() const { return m_source.isCached(); }
    bool isExportCached() const;
    bool isJB2cached() const { return m_export_jb2.isCached(); }
    bool isBG44cached() const { return m_export_bg44.isCached(); }
private:
    void setFilename(QString& fname, uint& fsize, const QString& new_fname);
private:
    ExportSuggestion m_export_suggestion;

    FileTracker m_source;
    FileTracker m_export_foreground;
    FileTracker m_export_background;
    FileTracker m_export_bg44;
    FileTracker m_export_jb2;
};

} //namespace publish

#endif // SOURCEIMAGESINFO_H
