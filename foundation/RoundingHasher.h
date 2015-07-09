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

#ifndef ROUNDING_HASHER_H_
#define ROUNDING_HASHER_H_

#include "foundation_config.h"
#include <QCryptographicHash>

class QByteArray;
class QPoint;
class QPointF;
class QLine;
class QLineF;
class QSize;
class QSizeF;
class QRect;
class QRectF;
class QPolygon;
class QPolygonF;

/**
 * @brief Hashes floating point values by rounding them first.
 *
 * Rounding is done by Utils::doubleToString(), which is the same function
 * used for serializing to XML. That way the hash will still be valid
 * after saving and re-loading the project.
 */
class FOUNDATION_EXPORT RoundingHasher
{
	// Member-wise copying is OK.
public:
	RoundingHasher(QCryptographicHash::Algorithm algo);

	RoundingHasher& operator<<(char const* str);

	RoundingHasher& operator<<(QByteArray const& data);

	RoundingHasher& operator<<(int val);

	RoundingHasher& operator<<(unsigned val);

	RoundingHasher& operator<<(long val);

	RoundingHasher& operator<<(unsigned long val);

	RoundingHasher& operator<<(long long val);

	RoundingHasher& operator<<(unsigned long long val);

	RoundingHasher& operator<<(float val);

	RoundingHasher& operator<<(double val);

	RoundingHasher& operator<<(QPoint const& pt);

	RoundingHasher& operator<<(QPointF const& pt);

	RoundingHasher& operator<<(QLine const& line);

	RoundingHasher& operator<<(QLineF const& line);

	RoundingHasher& operator<<(QSize const& size);

	RoundingHasher& operator<<(QSizeF const& size);

	RoundingHasher& operator<<(QRect const& rect);

	RoundingHasher& operator<<(QRectF const& rect);

	RoundingHasher& operator<<(QPolygon const& poly);

	RoundingHasher& operator<<(QPolygonF const& poly);

	QByteArray result() const;
private:
	QCryptographicHash m_hash;
};

#endif
