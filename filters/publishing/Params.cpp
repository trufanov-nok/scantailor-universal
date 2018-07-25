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
#include <QMetaProperty>
#include <QString>

//#include "settings/ini_keys.h"

namespace publishing
{

Params::Params()
:  RegenParams(), m_dpi(CommandLine::get().getDefaultOutputDpi())
{
}

Params::Params(QDomElement const& el)
:	RegenParams(),
    m_dpi(XmlUnmarshaller::dpi(el.namedItem("dpi").toElement()))

{
    QDomElement const cp(el.namedItem("encoder-params").toElement());

}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.dpi(m_dpi, "dpi"));
	
    QDomElement cp(doc.createElement("encoder-params"));
	el.appendChild(cp);
    QObject* obj = m_encoderState.toQObject();
    for (int i = 0; i < obj->metaObject()->propertyCount(); i++) {
        QMetaProperty prop = obj->metaObject()->property(i);
        const QString name = prop.name();
        const QString val = obj->property(prop.name()).toString();
        el.setAttribute(name, val);
    }

	
	return el;
}

} // namespace publishing
