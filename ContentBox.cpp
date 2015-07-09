/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "ContentBox.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "imageproc/AbstractImageTransform.h"
#include <QString>
#include <QDomDocument>
#include <QDomElement>
#include <QRectF>
#include <algorithm>
#include <array>

using namespace imageproc;

ContentBox::ContentBox()
{
}

ContentBox::ContentBox(
	AbstractImageTransform const& transform, QRectF const& transformed_rect)
{
	QRectF const& r = transformed_rect;
	auto const mapper(transform.backwardMapper());
	m_topEdgeMidPoint = mapper(0.5 * (r.topLeft() + r.topRight()));
	m_bottomEdgeMidPoint = mapper(0.5 * (r.bottomLeft() + r.bottomRight()));
	m_leftEdgeMidPoint = mapper(0.5 * (r.topLeft() + r.bottomLeft()));
	m_rightEdgeMidPoint = mapper(0.5 * (r.topRight() + r.bottomRight()));
}
	
ContentBox::ContentBox(QDomElement const& el)
:	m_topEdgeMidPoint(XmlUnmarshaller::pointF(el.namedItem("top").toElement()))
,	m_bottomEdgeMidPoint(XmlUnmarshaller::pointF(el.namedItem("bottom").toElement()))
,	m_leftEdgeMidPoint(XmlUnmarshaller::pointF(el.namedItem("left").toElement()))
,	m_rightEdgeMidPoint(XmlUnmarshaller::pointF(el.namedItem("right").toElement()))
{
}
	
ContentBox::~ContentBox()
{
}

bool
ContentBox::isValid() const
{
	// Content box is valid when no 2 midpoints are the same.
	std::array<QPointF, 4> points;
	points[0] = m_topEdgeMidPoint;
	points[1] = m_bottomEdgeMidPoint;
	points[2] = m_leftEdgeMidPoint;
	points[3] = m_rightEdgeMidPoint;

	auto less = [](QPointF const& lhs, QPointF const& rhs) {
		if (lhs.x() != rhs.x()) {
			return lhs.x() < rhs.x();
		}
		return lhs.y() < rhs.y();
	};

	std::sort(points.begin(), points.end(), less);
	return std::adjacent_find(points.begin(), points.end()) == points.end();
}

QDomElement
ContentBox::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);

	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.pointF(m_topEdgeMidPoint, "top"));
	el.appendChild(marshaller.pointF(m_bottomEdgeMidPoint, "bottom"));
	el.appendChild(marshaller.pointF(m_leftEdgeMidPoint, "left"));
	el.appendChild(marshaller.pointF(m_rightEdgeMidPoint, "right"));
	return el;
}

QRectF
ContentBox::toTransformedRect(AbstractImageTransform const& transform) const
{
	if (!isValid()) {
		return QRectF();
	}

	auto const mapper(transform.forwardMapper());
	QPointF const top(mapper(m_topEdgeMidPoint));
	QPointF const bottom(mapper(m_bottomEdgeMidPoint));
	QPointF const left(mapper(m_leftEdgeMidPoint));
	QPointF const right(mapper(m_rightEdgeMidPoint));
	QPointF const top_left(left.x(), top.y());
	QPointF const bottom_right(right.x(), bottom.y());
	return QRectF(top_left, bottom_right).normalized();
}
