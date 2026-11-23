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

#include "SmoothFill.h"
#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QRectF>
#include <cstdlib>
#ifndef Q_MOC_RUN
#include <boost/test/unit_test.hpp>
#endif

namespace imageproc
{

namespace tests
{

namespace
{

QPainterPath rectPath(QRectF const& rect)
{
    QPainterPath path;
    path.addRect(rect);
    return path;
}

// The colors change from left to right, so one fill color cannot match both zone edges.
QImage gradientImage(int width, int height, int left, int right)
{
    QImage image(width, height, QImage::Format_RGB32);
    for (int y = 0; y < height; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            int const v = left + (right - left) * x / (width - 1);
            line[x] = qRgb(v, v - 10, v - 20);
        }
    }
    return image;
}

QImage applyPatch(QImage const& image, QPainterPath const& path)
{
    QPoint origin;
    QImage const patch(smoothFillPatch(image, path, &origin));
    QImage result(image.convertToFormat(QImage::Format_ARGB32_Premultiplied));
    QPainter painter(&result);
    painter.drawImage(origin, patch);
    painter.end();
    return result.convertToFormat(QImage::Format_RGB32);
}

int maxDiff(QRgb a, QRgb b)
{
    return std::max(
        std::max(std::abs(qRed(a) - qRed(b)), std::abs(qGreen(a) - qGreen(b))),
        std::abs(qBlue(a) - qBlue(b))
    );
}

} // namespace

BOOST_AUTO_TEST_SUITE(SmoothFillTestSuite);

BOOST_AUTO_TEST_CASE(test_uniform_background)
{
    QImage image(200, 150, QImage::Format_RGB32);
    image.fill(qRgb(230, 220, 200));
    QPainter(&image).fillRect(80, 60, 30, 20, Qt::black);

    QImage const result(applyPatch(image, rectPath(QRectF(70, 50, 50, 40))));
    for (int y = 50; y < 90; ++y) {
        for (int x = 70; x < 120; ++x) {
            BOOST_REQUIRE_LE(maxDiff(result.pixel(x, y), qRgb(230, 220, 200)), 1);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_gradient_background)
{
    QImage image(gradientImage(300, 100, 120, 240));
    QPainter(&image).fillRect(110, 30, 80, 40, Qt::black);

    QImage const result(applyPatch(image, rectPath(QRectF(100, 20, 100, 60))));
    for (int y = 20; y < 80; ++y) {
        for (int x = 100; x < 200; ++x) {
            BOOST_REQUIRE_LE(maxDiff(result.pixel(x, y), image.pixel(x, 0)), 3);
        }
    }
}

BOOST_AUTO_TEST_CASE(test_stroke_across_edge)
{
    QImage image(200, 150, QImage::Format_RGB32);
    image.fill(qRgb(240, 240, 240));
    QPainter(&image).fillRect(0, 74, 200, 2, Qt::black);

    QImage const result(applyPatch(image, rectPath(QRectF(70, 50, 50, 50))));
    BOOST_CHECK_LE(maxDiff(result.pixel(95, 60), qRgb(240, 240, 240)), 2);
    BOOST_CHECK_LE(maxDiff(result.pixel(95, 90), qRgb(240, 240, 240)), 2);
}

BOOST_AUTO_TEST_CASE(test_large_zone_uses_coarse_grid)
{
    QImage image(gradientImage(2400, 1200, 100, 220));

    QImage const result(applyPatch(image, rectPath(QRectF(10, 10, 2380, 1180))));
    for (int x = 10; x < 2390; x += 97) {
        BOOST_REQUIRE_LE(maxDiff(result.pixel(x, 600), image.pixel(x, 600)), 3);
    }
}

BOOST_AUTO_TEST_CASE(test_bilevel_image)
{
    QImage image(120, 120, QImage::Format_Mono);
    image.setColorCount(2);
    image.fill(1);
    image.setColor(0, qRgb(0, 0, 0));
    image.setColor(1, qRgb(255, 255, 255));
    for (int y = 0; y < 120; ++y) {
        for (int x = 0; x < 120; ++x) {
            image.setPixel(x, y, x < 40 ? 0 : 1);
        }
    }

    QPoint origin;
    QImage const patch(smoothFillPatch(image, rectPath(QRectF(60, 40, 30, 30)), &origin));
    for (int y = 0; y < patch.height(); ++y) {
        for (int x = 0; x < patch.width(); ++x) {
            QRgb const p = patch.pixel(x, y);
            if (qAlpha(p) == 255) {
                BOOST_REQUIRE_EQUAL(qGray(p), 255);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(test_grain_is_deterministic)
{
    QImage image(200, 200, QImage::Format_RGB32);
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            int const v = 200 + int((unsigned(x * 7919 + y * 104729) >> 3) % 21) - 10;
            image.setPixel(x, y, qRgb(v, v, v));
        }
    }
    QPainterPath const path(rectPath(QRectF(50, 50, 100, 100)));

    QImage const first(applyPatch(image, path));
    QImage const second(applyPatch(image, path));
    BOOST_CHECK(first == second);

    // Some grain must stay, or the zone looks flat against the noisy background.
    int min_v = 255;
    int max_v = 0;
    for (int x = 60; x < 140; ++x) {
        int const v = qGray(first.pixel(x, 100));
        min_v = std::min(min_v, v);
        max_v = std::max(max_v, v);
    }
    BOOST_CHECK_GT(max_v - min_v, 4);
}

BOOST_AUTO_TEST_CASE(test_exclude_old_fill)
{
    QImage image(200, 150, QImage::Format_RGB32);
    image.fill(qRgb(220, 210, 190));
    // The left half of the ring is in the black area of an old fill, as after a zone that became smaller.
    QPainter(&image).fillRect(40, 40, 50, 70, Qt::black);

    QPainterPath const exclude(rectPath(QRectF(40, 40, 50, 70)));
    QPoint origin;
    QImage const patch(smoothFillPatch(image, rectPath(QRectF(60, 50, 60, 50)), &origin, exclude));
    QRgb const center = patch.pixel(90 - origin.x(), 75 - origin.y());
    BOOST_CHECK_LE(maxDiff(center, qRgb(220, 210, 190)), 3);
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
