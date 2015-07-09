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

#ifndef TO_POINT_H_
#define TO_POINT_H_

#include "foundation_config.h"
#include <Eigen/Core>
#include <QPoint>
#include <QPointF>

inline QPoint toPoint(Eigen::Vector2i const& vec)
{
	return QPoint(vec[0], vec[1]);
}

inline QPointF toPoint(Eigen::Vector2f const& vec)
{
	return QPointF(vec[0], vec[1]);
}

inline QPointF toPoint(Eigen::Vector2d const& vec)
{
	return QPointF(vec[0], vec[1]);
}

#endif
