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

#ifndef TO_VEC_H_
#define TO_VEC_H_

#include "foundation_config.h"
#include <Eigen/Core>
#include <QPoint>
#include <QPointF>

inline Eigen::Vector2i toVec(QPoint const& pt)
{
	return Eigen::Vector2i(pt.x(), pt.y());
}

inline Eigen::Vector2d toVec(QPointF const& pt)
{
	return Eigen::Vector2d(pt.x(), pt.y());
}

#endif
