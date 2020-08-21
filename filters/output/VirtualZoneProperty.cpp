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

#include "VirtualZoneProperty.h"
#include "PropertyFactory.h"
#include <QDomDocument>
#include <QDomDocument>
#include <QString>

namespace output
{

char const VirtualZoneProperty::m_propertyName[] = "VirtualZoneProperty";

VirtualZoneProperty::VirtualZoneProperty(QDomElement const& el)
    :   m_virtual(!el.attribute("virtual").isEmpty())
{
}

void
VirtualZoneProperty::registerIn(PropertyFactory& factory)
{
    factory.registerProperty(m_propertyName, &VirtualZoneProperty::construct);
}

IntrusivePtr<Property>
VirtualZoneProperty::clone() const
{
    return IntrusivePtr<Property>(new VirtualZoneProperty(*this));
}

QDomElement
VirtualZoneProperty::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));
    if (m_virtual) {
        el.setAttribute("virtual", "true");
    }
    return el;
}

IntrusivePtr<Property>
VirtualZoneProperty::construct(QDomElement const& el)
{
    return IntrusivePtr<Property>(new VirtualZoneProperty(el));
}

} // namespace output
