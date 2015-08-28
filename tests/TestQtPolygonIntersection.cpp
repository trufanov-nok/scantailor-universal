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

#include <QPolygonF>
#include <QPointF>
#include <boost/test/auto_unit_test.hpp>

namespace Tests
{

BOOST_AUTO_TEST_SUITE(QtPolygonIntersectionTestSuite);

BOOST_AUTO_TEST_CASE(test_QTBUG_48003)
{
	QPolygonF p1, p2;

	p1 << QPointF(4830, 6297)
	   << QPointF(4830, 2985.0666764423463)
	   << QPointF(0, 3027.2175508956193)
	   << QPointF(0, 6297);

	p2 << QPointF(4829.9999999999991, -326.86664711530557)
	   << QPointF(4830.0000000000009, 9608.9333235576541)
	   << QPointF(9.3546143180151939e-013, 9608.9333235576541)
	   << QPointF(-9.3546143180151939e-013, -326.86664711530648);

	BOOST_CHECK(!p1.intersected(p2).isEmpty());
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace Tests
