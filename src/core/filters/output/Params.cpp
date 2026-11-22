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
    // ColorParams reads the options of the other color mode too, so that a mode change keeps them.
    m_colorParams = ColorParams(el.namedItem("color-params").toElement());
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

    el.appendChild(m_colorParams.toXml(doc, "color-params"));

    return el;
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
        m_colorParams.copyModeAndOptions(params);
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
