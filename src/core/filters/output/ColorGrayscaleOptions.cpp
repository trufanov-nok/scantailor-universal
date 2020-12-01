/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "ColorGrayscaleOptions.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

namespace output
{

ColorGrayscaleOptions::ColorGrayscaleOptions(QDomElement const& el, bool mixed_mode)
{
    QString default_value = mixed_mode ? "1" : "0";
    m_whiteMargins = el.attribute("whiteMargins", default_value) == "1";
    m_normalizeIllumination = el.attribute("normalizeIllumination", default_value) == "1";
    m_autoLayerEnabled = el.attribute("autoLayer", default_value) == "1";
    m_pictureZonesLayerEnabled = el.attribute("pictureZonesLayer", "0") == "1";
    m_foregroundLayerEnabled = el.attribute("foregroundLayer", "0") == "1";
}

QDomElement
ColorGrayscaleOptions::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));
    el.setAttribute("whiteMargins", m_whiteMargins ? "1" : "0");
    el.setAttribute("normalizeIllumination", m_normalizeIllumination ? "1" : "0");
    el.setAttribute("autoLayer", m_autoLayerEnabled ? "1" : "0");
    el.setAttribute("pictureZonesLayer", m_pictureZonesLayerEnabled ? "1" : "0");
    el.setAttribute("foregroundLayer", m_foregroundLayerEnabled ? "1" : "0");
    return el;
}

bool
ColorGrayscaleOptions::operator==(ColorGrayscaleOptions const& other) const
{
    if (m_whiteMargins != other.m_whiteMargins) {
        return false;
    }

    if (m_normalizeIllumination != other.m_normalizeIllumination) {
        return false;
    }

    if (m_autoLayerEnabled != other.m_autoLayerEnabled) {
        return false;
    }

    if (m_pictureZonesLayerEnabled != other.m_pictureZonesLayerEnabled) {
        return false;
    }

    if (m_foregroundLayerEnabled != other.m_foregroundLayerEnabled) {
        return false;
    }

    return true;
}

bool
ColorGrayscaleOptions::operator!=(ColorGrayscaleOptions const& other) const
{
    return !(*this == other);
}

} // namespace output
