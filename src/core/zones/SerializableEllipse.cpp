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

SerializableEllipse::SerializableEllipse(): m_angle(0.)
{
}

SerializableEllipse::SerializableEllipse(EditableEllipse const& ellipse)
{
    m_center = ellipse.center();
    m_rx = ellipse.rx();
    m_ry = ellipse.ry();
    m_angle  = ellipse.angleRad();
}

SerializableEllipse::SerializableEllipse(const QPointF& center, double rx, double ry, double angle) :
    m_center(center), m_rx(rx), m_ry(ry), m_angle(angle)
{
}

SerializableEllipse::SerializableEllipse(QDomElement const& el) :
    m_center(XmlUnmarshaller::pointF(el.namedItem("center").toElement())),
    m_rx(el.attribute("rx").toDouble()),
    m_ry(el.attribute("ry").toDouble()),
    m_angle(el.attribute("angle").toDouble())
{
}

QDomElement
SerializableEllipse::toXml(QDomDocument& doc, QString const& name) const
{
    QDomElement el(doc.createElement(name));

    XmlMarshaller marshaller(doc);
    el.setAttribute("rx", m_rx);
    el.setAttribute("ry", m_ry);
    el.setAttribute("angle", m_angle);
    el.appendChild(marshaller.pointF(m_center, "center"));
    return el;
}

SerializableEllipse
SerializableEllipse::transformed(QTransform const& xform) const
{
    QPointF new_center = xform.map(m_center);
    const double c = cos(m_angle);
    const double s = sin(m_angle);
    const QPointF new_rx = xform.map(QPointF(m_rx*c, m_rx*s) + m_center);
    const QPointF new_ry = xform.map(QPointF(m_ry*s, -m_ry*c) + m_center);
    double rx = distance(new_center, new_rx);
    double ry = distance(new_center, new_ry);
    double new_angle = new_rx.x() ? atan(new_rx.y()/new_rx.x()) : 0.;
    return SerializableEllipse(new_center, rx, ry, new_angle);
}

SerializableEllipse
SerializableEllipse::transformed(
    boost::function<QPointF(QPointF const&)> const& xform) const
{
    QPointF new_center = xform(m_center);
    const double c = cos(m_angle);
    const double s = sin(m_angle);
    const QPointF new_rx = xform(QPointF(m_rx*c, m_rx*s) + m_center);
    const QPointF new_ry = xform(QPointF(m_ry*s, -m_ry*c) + m_center);
    double rx = distance(new_center, new_rx);
    double ry = distance(new_center, new_ry);
    double new_angle = new_rx.x() ? atan(new_rx.y()/new_rx.x()) : 0.;
    return SerializableEllipse(new_center, rx, ry, new_angle);
}

bool
SerializableEllipse::operator !=(const SerializableEllipse &b) const
{
    if (m_center != b.center() ||
            m_rx != b.rx() || m_ry != b.ry() ||
            m_angle != b.angleRad()) {
        return true;
    }
    return false;
}
