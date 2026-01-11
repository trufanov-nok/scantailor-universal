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

#include "BlackWhiteOptions.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include "settings/ini_keys.h"
#include "settings/globalstaticsettings.h"

namespace output
{

BlackWhiteOptions::BlackWhiteOptions():
    m_thresholdAdjustment(GlobalStaticSettings::m_binrization_threshold_control_default),
    m_thresholdForegroundAdjustment(m_thresholdAdjustment),
    m_thresholdMethod(T_OTSU),
    m_thresholdWindowSize(200),
    m_thresholdCoef(0.3)
{
}

BlackWhiteOptions::BlackWhiteOptions(QDomElement const& el)
    :   m_thresholdAdjustment(el.attribute("thresholdAdj").toInt()),
        m_thresholdForegroundAdjustment(el.attribute("thresholdForegAdj").toInt()),
        m_thresholdMethod(parseThresholdMethod(el.attribute("thresholdMethod"))),
        m_thresholdWindowSize(el.attribute("thresholdWinSize").toInt()),
        m_thresholdCoef(el.attribute("thresholdCoef").toDouble())
{
   if (m_thresholdWindowSize <= 0)
    {
        m_thresholdWindowSize = 200;
    }
    if (m_thresholdCoef < 0.0)
    {
        m_thresholdCoef = 0.0;
    }
}

QDomElement
BlackWhiteOptions::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));
    el.setAttribute("thresholdAdj", m_thresholdAdjustment);
    el.setAttribute("thresholdForegAdj", m_thresholdForegroundAdjustment);
    el.setAttribute("thresholdMethod", formatThresholdMethod(m_thresholdMethod));
    el.setAttribute("thresholdWinSize", m_thresholdWindowSize);
    el.setAttribute("thresholdCoef", m_thresholdCoef);

    return el;
}

bool
BlackWhiteOptions::operator==(BlackWhiteOptions const& other) const
{
    if (m_thresholdAdjustment != other.m_thresholdAdjustment) {
        return false;
    }
    if (m_thresholdForegroundAdjustment != other.m_thresholdForegroundAdjustment) {
        return false;
    }
    if (m_thresholdMethod != other.m_thresholdMethod)
    {
        return false;
    }
    if (m_thresholdWindowSize != other.m_thresholdWindowSize)
    {
        return false;
    }
    if (m_thresholdCoef != other.m_thresholdCoef)
    {
        return false;
    }

    return true;
}

bool
BlackWhiteOptions::operator!=(BlackWhiteOptions const& other) const
{
    return !(*this == other);
}

ThresholdFilter
BlackWhiteOptions::parseThresholdMethod(QString const& str)
{
    if (str == "sauvola")
    {
        return T_SAUVOLA;
    }
    else if (str == "wolf")
    {
        return T_WOLF;
    }
    else if (str == "window")
    {
        return T_WINDOW;
    }
    else if (str == "bradley")
    {
        return T_BRADLEY;
    }
    else if (str == "grad")
    {
        return T_GRAD;
    }
    else if (str == "edgeplus")
    {
        return T_EDGEPLUS;
    }
    else if (str == "blurdiv")
    {
        return T_BLURDIV;
    }
    else if (str == "edgediv")
    {
        return T_EDGEDIV;
    }
    else
    {
        return T_OTSU;
    }
}

QString
BlackWhiteOptions::formatThresholdMethod(ThresholdFilter type)
{
    QString str = "";
    switch (type)
    {
    case T_OTSU:
        str = "otsu";
        break;
    case T_SAUVOLA:
        str = "sauvola";
        break;
    case T_WOLF:
        str = "wolf";
        break;
    case T_WINDOW:
        str = "window";
        break;
    case T_BRADLEY:
        str = "bradley";
        break;
    case T_GRAD:
        str = "grad";
        break;
    case T_EDGEPLUS:
        str = "edgeplus";
        break;
    case T_BLURDIV:
        str = "blurdiv";
        break;
    case T_EDGEDIV:
        str = "edgediv";
        break;
    }
    return str;
}
} // namespace output
