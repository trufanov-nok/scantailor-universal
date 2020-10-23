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

#include "OCRParams.h"
#include "settings/globalstaticsettings.h"

namespace publish {

OCRParams::OCRParams():
    m_applyOCR(GlobalStaticSettings::m_ocr_enabled_by_default)
{
}

OCRParams::OCRParams(QDomElement const& el)
{
    m_applyOCR = el.attribute("ocr").toUInt();
    if (m_applyOCR) {
        if (el.hasAttribute("file")) {
            m_OCRfile = el.attribute("file");
            m_OCRFileChanged = QDateTime::fromString(el.attribute("file_ver"), "dd.MM.yyyy hh:mm:ss.zzz");
        }
    }
}

QDomElement
OCRParams::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));
    el.setAttribute("ocr", m_applyOCR ? "1" : "0");
    if (m_applyOCR) {
        if (!m_OCRfile.isEmpty()) {
            el.setAttribute("file", m_OCRfile);
            el.setAttribute("file_ver", m_OCRFileChanged.toString("dd.MM.yyyy hh:mm:ss.zzz"));
        }
    }
    return el;
}

bool
OCRParams::operator !=(const OCRParams &other) const
{
    if (!other.m_applyOCR && !m_applyOCR) {
        return false;
    }

    return (m_applyOCR != other.m_applyOCR ||
            m_OCRfile != other.m_OCRfile ||
            m_OCRFileChanged != other.m_OCRFileChanged);
}

bool
OCRParams::isCached() const
{
    if (m_applyOCR && !m_OCRfile.isEmpty()) {
        const QFileInfo fi(m_OCRfile);
        if (fi.exists() && fi.lastModified() == m_OCRFileChanged) {
            return true;
        }
    }
    return false;
}

} //namespace
