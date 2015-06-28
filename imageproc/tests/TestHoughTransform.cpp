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

#include "HoughTransform.h"
#include "ToLineProjector.h"
#include <QImage>
#include <QSize>
#include <QPoint>
#include <QPointF>
#include <QLineF>
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QString>
#include <QtGlobal>
#include <QDebug>
#include <boost/test/auto_unit_test.hpp>
#include <cmath>

namespace imageproc
{

namespace tests
{

BOOST_AUTO_TEST_SUITE(HoughTransformTestSuite);

BOOST_AUTO_TEST_CASE(test_line_detection)
{
	QLineF const actual_line(QPointF(20, 20), QPointF(80, 80));
	QImage image(100, 100, QImage::Format_ARGB32_Premultiplied);

	image.fill(0xffffffff);
	{
		QPainter painter(&image);
		QPen pen(Qt::black);
		pen.setWidthF(2.0);
		painter.setPen(pen);
		painter.drawLine(actual_line);
	}
	
	HoughTransform<unsigned> ht(
		image.size(), 1.5, HoughAngleCollection<HoughAngle>(0.0, 90.0, 1.0)
	);

	int const width = image.width();
	int const height = image.height();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			QRgb const pixel = image.pixel(x, y);

			auto updater = [pixel](unsigned& hough_val,
					HoughAngle const& angle, QPoint hough_space_pos) {
				hough_val += 255 - qGray(pixel);
			};

			ht.processSample3(x, y, updater);
		}
	}

	QPoint best_hough_pos(0, 0);
	unsigned best_hough_value = 0;
	for (int y = 0; y < ht.houghSize().height(); ++y) {
		for (int x = 0; x < ht.houghSize().width(); ++x) {
			unsigned const value = ht.houghValue(x, y);
			if (value > best_hough_value) {
				best_hough_value = value;
				best_hough_pos = QPoint(x, y);
			}
		}
	}

	QLineF const best_line = ht.houghToSpatial(best_hough_pos);

	// Check angle between the detected line and the actual line.
	BOOST_CHECK_LE(best_line.angleTo(actual_line), 1.0);

	// Check distance to origin.
	double const dist_to_origin = ToLineProjector(best_line).projectionDist(QPointF(0, 0));
	BOOST_CHECK_LE(dist_to_origin, 1.0);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
