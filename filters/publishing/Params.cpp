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
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "CommandLine.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

//#include "settings/ini_keys.h"

namespace publishing
{

Params::Params()
:  RegenParams(),
  m_dpi(CommandLine::get().getDefaultOutputDpi())
{    
}

Params::Params(QDomElement const& el)
:	RegenParams(),
    m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement())),
    m_executedCommand(XmlUnmarshaller::string(el.namedItem("executed_command").toElement()))

{
    m_inputImageInfo.fileName = XmlUnmarshaller::string(el.namedItem("input_filename").toElement());
    m_inputImageInfo.fileSize = el.attribute("input_filesize").toUInt();
    m_inputImageInfo.imageHash = el.attribute("input_image_hash").toLongLong();
    m_inputImageInfo.imageColorMode = (ImageInfo::ColorMode) el.attribute("input_image_color_mode").toUInt();

    m_encoderId = XmlUnmarshaller::string(el.namedItem("encoder_id").toElement());

    QDomElement const ep(el.namedItem("encoder_params").toElement());
    QDomNamedNodeMap map = ep.attributes();
    for (int i = 0; i < map.count(); i++) {
        const QDomNode item = map.item(i);
        m_encoderState[item.nodeName()] = item.nodeValue();
    }

    QDomElement const cp(el.namedItem("convertor_params").toElement());
    map = cp.attributes();
    for (int i = 0; i < map.count(); i++) {
        const QDomNode item = map.item(i);
        m_converterState[item.nodeName()] = item.nodeValue();
    }

}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.dpi(m_dpi, "dpi"));

    el.appendChild(marshaller.string("encoder_id", m_encoderId));
	
    QDomElement ep(doc.createElement("encoder_params"));
    for (QVariantMap::const_iterator it = m_encoderState.constBegin();
           it != m_encoderState.constEnd(); ++it) {
        const QString name = it.key();
        const QString val = it->toString();
        ep.setAttribute(name, val);
    }
    el.appendChild(ep);

    QDomElement cp(doc.createElement("convertor_params"));
    for (QVariantMap::const_iterator it = m_converterState.constBegin();
           it != m_converterState.constEnd(); ++it) {
        const QString name = it.key();
        const QString val = it->toString();
        cp.setAttribute(name, val);
    }
    el.appendChild(cp);

    el.appendChild(marshaller.string("input_filename", m_inputImageInfo.fileName));
    el.setAttribute("input_filesize", QString::number(m_inputImageInfo.fileSize));
    el.setAttribute("input_image_hash", QString::number(m_inputImageInfo.imageHash));
    el.setAttribute("input_image_color_mode", QString::number((int)m_inputImageInfo.imageColorMode));
    el.appendChild(marshaller.string("executed_command", m_executedCommand));
	
	return el;
}

ImageInfo::ImageInfo(const QString& filename, const QImage& image)
{

    fileName = filename;
    fileSize = QFileInfo(filename).size();

    imageHash = image.cacheKey();

    if (image.colorCount() <= 2) {
        imageColorMode = ImageInfo::ColorMode::BlackAndWhite;
    } else if (image.isGrayscale()) {
         imageColorMode = ImageInfo::ColorMode::Grayscale;
    } else {
         imageColorMode = ImageInfo::ColorMode::Color;
    }

}

} // namespace publishing
