/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015-2016  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "OpenCLAffineTransform.h"
#include "Utils.h"
#include "GridAccessor.h"
#include "PerformanceTimer.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/AffineTransform.h"
#include <QSize>
#include <QSizeF>
#include <QRectF>
#include <QColor>
#include <QTransform>
#include <Qt>
#include <QDebug>
#include <CL/cl2.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <cstdint>
#include <cmath>
#include <algorithm>

using namespace imageproc;

namespace opencl
{

namespace tests
{

class AffineTransformFixture : protected DeviceListFixture, protected ProgramBuilderFixture
{
public:
	AffineTransformFixture() {
		addSource("rgba_color_mixer.cl");
		addSource("affine_transform.cl");
	}
};

BOOST_FIXTURE_TEST_SUITE(AffineTransformTestSuite, AffineTransformFixture);

#define LOG_PERFORMANCE 0

BOOST_AUTO_TEST_CASE(test_argb)
{
	boost::random::mt19937 rng;
	boost::random::uniform_int_distribution<> dist(0, 255);
	QImage input(1000, 1000, QImage::Format_ARGB32);
	for (int y = 0; y < input.height(); ++y) {
		for (int x = 0; x < input.width(); ++x) {
			input.setPixel(x, y, qRgba(dist(rng), dist(rng), dist(rng), dist(rng)));
		}
	}

	QTransform xform;
	xform.translate(0.5 * input.width(), 0.5 * input.height());
	xform.rotate(30.0);
	xform.translate(-0.5 * input.width(), -0.5 * input.height());

	QRect const output_rect(-50, -50, 1050, 1050);
	QSizeF const min_mapping_area(0.7, 0.7);

	// We only test with assumeColor() mode, as two other modes are brittle with
	// regards to rounding errors.
	OutsidePixels const outside_pixels = OutsidePixels::assumeColor(Qt::transparent);

#if LOG_PERFORMANCE
	PerformanceTimer ptimer2;
#endif

	QImage const control = imageproc::affineTransform(
		input, xform, output_rect, outside_pixels, min_mapping_area
	);

#if LOG_PERFORMANCE
	for (int i = 0; i < 99; ++i) {
		imageproc::affineTransform(
			input, xform, output_rect, outside_pixels, min_mapping_area
		);
	}

	ptimer2.print("[affine-transform x100] Non-accelerated version:");
#endif

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

#if LOG_PERFORMANCE
		PerformanceTimer ptimer;
#endif

		QImage const output = affineTransform(
			command_queue, program, input, xform,
			output_rect, outside_pixels, min_mapping_area
		);

#if LOG_PERFORMANCE
		for (int i = 0; i < 99; ++i) {
			affineTransform(
				command_queue, program, input, xform,
				output_rect, outside_pixels, min_mapping_area
			);
		}

		ptimer.print(("[affine-transform x100] "+device.getInfo<CL_DEVICE_NAME>() + ":").c_str());
#endif

		int max_err = 0;
		rasterOpGenericXY(
			[&max_err](uint32_t argb1, uint32_t argb2, int x, int y) {
				uint32_t const argb1_pm = qPremultiply(argb1);
				uint32_t const argb2_pm = qPremultiply(argb2);
				int const alpha1 = qAlpha(argb1_pm);
				int const alpha2 = qAlpha(argb2_pm);
				int const red1 = qRed(argb1_pm);
				int const red2 = qRed(argb2_pm);
				int const green1 = qGreen(argb1_pm);
				int const green2 = qGreen(argb2_pm);
				int const blue1 = qBlue(argb1_pm);
				int const blue2 = qBlue(argb2_pm);
				int const err = std::max(
					std::max(std::abs(alpha1 - alpha2), std::abs(red1 - red2)),
					std::max(std::abs(green1 - green2), std::abs(blue1 - blue2))
				);
				if (err > max_err) {
					max_err = err;
				}
			},
			GridAccessor<uint32_t const>{
				(uint32_t const*)output.bits(), output.bytesPerLine()/4,
				output.width(), output.height()
			},
			GridAccessor<uint32_t const>{
				(uint32_t const*)control.bits(), control.bytesPerLine()/4,
				control.width(), control.height()
			}
		);

		// imageproc::affineTransform() subdivides each pixel into a 32x32 grid and
		// then applies integer arithmetics on those sub-pixels.
		// Therefore, the maximum error is 1/32 aka 8/256.
		// Not sure why, but we are getting slightly larger errors up to 15/256.
		BOOST_CHECK_LE(max_err, 15);

	} // for (device)
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace opencl
