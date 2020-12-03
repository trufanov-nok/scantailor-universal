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

#ifndef M_PI
// Don't bother with tracking if first #include <math.h> is made with _USE_MATH_DEFINES defined
// This caused problems wile building in Windows
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#endif

class EditableEllipse;

class SerializableEllipse
{
public:
    SerializableEllipse();

    SerializableEllipse(EditableEllipse const& ellipse);

    SerializableEllipse(const QPointF& center, const QPointF& diffVertX = QPointF(10,0), const QPointF& diffVertY = QPointF(0,10));

    explicit SerializableEllipse(QDomElement const& el);

    QDomElement toXml(QDomDocument& doc, QString const& name) const;

    SerializableEllipse transformed(QTransform const& xform) const;

    SerializableEllipse transformed(
        boost::function<QPointF(QPointF const&)> const& xform) const;

    bool isValid() const { return !m_center.isNull(); }

    const QPointF & center() const { return m_center; }
    const QPointF & diffX() const { return m_diffVertX; }
    const QPointF & diffY() const { return m_diffVertY; }
    double angle() const { return 180./M_PI * angleRad(); }
    double angleRad() const { return atan2(m_diffVertX.y(), m_diffVertX.x()); }
    double rx() const { return distance(m_center, m_center + m_diffVertX); }
    double ry() const { return distance(m_center, m_center + m_diffVertY); }

    static double distance(QPointF const& a, QPointF const& b) {
        const double dx = b.x() - a.x();
        const double dy = b.y() - a.y();
        return std::sqrt(dx*dx + dy*dy);
    }

    bool operator !=(const SerializableEllipse &b) const;
    bool operator ==(const SerializableEllipse &b) const;

private:
    QPointF m_center;
    QPointF m_diffVertX;
    QPointF m_diffVertY;
};

#endif // SERIALIZABLEELIPSE_H_
