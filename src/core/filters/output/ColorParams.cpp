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

#include "ColorParams.h"
#include "CommandLine.h"

#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <algorithm>

namespace output
{

ColorParams::ColorParams()
    :   m_colorMode(DefaultColorMode()),
        m_optionsMode(m_colorMode == MIXED ? MIXED : COLOR_GRAYSCALE),
        m_hasOtherOptions(false)
{
}

ColorParams::ColorParams(QDomElement const& el)
    :   m_colorMode(parseColorMode(el.attribute("colorMode"))),
        m_hasOtherOptions(false)
{
    QDomElement const options_el(el.namedItem("color-or-grayscale").toElement());
    m_optionsMode = m_colorMode;
    if (m_optionsMode == BLACK_AND_WHITE) {
        m_optionsMode = options_el.attribute("mode") == "mixed" ? MIXED : COLOR_GRAYSCALE;
    }
    m_colorGrayscaleOptions = ColorGrayscaleOptions(options_el, m_optionsMode == MIXED);

    QDomElement const other_el(el.namedItem("other-color-or-grayscale").toElement());
    if (!other_el.isNull()) {
        m_otherOptions = ColorGrayscaleOptions(other_el, m_optionsMode != MIXED);
        m_hasOtherOptions = true;
    }
    m_bwOptions = BlackWhiteOptions(el.namedItem("bw").toElement());
}

void
ColorParams::setColorMode(ColorMode const mode)
{
    // B/W mode keeps the color options, and the next change to a color mode finds them again.
    if (mode != BLACK_AND_WHITE && mode != m_optionsMode) {
        if (m_hasOtherOptions) {
            std::swap(m_colorGrayscaleOptions, m_otherOptions);
        } else {
            m_otherOptions = m_colorGrayscaleOptions;
            m_hasOtherOptions = true;
            bool const defaults = (mode == MIXED);
            m_colorGrayscaleOptions = ColorGrayscaleOptions(defaults, defaults);
        }
        m_optionsMode = mode;
    }
    m_colorMode = mode;
}

void
ColorParams::copyModeAndOptions(ColorParams const& other)
{
    // A B/W source can hold options of either color mode, so first move to that mode to keep the options of the other mode.
    setColorMode(other.m_optionsMode);
    m_colorMode = other.m_colorMode;
    m_colorGrayscaleOptions = other.m_colorGrayscaleOptions;
}

ColorParams::ColorMode
ColorParams::DefaultColorMode()
{
    return CommandLine::get().getDefaultColorMode();
}

QDomElement
ColorParams::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));
    el.setAttribute("colorMode", formatColorMode(m_colorMode));
    QDomElement options_el(m_colorGrayscaleOptions.toXml(doc, "color-or-grayscale"));
    if (m_colorMode == BLACK_AND_WHITE && m_optionsMode == MIXED) {
        options_el.setAttribute("mode", "mixed");
    }
    el.appendChild(options_el);
    if (m_hasOtherOptions) {
        el.appendChild(m_otherOptions.toXml(doc, "other-color-or-grayscale"));
    }
    el.appendChild(m_bwOptions.toXml(doc, "bw"));
    return el;
}

ColorParams::ColorMode
ColorParams::parseColorMode(QString const& str)
{
    if (str == "bw") {
        return BLACK_AND_WHITE;
    } else if (str == "bitonal") {
        // Backwards compatibility.
        return BLACK_AND_WHITE;
    } else if (str == "colorOrGray") {
        return COLOR_GRAYSCALE;
    } else if (str == "mixed") {
        return MIXED;
    } else {
        return DefaultColorMode();
    }
}

QString
ColorParams::formatColorMode(ColorParams::ColorMode const mode)
{
    char const* str = "";
    switch (mode) {
    case BLACK_AND_WHITE:
        str = "bw";
        break;
    case COLOR_GRAYSCALE:
        str = "colorOrGray";
        break;
    case MIXED:
        str = "mixed";
        break;
    }
    return QLatin1String(str);
}

} // namespace output
