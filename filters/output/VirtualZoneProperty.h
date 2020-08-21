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

#ifndef OUTPUT_VIRTUAL_ZONE_PROPERTY_H_
#define OUTPUT_VIRTUAL_ZONE_PROPERTY_H_

#include "Property.h"
#include "IntrusivePtr.h"
#include <QString>
#include <QUuid>

class PropertyFactory;
class QDomDocument;
class QDomElement;
class QString;

namespace output
{

class VirtualZoneProperty : public Property
{
public:
    enum Layer { NO_OP, ERASER1, PAINTER2, ERASER3 };

    VirtualZoneProperty(bool is_virtual = false)
        : m_virtual(is_virtual), m_uuid(QUuid::createUuid().toString()) {}

    VirtualZoneProperty(QDomElement const& el);

    static void registerIn(PropertyFactory& factory);

    virtual IntrusivePtr<Property> clone() const;

    virtual QDomElement toXml(QDomDocument& doc, QString const& name) const;

    bool isVirtual() const
    {
        return m_virtual;
    }

    const QString& uuid() const
    {
        return m_uuid;
    }

    void setVirtual(bool is_virtual)
    {
        m_virtual = is_virtual;
    }
private:
    static IntrusivePtr<Property> construct(QDomElement const& el);

    static char const m_propertyName[];
    bool m_virtual;
    QString m_uuid;
};

} // namespace output

#endif
