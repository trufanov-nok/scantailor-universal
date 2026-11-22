/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2026  ScanTailor Universal contributors

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

#include "PictureSkewFinder.h"
#include "GrayImage.h"
#include "BinaryImage.h"
#include "BWColor.h"
#include <QImage>
#include <QPainter>
#include <QTransform>
#include <QColor>
#ifndef Q_MOC_RUN
#include <boost/test/unit_test.hpp>
#endif
#include <math.h>

namespace imageproc
{

namespace tests
{

namespace
{

// A gray photo with a frame, turned clockwise by the given angle, on a white page.
GrayImage makePhotoPage(double angle)
{
    QImage image(900, 1300, QImage::Format_RGB32);
    image.fill(0xffffffff);
    {
        QPainter painter(&image);
        QTransform xform;
        xform.translate(0.5 * image.width(), 0.5 * image.height());
        xform.rotate(angle);
        xform.translate(-0.5 * image.width(), -0.5 * image.height());
        painter.setWorldTransform(xform);
        painter.setPen(Qt::NoPen);
        for (int y = 200; y < 1000; y += 20) {
            int const level = 70 + (y / 20 % 5) * 20;
            painter.fillRect(QRect(150, y, 600, 20), QColor(level, level, level));
        }
    }
    return GrayImage(image);
}

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(PictureSkewFinderTestSuite);

BOOST_AUTO_TEST_CASE(test_photo_angle)
{
    GrayImage const image(makePhotoPage(3.0));
    BinaryImage const page_mask(image.size(), BLACK);
    PictureSkew const skew(PictureSkewFinder::findSkew(image, page_mask));
    BOOST_CHECK(fabs(skew.angle() + 3.0) < 0.11);
    BOOST_CHECK(skew.isReliable());
}

BOOST_AUTO_TEST_CASE(test_text_page_is_not_a_picture)
{
    QImage image(900, 1300, QImage::Format_RGB32);
    image.fill(0xffffffff);
    {
        QPainter painter(&image);
        for (int y = 100; y < 1200; y += 30) {
            painter.fillRect(QRect(100, y, 700, 6), QColor(0, 0, 0));
        }
    }
    BinaryImage const page_mask(image.size(), BLACK);
    PictureSkew const skew(PictureSkewFinder::findSkew(GrayImage(image), page_mask));
    BOOST_CHECK(skew.pictureFraction() < PictureSkew::MIN_PICTURE_FRACTION);
    BOOST_CHECK(!skew.isReliable());
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
