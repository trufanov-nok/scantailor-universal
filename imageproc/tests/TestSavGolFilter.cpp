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

#include "SavGolFilter.h"
#include "GrayImage.h"
#include "GridAccessor.h"
#include "RasterOpGeneric.h"
#include "Utils.h"
#include <boost/test/auto_unit_test.hpp>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cstdlib>

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(SavGolFilterTestSuite);

BOOST_AUTO_TEST_CASE(TestBlackImageRemainsUnchanged)
{
	GrayImage black_image(QSize(99, 101));
	black_image.fill(0x00);

	BOOST_CHECK(black_image == savGolFilter(black_image, QSize(5, 5), 3, 3));
}

BOOST_AUTO_TEST_CASE(TestWhiteImageRemainsUnchanged)
{
	GrayImage white_image(QSize(99, 101));
	white_image.fill(0xff);

	BOOST_CHECK(white_image == savGolFilter(white_image, QSize(5, 5), 3, 3));
}

BOOST_AUTO_TEST_CASE(TestSolidGrayImageRemainsUnchanged)
{
	GrayImage gray_image(QSize(99, 101));
	gray_image.fill(0x80);

	BOOST_CHECK(gray_image == savGolFilter(gray_image, QSize(5, 5), 3, 3));
}


BOOST_AUTO_TEST_CASE(TestEdgePixelsAreHandledByMirroring)
{
	GrayImage src(randomGrayImage(104, 104));
	QRect const inner_rect(2, 2, 100, 100);

	// Apply mirroring.
	GridAccessor<uint8_t> const pixels(src.accessor());
	for (int y = 0; y < 104; ++y) {
		for (int x = 0; x < 104; ++x) {
			int mx = x;
			int my = y;

			if (x < inner_rect.left()) {
				mx = inner_rect.left() + (inner_rect.left() - x);
			} else if (x > inner_rect.right()) {
				mx = inner_rect.right() - (x - inner_rect.right());
			}

			if (y < inner_rect.top()) {
				my = inner_rect.top() + (inner_rect.top() - y);
			} else if (y > inner_rect.bottom()) {
				my = inner_rect.bottom() - (y - inner_rect.bottom());
			}

			pixels(x, y) = pixels(mx, my);
		}
	}

	GrayImage const full_filtered = savGolFilter(src, QSize(5, 5), 3, 3);
	GrayImage const inner_filtered = savGolFilter(
		GrayImage(src.toQImage().copy(inner_rect)), QSize(5, 5), 3, 3
	);

	int max_err = 0;
	rasterOpGeneric([&max_err](int px1, int px2) {
		max_err = std::max<int>(max_err, std::abs(px1 - px2));
	}, inner_filtered, GrayImage(full_filtered.toQImage().copy(inner_rect)));

	BOOST_CHECK_LE(max_err, 1);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
