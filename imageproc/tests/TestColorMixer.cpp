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

#include "ColorMixer.h"
#include <QColor>
#include <boost/test/auto_unit_test.hpp>

namespace imageproc
{

namespace tests
{

BOOST_AUTO_TEST_SUITE(ColorMixerTestSuite);

BOOST_AUTO_TEST_CASE(test_mixing_identical_colors)
{
	GrayColorMixer<unsigned> gray_mixer;
	gray_mixer.add(100, 20);
	gray_mixer.add(100, 10);
	BOOST_CHECK(gray_mixer.mix(30) == 100);

	RgbColorMixer<unsigned> rgb_mixer;
	rgb_mixer.add(qRgb(99, 100, 101), 20);
	rgb_mixer.add(qRgb(99, 100, 101), 10);
	BOOST_CHECK(rgb_mixer.mix(30) == qRgb(99, 100, 101));

	ArgbColorMixer<unsigned> argb_mixer;
	argb_mixer.add(qRgba(99, 100, 101, 50), 20);
	argb_mixer.add(qRgba(99, 100, 101, 50), 10);
	BOOST_CHECK(argb_mixer.mix(30) == qRgba(99, 100, 101, 50));
}

BOOST_AUTO_TEST_CASE(test_mixing_similar_colors_integer)
{
	GrayColorMixer<unsigned> gray_mixer;
	gray_mixer.add(99, 10);
	gray_mixer.add(101, 10);
	BOOST_CHECK(gray_mixer.mix(20) == 100);

	RgbColorMixer<unsigned> rgb_mixer;
	rgb_mixer.add(qRgb(99, 100, 101), 10);
	rgb_mixer.add(qRgb(101, 102, 103), 10);
	BOOST_CHECK(rgb_mixer.mix(20) == qRgb(100, 101, 102));

	ArgbColorMixer<unsigned> argb_mixer;
	argb_mixer.add(qRgba(99, 100, 101, 50), 10);
	argb_mixer.add(qRgba(101, 102, 103, 50), 10);
	BOOST_CHECK(argb_mixer.mix(20) == qRgba(100, 101, 102, 50));
}

BOOST_AUTO_TEST_CASE(test_mixing_similar_colors_float)
{
	GrayColorMixer<float> gray_mixer;
	gray_mixer.add(99, 10);
	gray_mixer.add(101, 10);
	BOOST_CHECK(gray_mixer.mix(20) == 100);

	RgbColorMixer<float> rgb_mixer;
	rgb_mixer.add(qRgb(99, 100, 101), 10);
	rgb_mixer.add(qRgb(101, 102, 103), 10);
	BOOST_CHECK(rgb_mixer.mix(20) == qRgb(100, 101, 102));

	ArgbColorMixer<float> argb_mixer;
	argb_mixer.add(qRgba(99, 100, 101, 50), 10);
	argb_mixer.add(qRgba(101, 102, 103, 50), 10);
	BOOST_CHECK(argb_mixer.mix(20) == qRgba(100, 101, 102, 50));
}

BOOST_AUTO_TEST_CASE(test_mixing_colors_that_differ_only_in_alpha)
{
	ArgbColorMixer<unsigned> mixer;
	mixer.add(qRgba(99, 100, 101, 50), 10);
	mixer.add(qRgba(99, 100, 101, 150), 10);
	BOOST_CHECK(mixer.mix(20) == qRgba(99, 100, 101, 100));
}

BOOST_AUTO_TEST_CASE(test_mixing_with_transparent_white)
{
	ArgbColorMixer<unsigned> mixer;
	mixer.add(qRgba(99, 100, 101, 50), 10);
	mixer.add(qRgba(0xff, 0xff, 0xff, 0x00), 10);
	BOOST_CHECK(mixer.mix(20) == qRgba(99, 100, 101, 25));
}

BOOST_AUTO_TEST_CASE(test_mixing_with_transparent_black)
{
	ArgbColorMixer<unsigned> mixer;
	mixer.add(qRgba(99, 100, 101, 50), 10);
	mixer.add(qRgba(0x00, 0x00, 0x00, 0x00), 10);
	BOOST_CHECK(mixer.mix(20) == qRgba(99, 100, 101, 25));
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace imageproc
