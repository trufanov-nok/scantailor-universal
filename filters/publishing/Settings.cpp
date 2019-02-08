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

#include "Settings.h"
#include "../../Utils.h"

namespace publishing
{

Settings::Settings()
{
}

Settings::~Settings()
{
}

void
Settings::clear()
{
    QMutexLocker locker(&m_mutex);
    m_perPageParams.clear();
}

void
Settings::performRelinking(AbstractRelinker const& relinker)
{

}

Params
Settings::getParams(PageId const& page_id) const
{
    QMutexLocker const locker(&m_mutex);

    PerPageParams::const_iterator const it(m_perPageParams.find(page_id));
    if (it != m_perPageParams.end()) {
        return it->second;
    } else {
        return Params();
    }
}

void
Settings::setParams(PageId const& page_id, Params const& params)
{
    QMutexLocker const locker(&m_mutex);
    Utils::mapSetValue(m_perPageParams, page_id, params);
}


void
Settings::setDpi(PageId const& page_id, Dpi const& dpi)
{
    QMutexLocker const locker(&m_mutex);

    PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
    if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
        Params params;
        params.setOutputDpi(dpi);
        m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
    } else {
        it->second.setOutputDpi(dpi);
    }
}

void
Settings::setEncoderState(PageId const& page_id, QVariantMap const& val)
{
    QMutexLocker const locker(&m_mutex);

    PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
    if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
        Params params;
        params.setEncoderState(val);
        m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
    } else {
        it->second.setEncoderState(val);
    }
}

void
Settings::setConverterState(PageId const& page_id, QVariantMap const& val)
{
    QMutexLocker const locker(&m_mutex);

    PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
    if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
        Params params;
        params.setConverterState(val);
        m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
    } else {
        it->second.setConverterState(val);
    }
}

Dpi
Settings::dpi(PageId const& page_id) const
{
    QMutexLocker const locker(&m_mutex);

    PerPageParams::const_iterator const it(m_perPageParams.find(page_id));
    if (it != m_perPageParams.end()) {
        return it->second.outputDpi();
    } else {
        return Dpi();
    }
}

QString Settings::imageFilename(PageId const& page_id) const
{
    QMutexLocker const locker(&m_mutex);

    PerPageParams::const_iterator const it(m_perPageParams.find(page_id));
    if (it != m_perPageParams.end()) {
        return it->second.imageFilename();
    } else {
        return QString();
    }
}

void
Settings::setImageFilename(PageId const& page_id, QString const& fileName)
{
    QMutexLocker const locker(&m_mutex);

    PerPageParams::iterator const it(m_perPageParams.lower_bound(page_id));
    if (it == m_perPageParams.end() || m_perPageParams.key_comp()(page_id, it->first)) {
        Params params;
        params.setImageFilename(fileName);
        m_perPageParams.insert(it, PerPageParams::value_type(page_id, params));
    } else {
        it->second.setImageFilename(fileName);
    }
}

QVariantMap
Settings::encoderState(PageId const& page_id) const
{
    QMutexLocker const locker(&m_mutex);

    PerPageParams::const_iterator const it(m_perPageParams.find(page_id));
    if (it != m_perPageParams.end()) {
        return it->second.encoderState();
    } else {
        return QVariantMap();
    }
}


QVariantMap
Settings::converterState(PageId const& page_id) const
{
    QMutexLocker const locker(&m_mutex);

    PerPageParams::const_iterator const it(m_perPageParams.find(page_id));
    if (it != m_perPageParams.end()) {
        return it->second.converterState();
    } else {
        return QVariantMap();
    }
}

bool
Settings::allDjVusGenerated(QStringList& completed_djvus, QStringList& missing_pages) const
{
    QMutexLocker const locker(&m_mutex);
    completed_djvus.clear();
    missing_pages.clear();

    for (PerPageParams::const_iterator it = m_perPageParams.cbegin(); it != m_perPageParams.cend(); ++it) {
        const QString fname = it->second.djvuFilename();
        if (QFile::exists(fname)) {
            completed_djvus.append(fname);
        } else {
            QFileInfo info(it->second.imageFilename());
            missing_pages.append(info.fileName());
        }
    }

    return missing_pages.isEmpty();
}

} // namespace publishing
