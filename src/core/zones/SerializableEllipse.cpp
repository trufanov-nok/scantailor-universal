/*
    Scan Tailor Universal - Interactive post-processing tool for scanned
    pages. A fork of Scan Tailor by Joseph Artsimovich.
    Copyright (C) 2020 Alexander Trufanov <trufanovan@gmail.com>

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

#include "SerializableEllipse.h"
#include "EditableEllipse.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <QDomNode>
#include <QDomElement>
#include <QString>
#include <assert.h>
#include <cmath>

SerializableEllipse::SerializableEllipse()
{
}

SerializableEllipse::SerializableEllipse(EditableEllipse const& ellipse)
{
    m_center = ellipse.center();
    m_diffVertX = ellipse.const_data()[0] - m_center;
    m_diffVertY = ellipse.const_data()[1] - m_center;
}

SerializableEllipse::SerializableEllipse(const QPointF& center, const QPointF& diffVertX, const QPointF& diffVertY) :
    m_center(center), m_diffVertX(diffVertX), m_diffVertY(diffVertY)
{
}

SerializableEllipse::SerializableEllipse(QDomElement const& el) :
    m_center(XmlUnmarshaller::pointF(el.namedItem("center").toElement())),
    m_diffVertX(XmlUnmarshaller::pointF(el.namedItem("dx").toElement())),
    m_diffVertY(XmlUnmarshaller::pointF(el.namedItem("dy").toElement()))
{
}

QDomElement
SerializableEllipse::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));

    XmlMarshaller marshaller(doc);
    el.appendChild(marshaller.pointF(m_center, "center"));
    el.appendChild(marshaller.pointF(m_diffVertX, "dx"));
    el.appendChild(marshaller.pointF(m_diffVertY, "dy"));
    return el;
}

SerializableEllipse
SerializableEllipse::transformed(QTransform const& xform) const
{
    const QPointF new_center = xform.map(m_center);
    // can't use just xform.map(m_diffVertX) as it'll be rotated against wrong point
    const QPointF new_diffX = xform.map(m_center + m_diffVertX) - new_center;
    const QPointF new_diffY = xform.map(m_center + m_diffVertY) - new_center;
    return SerializableEllipse(new_center, new_diffX, new_diffY);
}

SerializableEllipse
SerializableEllipse::transformed(
        boost::function<QPointF(QPointF const&)> const& xform) const
{
    const QPointF new_center = xform(m_center);
    // can't use just xform(m_diffVertX) as it'll be rotated against wrong point
    const QPointF new_diffX = xform(m_center + m_diffVertX) - new_center;
    const QPointF new_diffY = xform(m_center + m_diffVertY) - new_center;
    return SerializableEllipse(new_center, new_diffX, new_diffY);
}

bool
SerializableEllipse::operator !=(const SerializableEllipse &b) const
{
    return (m_center != b.center() ||
            m_diffVertX != b.diffX() ||
            m_diffVertY != b.diffY());
}

bool
SerializableEllipse::operator ==(const SerializableEllipse &b) const
{
    return !(*this != b);
}
