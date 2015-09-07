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

#include "RenderPolynomialSurface.h"
#include "Utils.h"
#include "PerformanceTimer.h"
#include "imageproc/GrayImage.h"
#include "imageproc/PolynomialSurface.h"
#include "imageproc/RasterOpGeneric.h"
#include <QPoint>
#include <QPointF>
#include <QtGlobal>
#include <CL/cl.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <string>
#include <cmath>

using namespace imageproc;

namespace opencl
{

namespace tests
{

class RenderPolynomialSurfaceFixture : protected DeviceListFixture, protected ProgramBuilderFixture
{
public:
	RenderPolynomialSurfaceFixture() {
		addSource("render_polynomial_surface.cl");
	}
};

BOOST_FIXTURE_TEST_SUITE(RenderPolynomialSurfaceTestSuite, RenderPolynomialSurfaceFixture);

#define LOG_PERFORMANCE 0

BOOST_AUTO_TEST_CASE(test)
{
	// Prepare an image to be approximated by a polynomial surface.
	GrayImage input(QSize(201, 201));
	QPoint const center = input.rect().center();
	rasterOpGenericXY(
		[center](uint8_t& pixel, int x, int y) {
			QPoint const to_center = center - QPoint(x, y);
			double const proximity = QPoint::dotProduct(to_center, to_center);
			pixel = qBound(0, qRound(std::exp(-proximity / (50.0*50.0)) * 100.0), 255);
		},
		input
	);

	// Approximate the image with a polynomial surface.
	PolynomialSurface const ps(5, 5, input);

#if LOG_PERFORMANCE
	PerformanceTimer ptimer2;
#endif

	GrayImage const control(ps.render(QSize(1001, 999)));

#if LOG_PERFORMANCE
	for (int i = 0; i < 99; ++i) {
		ps.render(control.size());
	}

	ptimer2.print("[ps-render x100] Non-accelerated version:");
#endif

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

#if LOG_PERFORMANCE
		PerformanceTimer ptimer;
#endif

		GrayImage const output = renderPolynomialSurface(
			command_queue, program, control.width(), control.height(), ps.coeffs()
		);

#if LOG_PERFORMANCE
		for (int i = 0; i < 99; ++i) {
			renderPolynomialSurface(
				command_queue, program, control.width(), control.height(), ps.coeffs()
			);
		}
		ptimer.print(("[ps-render x100] "+device.getInfo<CL_DEVICE_NAME>() + ": ").c_str());
#endif

		BOOST_REQUIRE_EQUAL(output.width(), control.width());
		BOOST_REQUIRE_EQUAL(output.height(), control.height());

		int max_err = 0;
		rasterOpGeneric(
			[&max_err](int output, int control) {
				int const err = std::abs(output - control);
				if (err > max_err) {
					max_err = err;
				}
			},
			output, control
		);

		BOOST_CHECK_LE(max_err, 1);

	} // for (device)
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace opencl
