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

#ifndef OCRPARAMS_H
#define OCRPARAMS_H

#include <QDateTime>
#include <QFileInfo>
#include <QDomDocument>

namespace publish {

class OCRParams
{
public:
    OCRParams();
    OCRParams(QDomElement const& el);
    QDomElement toXml(QDomDocument& doc, QString const& name) const;
    ~OCRParams(){}

    bool operator !=(const OCRParams &other) const;
    bool operator ==(const OCRParams &other) const { return !(*this != other); }


    bool applyOCR() const { return m_applyOCR; }
    void setApplyOCR(bool val) {
        if (m_applyOCR != val) {
            m_applyOCR = val;
            resetOCRFileInfo();
        }
    }

    QString OCRFile() const { return m_OCRfile; }
    void setOCRFile(const QString& val) {
        resetOCRFileInfo();
        m_OCRfile = val;
        updateOCRFileInfo();
    }

    QDateTime OCRLastModified() const { return m_OCRFileChanged; }

    void resetOCRFileInfo() {
        m_OCRfile.clear();
        m_OCRFileChanged = QDateTime();
    }
    void updateOCRFileInfo() {
        if (m_applyOCR && !m_OCRfile.isEmpty()) {
            const QFileInfo fi(m_OCRfile);
            if (fi.exists()) {
                m_OCRFileChanged = fi.lastModified();
            }
        }
    }

    bool isCached() const;

private:
    bool m_applyOCR;
    QString m_OCRfile;
    QDateTime m_OCRFileChanged;
};

} // namespace

#endif // OCRPARAMS_H
