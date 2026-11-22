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

#include "OrientationDetector.h"
#include <QImage>
#include <QTransform>
#include <vector>
#include <boost/test/unit_test.hpp>

namespace imageproc
{
namespace tests
{

BOOST_AUTO_TEST_SUITE(OrientationDetectorTestSuite);

static void
fillBox(QImage& image, int x0, int y0, int x1, int y1)
{
    for (int y = y0; y < y1; ++y) {
        uchar* line = image.scanLine(y);
        for (int x = x0; x < x1; ++x) {
            line[x] = 0;
        }
    }
}

// The tests run without fonts, thus boxes take the place of letters with ascenders, descenders and dots.
static QImage
makeTextPage()
{
    QImage image(1300, 1700, QImage::Format_Grayscale8);
    image.fill(255);
    image.setDotsPerMeterX(11811);
    image.setDotsPerMeterY(11811);
    int n = 0;
    for (int baseline = 100; baseline < 1600; baseline += 50) {
        int x = 100;
        while (x < 1150) {
            for (int letter = 0; letter < 6 && x < 1150; ++letter, ++n) {
                int const kind = (n * 7) % 20;
                if (kind < 5) {
                    fillBox(image, x, baseline - 26, x + 13, baseline);
                    x += 16;
                } else if (kind < 8) {
                    fillBox(image, x, baseline - 18, x + 13, baseline + 8);
                    x += 16;
                } else if (kind < 10) {
                    fillBox(image, x, baseline - 18, x + 4, baseline);
                    fillBox(image, x, baseline - 27, x + 4, baseline - 23);
                    x += 7;
                } else {
                    fillBox(image, x, baseline - 18, x + 13, baseline);
                    x += 16;
                }
            }
            x += 12;
        }
    }
    return image;
}

static QImage
rotateCcw(QImage const& image, int degrees)
{
    QImage rotated(image.transformed(QTransform().rotate(-degrees)));
    rotated.setDotsPerMeterX(image.dotsPerMeterX());
    rotated.setDotsPerMeterY(image.dotsPerMeterY());
    return rotated;
}

BOOST_AUTO_TEST_CASE(test_blank_page_gives_no_decision)
{
    QImage image(1000, 1400, QImage::Format_Grayscale8);
    image.fill(255);
    OrientationResult const result(OrientationDetector::detect(image));
    BOOST_CHECK(!result.isDecided());
    BOOST_CHECK_EQUAL(result.confidence(), 0.0);
}

BOOST_AUTO_TEST_CASE(test_text_page_in_four_rotations)
{
    QImage const page(makeTextPage());
    for (int r = 0; r < 360; r += 90) {
        OrientationResult const result(OrientationDetector::detect(rotateCcw(page, r)));
        BOOST_CHECK(result.isDecided());
        BOOST_CHECK_EQUAL(result.angle(), r);
        BOOST_CHECK(result.confidence() >= OrientationDetector::confidenceThreshold());
    }
}

BOOST_AUTO_TEST_CASE(test_exif_is_only_a_hint)
{
    OrientationResult const text(90, 1.0, true, "text");
    OrientationResult const none(0, 0.0, false, "no text");

    OrientationResult r(OrientationDetector::combine(text, 270));
    BOOST_CHECK(r.isDecided());
    BOOST_CHECK_EQUAL(r.angle(), 90);
    BOOST_CHECK(r.reason() == "text, exif disagrees");

    r = OrientationDetector::combine(text, 90);
    BOOST_CHECK(r.reason() == "text, exif agrees");

    r = OrientationDetector::combine(none, 180);
    BOOST_CHECK(r.isDecided());
    BOOST_CHECK_EQUAL(r.angle(), 180);
    BOOST_CHECK(r.confidence() < OrientationDetector::confidenceThreshold());

    r = OrientationDetector::combine(none, -1);
    BOOST_CHECK(!r.isDecided());
}

BOOST_AUTO_TEST_CASE(test_book_rotation_keeps_sideways_tables)
{
    std::vector<OrientationResult> results(6, OrientationResult(90, 1.0, true, "text"));
    results.push_back(OrientationResult(0, 1.0, true, "text"));
    results.push_back(OrientationResult(270, 1.0, true, "text"));
    results.push_back(OrientationResult(0, 0.0, false, "no text"));
    OrientationDetector::applyBookRotation(results);
    BOOST_CHECK_EQUAL(results[6].angle(), 90);
    BOOST_CHECK(results[6].confidence() < 1.0);
    BOOST_CHECK_EQUAL(results[7].angle(), 270);
    // A scan without a decision, such as a cover, takes the rotation of the book.
    BOOST_CHECK(results[8].isDecided());
    BOOST_CHECK_EQUAL(results[8].angle(), 90);
    BOOST_CHECK(results[8].confidence() < OrientationDetector::confidenceThreshold());

    std::vector<OrientationResult> few(4, OrientationResult(90, 1.0, true, "text"));
    few.push_back(OrientationResult(0, 1.0, true, "text"));
    few.push_back(OrientationResult(0, 0.0, false, "no text"));
    OrientationDetector::applyBookRotation(few);
    BOOST_CHECK_EQUAL(few[4].angle(), 0);
    BOOST_CHECK(!few[5].isDecided());
}

BOOST_AUTO_TEST_CASE(test_shape_axis_of_covers_and_spreads)
{
    // A spread that lies on its side: two equal light pages, one above the other, with a dark fold between them.
    QImage spread(1000, 1100, QImage::Format_Grayscale8);
    spread.fill(10);
    for (int y = 100; y < 1000; ++y) {
        uchar* line = spread.scanLine(y);
        for (int x = 200; x < 880; ++x) {
            line[x] = (y >= 546 && y <= 553) ? 120 : 220;
        }
    }
    BOOST_CHECK_EQUAL(OrientationDetector::shapeAxis(spread), 90);

    // A single cover that stands upright has no fold.
    QImage cover(1000, 1100, QImage::Format_Grayscale8);
    cover.fill(10);
    for (int y = 100; y < 1000; ++y) {
        uchar* line = cover.scanLine(y);
        for (int x = 250; x < 850; ++x) {
            line[x] = 200;
        }
    }
    BOOST_CHECK_EQUAL(OrientationDetector::shapeAxis(cover), 0);

    QImage empty(800, 800, QImage::Format_Grayscale8);
    empty.fill(10);
    BOOST_CHECK_EQUAL(OrientationDetector::shapeAxis(empty), -1);
}

BOOST_AUTO_TEST_CASE(test_book_rotation_uses_size_and_shape)
{
    QSize const page_size(2231, 2924);
    OrientationResult plate(0, 0.0, false, "no text");
    plate.setShape(page_size, -1);
    BOOST_CHECK_EQUAL(OrientationDetector::applyBookRotation(plate, 90, page_size).angle(), 90);

    OrientationResult spread(0, 0.0, false, "no text");
    spread.setShape(QSize(3576, 3458), 90);
    BOOST_CHECK_EQUAL(OrientationDetector::applyBookRotation(spread, 90, page_size).angle(), 90);

    OrientationResult cover(0, 0.0, false, "no text");
    cover.setShape(QSize(3576, 3458), 0);
    BOOST_CHECK(!OrientationDetector::applyBookRotation(cover, 90, page_size).isDecided());
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests
} // namespace imageproc
