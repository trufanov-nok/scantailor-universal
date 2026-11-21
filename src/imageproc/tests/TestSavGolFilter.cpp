/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

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
#include "Grayscale.h"
#include "Utils.h"
#include <QImage>
#include <QSize>
#ifndef Q_MOC_RUN
#include <boost/test/unit_test.hpp>
#endif
#include <stdlib.h>
#include <stdint.h>

namespace imageproc
{

namespace tests
{

using namespace utils;

BOOST_AUTO_TEST_SUITE(SavGolFilterTestSuite);

BOOST_AUTO_TEST_CASE(test_result_does_not_depend_on_threads)
{
    int const w = 301;
    int const h = 203;
    QImage gray(w, h, QImage::Format_Indexed8);
    gray.setColorTable(createGrayscalePalette());
    for (int y = 0; y < h; ++y) {
        uint8_t* line = gray.scanLine(y);
        for (int x = 0; x < w; ++x) {
            line[x] = static_cast<uint8_t>(rand() & 0xff);
        }
    }

    OmpThreadsGuard threads;
    threads.set(1);
    QImage const expected(savGolFilter(gray, QSize(11, 11), 4, 4));
    threads.set(8);
    for (int run = 0; run < 10; ++run) {
        BOOST_REQUIRE(savGolFilter(gray, QSize(11, 11), 4, 4) == expected);
    }
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
