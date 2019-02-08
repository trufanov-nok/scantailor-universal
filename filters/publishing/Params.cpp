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
#include <QCryptographicHash>

//#include "settings/ini_keys.h"

namespace publishing
{

Params::Params()
:  RegenParams(),
  m_dpi(CommandLine::get().getDefaultOutputDpi()),
  m_djvuSize(0)
{    
}

Params::Params(QDomElement const& el)
:	RegenParams(),
    m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement())),
    m_commandToExecute(XmlUnmarshaller::string(el.namedItem("command_to_execute").toElement())),
    m_executedCommand(XmlUnmarshaller::string(el.namedItem("executed_command").toElement()))

{
    m_inputImageInfo.fileName = XmlUnmarshaller::string(el.namedItem("input_filename").toElement());    
    m_inputImageInfo.imageHash = QByteArray::fromHex(el.attribute("input_image_hash").toLocal8Bit());
    m_inputImageInfo.imageColorMode = (ImageInfo::ColorMode) el.attribute("input_image_color_mode").toUInt();

    m_encoderId = XmlUnmarshaller::string(el.namedItem("encoder_id").toElement());

    QDomElement const ep(el.namedItem("encoder_params").toElement());
    for (int i = 0; i < ep.childNodes().count(); i++) {
        if (ep.childNodes().item(i).isCDATASection()) {
            QStringList sl = ep.childNodes().item(0).toCDATASection().data().split("\n", QString::SkipEmptyParts);
            assert(sl.count() % 2 == 0);
            for (int i = 0; i < sl.count(); i+=2) {
                m_encoderState[sl[i]] = sl[i+1];
            }
            break;
        }
    }

    QDomElement const cp(el.namedItem("converter_params").toElement());

    for (int i = 0; i < cp.childNodes().count(); i++) {
        if (cp.childNodes().item(i).isCDATASection()) {
            QStringList sl = cp.childNodes().item(0).toCDATASection().data().split("\n", QString::SkipEmptyParts);
            assert(sl.count() % 2 == 0);
            for (int i = 0; i < sl.count(); i+=2) {
                m_converterState[sl[i]] = sl[i+1];
            }
            break;
        }
    }

    m_djvuSize = el.attribute("djvu_filesize").toInt();
}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.dpi(m_dpi, "dpi"));

    el.appendChild(marshaller.string(m_encoderId, "encoder_id"));
	
    QDomElement ep(doc.createElement("encoder_params"));
    QStringList sl;
    for (QVariantMap::const_iterator it = m_encoderState.constBegin();
           it != m_encoderState.constEnd(); ++it) {
        const QString name = it.key();
        const QString val = it->toString();
        sl << name << val;
    }
    if (!sl.isEmpty()) {
        ep.appendChild(doc.createCDATASection(sl.join("\n")));
        el.appendChild(ep);
    }

    QDomElement cp(doc.createElement("converter_params"));
    sl.clear();
    for (QVariantMap::const_iterator it = m_converterState.constBegin();
           it != m_converterState.constEnd(); ++it) {
        const QString name = it.key();
        const QString val = it->toString();
         sl << name << val;
    }
    if (!sl.isEmpty()) {
        cp.appendChild(doc.createCDATASection(sl.join("\n")));
        el.appendChild(cp);
    }

    el.appendChild(marshaller.string(m_inputImageInfo.fileName, "input_filename"));
//    el.setAttribute("input_filesize", QString::number(m_inputImageInfo.fileSize));

    el.setAttribute("input_image_hash", (QString) m_inputImageInfo.imageHash.toHex());
    el.setAttribute("input_image_color_mode", QString::number((int)m_inputImageInfo.imageColorMode));
    el.setAttribute("djvu_filesize", m_djvuSize);
    el.appendChild(marshaller.string(m_commandToExecute, "command_to_execute"));
    el.appendChild(marshaller.string(m_executedCommand, "executed_command"));
	
	return el;
}

} // namespace publishing
