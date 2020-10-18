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

#include "EditableSpline.h"
#include "SerializableSpline.h"
#include "SplineSegment.h"
#include <QDomNode>
#include <QDomElement>
#include <QString>
#include <assert.h>
#include <cmath>

EditableSpline::EditableSpline()
{
}

EditableSpline::EditableSpline(SerializableSpline const& spline)
{
    copyFromSerializableSpline(spline);
}

void
EditableSpline::copyFromSerializableSpline(SerializableSpline const& spline)
{
    SplineVertex::Ptr vertex = m_sentinel.firstVertex();
    while (vertex.get()) {
        vertex->remove();
        vertex = m_sentinel.firstVertex();
    }

    const QPolygonF polygon = spline.toPolygon();
    for (QPointF const& pt : polygon) {
        appendVertex(pt);
    }

    SplineVertex::Ptr last_vertex(lastVertex());
    if (last_vertex.get() && firstVertex()->point() == last_vertex->point()) {
        last_vertex->remove();
    }

    setBridged(true);
}

void
EditableSpline::appendVertex(QPointF const& pt)
{
    m_sentinel.insertBefore(pt);
}

bool
EditableSpline::hasAtLeastSegments(int num) const
{
    for (SegmentIterator it((EditableSpline&)*this); num > 0 && it.hasNext(); it.next()) {
        --num;
    }

    return num == 0;
}

int
EditableSpline::segmentsCount() const
{
    int num = 0;
    for (SegmentIterator it((EditableSpline&)*this); it.hasNext(); it.next()) {
        num++;
    }

    return num;
}

QPolygonF
EditableSpline::toPolygon() const
{
    QPolygonF poly;

    SplineVertex::Ptr vertex(firstVertex());
    for (; vertex; vertex = vertex->next(SplineVertex::NO_LOOP)) {
        poly.push_back(vertex->point());
    }

    vertex = lastVertex()->next(SplineVertex::LOOP_IF_BRIDGED);
    if (vertex) {
        poly.push_back(vertex->point());
    }

    return poly;
}

qreal get_angle(QVector<QPointF>& vec)
{
    const QPointF& a = vec[0];
    const QPointF& b = vec[1];
    const QPointF& c = vec[2];
    const QPointF ab(b - a);
    const QPointF cb(b - c);
    qreal ang = (ab.x() * cb.x() + ab.y() * cb.y()) /
                (sqrt(ab.x() * ab.x() + ab.y() * ab.y()) * sqrt(cb.x() * cb.x() + cb.y() * cb.y()));
    ang = acos(ang) * 180.0 / 3.14159265;
    return ang;
}

void _simplify(QVector<QPointF>& vec, SplineVertex::Ptr vertex, qreal ang, const SplineVertex::Loop loop)
{
    vec.append(vertex->point());
    if (vec.count() == 3) {
        if (fabs(get_angle(vec)) > 180. - ang) {
            vec.remove(1);
            vertex->prev(loop)->remove();
        } else {
            vec.pop_front();
        }
    }
}

void
EditableSpline::simplify(qreal ang)
{
    SplineVertex::Ptr vertex(firstVertex());
    QVector<QPointF> vec;
    for (; vertex; vertex = vertex->next(SplineVertex::NO_LOOP)) {
        _simplify(vec, vertex, ang, SplineVertex::NO_LOOP);
    }

    vertex = lastVertex()->next(SplineVertex::LOOP_IF_BRIDGED);
    if (vertex) {
        _simplify(vec, vertex, ang, SplineVertex::LOOP_IF_BRIDGED);
    }
}

/*======================== Spline::SegmentIterator =======================*/

bool
EditableSpline::SegmentIterator::hasNext() const
{
    return m_ptrNextVertex && m_ptrNextVertex->next(SplineVertex::LOOP_IF_BRIDGED);
}

SplineSegment
EditableSpline::SegmentIterator::next()
{
    assert(hasNext());

    SplineVertex::Ptr origin(m_ptrNextVertex);
    m_ptrNextVertex = m_ptrNextVertex->next(SplineVertex::NO_LOOP);
    if (!m_ptrNextVertex) {
        return SplineSegment(origin, origin->next(SplineVertex::LOOP_IF_BRIDGED));
    } else {
        return SplineSegment(origin, m_ptrNextVertex);
    }
}

double EditableSpline::area() const
{
    QRectF const bbox(toPolygon().boundingRect());
    return bbox.width() * bbox.height();
}
