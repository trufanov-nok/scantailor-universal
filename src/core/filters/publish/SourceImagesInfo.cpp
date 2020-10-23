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

#include "SourceImagesInfo.h"
#include "OutputFileNameGenerator.h"
#include "settings/globalstaticsettings.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"

#include <QFileInfo>
#include <QDir>

namespace publish
{

FileTracker::FileTracker(): m_filesize(0) { }

FileTracker::FileTracker(const QString& filename):
    m_filename(filename), m_filesize(0)
{
}

FileTracker::FileTracker(QDomElement const& el):
    m_filesize(0)
{
    if (el.tagName().isEmpty()) {
        return;
    }

    m_filename = el.attribute("fname");
    m_filesize = el.attribute("fsize", "0").toUInt();
    // default format string doesn't include msecs
    m_last_modified = QDateTime::fromString(el.attribute("fmodified"), "dd.MM.yyyy hh:mm:ss.zzz");
}

QDomElement FileTracker::toXml(QDomDocument& doc, const QString& id) const
{
    QDomElement el(doc.createElement(id));
    el.setAttribute("fname", m_filename);
    el.setAttribute("fsize", m_filesize);
    // default format string doesn't include msecs
    el.setAttribute("fmodified", m_last_modified.toString("dd.MM.yyyy hh:mm:ss.zzz"));
    return el;
}

void FileTracker::update()
{
    const QFileInfo fi(m_filename);
    m_filesize = fi.exists() ? fi.size() : 0;
    m_last_modified = fi.lastModified();
}

bool FileTracker::isCached() const
{
    if (!m_filename.isEmpty()) {
        const QFileInfo fi(m_filename);
        if (fi.exists() && m_filesize  // don't allow empty files
                && fi.size() == m_filesize
                && fi.lastModified() == m_last_modified) {
            return true;
        }
    }
    return false;
}

bool
FileTracker::operator !=(const FileTracker &other) const
{
    return (m_filesize != other.m_filesize ||
            m_filename != other.m_filename ||
            m_last_modified != other.m_last_modified
            );
}



bool
SourceImagesInfo::operator !=(const SourceImagesInfo &other) const
{
    return (m_export_suggestion != other.m_export_suggestion ||
            m_source            != other.m_source            ||
            m_export_foreground != other.m_export_foreground ||
            m_export_background != other.m_export_background ||
            m_export_bg44       != other.m_export_bg44       ||
            m_export_jb2        != other.m_export_jb2
            );
}

SourceImagesInfo::SourceImagesInfo()
{
}

SourceImagesInfo::SourceImagesInfo(const PageId& page_id,
                                   const OutputFileNameGenerator &fname_gen,
                                   const ExportSuggestions& export_suggestions)
{
    const QString out_path = fname_gen.outDir() + "/";
    const QString djvu_path = out_path + GlobalStaticSettings::m_djvu_pages_subfolder + "/";
    const QString export_path = djvu_path + GlobalStaticSettings::m_djvu_layers_subfolder + "/";

    m_export_suggestion = export_suggestions[page_id];
    const QString source_fname = fname_gen.fileNameFor(page_id);
    m_source.setFilename(out_path + source_fname);

    QDir export_dir(export_path);
    if (!export_dir.exists()) {
        export_dir.mkpath(export_dir.path());
    }

    bool will_be_layered = m_export_suggestion.hasColorLayer && m_export_suggestion.hasBWLayer;
    if (!will_be_layered && m_export_suggestion.hasBWLayer) {
        // minidjvu-mod doesn't support images with a non-bw format even all pixels are b&w in them.
        will_be_layered = (m_export_suggestion.format != QImage::Format_Mono) &&
                (m_export_suggestion.format != QImage::Format_MonoLSB);
    }

    QString layer_fname
            = will_be_layered ? export_dir.path() + "/txt/" + source_fname : "";
    m_export_foreground.setFilename(layer_fname);

    layer_fname = will_be_layered ? export_dir.path() + "/pic/" + source_fname : "";
    m_export_background.setFilename(layer_fname);

    if (m_export_suggestion.hasColorLayer) {
        m_export_bg44.setFilename(djvu_path + QFileInfo(m_source.filename()).completeBaseName() + ".bg44");
    }
    if (m_export_suggestion.hasBWLayer) {
        m_export_jb2.setFilename(djvu_path + QFileInfo(m_source.filename()).completeBaseName() + ".jb2");
    }

    update();
}

void
SourceImagesInfo::update()
{
    m_source.update();
    m_export_background.update();
    m_export_foreground.update();
    m_export_bg44.update();
    m_export_jb2.update();
}

SourceImagesInfo::SourceImagesInfo(QDomElement const& el):
    m_export_suggestion(el.namedItem("suggest").toElement()),
    m_source(el.namedItem("source").toElement()),
    m_export_foreground(el.namedItem("foreground").toElement()),
    m_export_background(el.namedItem("background").toElement()),
    m_export_bg44(el.namedItem("bg44").toElement()),
    m_export_jb2(el.namedItem("jb2").toElement())
{
}

QDomElement
SourceImagesInfo::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));
    if (m_export_suggestion.isValid)
        el.appendChild(m_export_suggestion.toXml(doc, "suggest"));
    if (m_source.isValid())
        el.appendChild(m_source.toXml(doc, "source"));
    if (m_export_foreground.isValid())
        el.appendChild(m_export_foreground.toXml(doc, "foreground"));
    if (m_export_background.isValid())
        el.appendChild(m_export_background.toXml(doc, "background"));
    if (m_export_bg44.isValid())
        el.appendChild(m_export_bg44.toXml(doc, "bg44"));
    if (m_export_jb2.isValid())
        el.appendChild(m_export_jb2.toXml(doc, "jb2"));
    return el;
}

bool
SourceImagesInfo::isExportCached() const
{
    if (!m_export_background.isCached()) {
        return false;
    }

    return m_export_foreground.isCached();
}

} // namespace publish
