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

#include "Params.h"
#include "ColorGrayscaleOptions.h"
#include "BlackWhiteOptions.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "CommandLine.h"
#include <QDomDocument>
#include <QDomElement>
#include <QByteArray>
#include <QString>
#include "settings/ini_keys.h"

namespace output
{

Params::Params()
    :  RegenParams(), m_dpi(CommandLine::get().getDefaultOutputDpi())
{
    QSettings s;
    m_despeckleLevel = (DespeckleLevel) s.value(_key_output_despeckling_default_lvl, _key_output_despeckling_default_lvl_def).toUInt();
}

Params::Params(QDomElement const& el)
    :   RegenParams(),
        m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement())),
        m_distortionModel(el.namedItem("distortion-model").toElement()),
        m_depthPerception(el.attribute("depthPerception")),
        m_dewarpingMode(el.attribute("dewarpingMode")),
        m_despeckleLevel(despeckleLevelFromString(el.attribute("despeckleLevel")))
//        m_TiffCompression(el.attribute("tiff-compression"))

{
    QDomElement const cp(el.namedItem("color-params").toElement());
    m_colorParams.setColorMode(parseColorMode(cp.attribute("colorMode")));
    m_colorParams.setColorGrayscaleOptions(
        ColorGrayscaleOptions(
            cp.namedItem("color-or-grayscale").toElement(), m_colorParams.colorMode() == ColorParams::MIXED
        )
    );
    m_colorParams.setBlackWhiteOptions(
        BlackWhiteOptions(cp.namedItem("bw").toElement())
    );
}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
    XmlMarshaller marshaller(doc);

    QDomElement el(doc.createElement(name));
    el.appendChild(m_distortionModel.toXml(doc, "distortion-model"));
    el.setAttribute("depthPerception", m_depthPerception.toString());
    el.setAttribute("dewarpingMode", m_dewarpingMode.toString());
    el.setAttribute("despeckleLevel", despeckleLevelToString(m_despeckleLevel));
//    el.setAttribute("tiff-compression", m_TiffCompression);
    el.appendChild(marshaller.dpi(m_dpi, "dpi"));

    QDomElement cp(doc.createElement("color-params"));
    cp.setAttribute(
        "colorMode",
        formatColorMode(m_colorParams.colorMode())
    );

    cp.appendChild(
        m_colorParams.colorGrayscaleOptions().toXml(
            doc, "color-or-grayscale"
        )
    );
    cp.appendChild(m_colorParams.blackWhiteOptions().toXml(doc, "bw"));

    el.appendChild(cp);

    return el;
}

ColorParams::ColorMode
Params::parseColorMode(QString const& str)
{
    if (str == "bw") {
        return ColorParams::BLACK_AND_WHITE;
    } else if (str == "bitonal") {
        // Backwards compatibility.
        return ColorParams::BLACK_AND_WHITE;
    } else if (str == "colorOrGray") {
        return ColorParams::COLOR_GRAYSCALE;
    } else if (str == "mixed") {
        return ColorParams::MIXED;
    } else {
        return ColorParams::DefaultColorMode();
    }
}

QString
Params::formatColorMode(ColorParams::ColorMode const mode)
{
    char const* str = "";
    switch (mode) {
    case ColorParams::BLACK_AND_WHITE:
        str = "bw";
        break;
    case ColorParams::COLOR_GRAYSCALE:
        str = "colorOrGray";
        break;
    case ColorParams::MIXED:
        str = "mixed";
        break;
    }
    return QLatin1String(str);
}

void
Params::setColorParams(ColorParams const& params, ColorParamsApplyFilter const& filter)
{
    switch (filter) {
    case CopyAll:
        m_colorParams = params;
        break;
    case CopyMode: {
        if (m_colorParams.colorMode() != params.colorMode()) {
            // it's time to apply defaults
            //TRUF m_pictureShape = (PictureShape) QSettings().value("picture_zones_layer/default", output::FREE_SHAPE).toUInt();
        }
        m_colorParams.setColorMode(params.colorMode());
        m_colorParams.setColorGrayscaleOptions(params.colorGrayscaleOptions());
    }
    break;

    case CopyAllThresholds: {
        m_colorParams.setBlackWhiteOptions(params.blackWhiteOptions());
    }
    break;

    case CopyThreshold: {
        BlackWhiteOptions opt = params.blackWhiteOptions();
        opt.setThresholdForegroundAdjustment(m_colorParams.blackWhiteOptions().thresholdForegroundAdjustment());
        m_colorParams.setBlackWhiteOptions(opt);
    }
    break;
    case CopyForegroundThreshold: {
        BlackWhiteOptions opt = m_colorParams.blackWhiteOptions();
        opt.setThresholdForegroundAdjustment(params.blackWhiteOptions().thresholdForegroundAdjustment());
        m_colorParams.setBlackWhiteOptions(opt);
    }
    break;
    }
}

} // namespace output
