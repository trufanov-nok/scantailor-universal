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

#ifndef SERIALIZABLEELIPSE_H_
#define SERIALIZABLEELIPSE_H_

#include "RefCountable.h"
#include "IntrusivePtr.h"
#include <QPointF>
#include <QVector>
#include <QDomElement>
#include <QTransform>
#include <cmath>
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#endif

class EditableEllipse;

class SerializableEllipse
{
public:
    SerializableEllipse();

    SerializableEllipse(EditableEllipse const& ellipse);

    SerializableEllipse(const QPointF& center, double rx = 10., double ry = 10., double angle = 0);

    explicit SerializableEllipse(QDomElement const& el);

    QDomElement toXml(QDomDocument& doc, QString const& name) const;

    SerializableEllipse transformed(QTransform const& xform) const;

    SerializableEllipse transformed(
        boost::function<QPointF(QPointF const&)> const& xform) const;

    bool isValid() const { return !m_center.isNull(); }

    const QPointF & center() const { return m_center; }
    double angle() const { return 180./M_PI * m_angle; }
    double angleRad() const { return m_angle; }
    double rx() const { return m_rx; }
    double ry() const { return m_ry; }

    static double distance(QPointF const& a, QPointF const& b) {
        const double dx = b.x() - a.x();
        const double dy = b.y() - a.y();
        return std::sqrt(dx*dx + dy*dy);
    }

    bool operator !=(const SerializableEllipse &b) const;

private:
    QPointF m_center;
    double m_rx;
    double m_ry;
    double m_angle;
};

#endif // SERIALIZABLEELIPSE_H_
