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

#include "EditableEllipse.h"
#include "SerializableEllipse.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <QDomNode>
#include <QDomElement>
#include <QString>
#include <assert.h>
#include <cmath>


void
EditableEllipse::changePoint(int idx, QPointF const & new_point, bool make_circle) {
    const double d = SerializableEllipse::distance(m_center, new_point);
    if (make_circle) {
        m_rx = m_ry = d;
    } else
    if (idx == 0 || idx == 2) {
        m_rx = d;
    } else {
        m_ry = d;
    }

    const QPointF p = new_point - m_center;
    m_angle = p.x() ? atan(p.y()/p.x()) : 0.;

    while (idx-- > 0) {
        m_angle -= M_PI_2;
    }

    update_points();
}

EditableEllipse::EditableEllipse(SerializableEllipse const& ellipse)
{
    copyFromSerializableEllipse(ellipse);
}

void
EditableEllipse::copyFromSerializableEllipse(SerializableEllipse const& ellipse)
{
    m_center = ellipse.center();
    m_rx = ellipse.rx();
    m_ry = ellipse.ry();
    m_angle = ellipse.angleRad();
    m_points.resize(4);
    update_points();
}

bool
EditableEllipse::contains(QPointF const & point, bool *exactly_on_border) const
{
    const double c = cos(m_angle);
    const double s = sin(m_angle);
    const double d1 = (c*(point.x() - m_center.x()) + s*(point.y() - m_center.y()))/ rx();
    const double d2 = (s*(point.x() - m_center.x()) + c*(point.y() - m_center.y()))/ ry();
    const double res = (d1*d1 + d2*d2);
    if (exactly_on_border) {
        *exactly_on_border = (res == 1.);
    }
    return res <= 1.;
}

bool
EditableEllipse::operator ==(const SerializableEllipse &b) const
{
    if (m_center == b.center() &&
            m_rx == b.rx() && m_ry == b.ry() &&
            m_angle == b.angleRad()) {
        return true;
    }
    return false;
}

QTransform get_transform(double dx, double dy, double angle)
{
    return QTransform().translate(dx, dy).rotate(angle).translate(-dx, -dy);
}

QTransform
EditableEllipse::transform() const
{
    return get_transform(center().x(), center().y(), angle());
}

QTransform
EditableEllipse::transform(QTransform const& trans) const
{
    return trans * get_transform(center(trans).x(), center(trans).y(), angle());
}

double
EditableEllipse::area() const
{
    return M_PI * m_rx * m_ry / 4;
}
