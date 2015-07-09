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

#include "RoundingHasher.h"
#include "Utils.h"
#include <QByteArray>
#include <QPoint>
#include <QPointF>
#include <QLine>
#include <QLineF>
#include <QSize>
#include <QSizeF>
#include <QRect>
#include <QRectF>
#include <QPolygon>
#include <QPolygonF>
#include <string>
#include <string.h>

RoundingHasher::RoundingHasher(QCryptographicHash::Algorithm algo)
:	m_hash(algo)
{
}

RoundingHasher&
RoundingHasher::operator<<(char const* str)
{
	m_hash.addData(str, strlen(str));
	return *this;
}

RoundingHasher&
RoundingHasher::operator<<(QByteArray const& data)
{
	m_hash.addData(data);
	return *this;
}

RoundingHasher&
RoundingHasher::operator<<(int val)
{
	return (*this << std::to_string(val).c_str());
}

RoundingHasher&
RoundingHasher::operator<<(unsigned val)
{
	return (*this << std::to_string(val).c_str());
}

RoundingHasher&
RoundingHasher::operator<<(long val)
{
	return (*this << std::to_string(val).c_str());
}

RoundingHasher&
RoundingHasher::operator<<(unsigned long val)
{
	return (*this << std::to_string(val).c_str());
}

RoundingHasher&
RoundingHasher::operator<<(long long val)
{
	return (*this << std::to_string(val).c_str());
}

RoundingHasher&
RoundingHasher::operator<<(unsigned long long val)
{
	return (*this << std::to_string(val).c_str());
}

RoundingHasher&
RoundingHasher::operator<<(float val)
{
	return (*this << Utils::doubleToString(val).toUtf8());
}

RoundingHasher&
RoundingHasher::operator<<(double val)
{
	return (*this << Utils::doubleToString(val).toUtf8());
}

RoundingHasher&
RoundingHasher::operator<<(QPoint const& pt)
{
	return (*this << pt.x() << pt.y());
}

RoundingHasher&
RoundingHasher::operator<<(QPointF const& pt)
{
	return (*this << pt.x() << pt.y());
}

RoundingHasher&
RoundingHasher::operator<<(QLine const& line)
{
	return (*this << line.p1() << line.p2());
}

RoundingHasher&
RoundingHasher::operator<<(QLineF const& line)
{
	return (*this << line.p1() << line.p2());
}

RoundingHasher&
RoundingHasher::operator<<(QSize const& size)
{
	return (*this << size.width() << size.height());
}

RoundingHasher&
RoundingHasher::operator<<(QSizeF const& size)
{
	return (*this << size.width() << size.height());
}

RoundingHasher&
RoundingHasher::operator<<(QRect const& rect)
{
	return (*this << rect.x() << rect.y() << rect.width() << rect.height());
}

RoundingHasher&
RoundingHasher::operator<<(QRectF const& rect)
{
	return (*this << rect.x() << rect.y() << rect.width() << rect.height());
}

RoundingHasher&
RoundingHasher::operator<<(QPolygon const& poly)
{
	// With polygon it's a bit tricky, as we don't want to make a distinction
	// between open and closed polygons. For some reason QPolygon (unlike
	// QPolygonF) doesn't have isClosed() method, so we emulate it.
	bool const closed = !poly.empty() && poly.first() == poly.last();
	int const num_pts = closed ? poly.size() - 1 : poly.size();
	for (int i = 0; i < num_pts; ++i) {
		*this << poly[i];
	}
	return *this;
}

RoundingHasher&
RoundingHasher::operator<<(QPolygonF const& poly)
{
	// With polygon it's a bit tricky, as we don't want to make a distinction
	// between open and closed polygons.
	int const num_pts = poly.isClosed() ? poly.size() - 1 : poly.size();
	for (int i = 0; i < num_pts; ++i) {
		*this << poly[i];
	}
	return *this;
}

QByteArray
RoundingHasher::result() const
{
	return m_hash.result();
}
