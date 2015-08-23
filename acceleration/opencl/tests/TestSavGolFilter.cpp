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

#include "OpenCLSavGolFilter.h"
#include "Utils.h"
#include "PerformanceTimer.h"
#include "imageproc/GrayImage.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/SavGolFilter.h"
#include <QSize>
#include <QDebug>
#include <CL/cl.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <cstdint>
#include <cstdlib>

using namespace imageproc;

namespace opencl
{

namespace tests
{

class SavGolFilterFixture : protected DeviceListFixture, protected ProgramBuilderFixture
{
public:
	SavGolFilterFixture() {
		addSource("sav_gol_filter.cl");
	}
};

BOOST_FIXTURE_TEST_SUITE(SavGolFilterTestSuite, SavGolFilterFixture);

#define LOG_PERFORMANCE 0

BOOST_AUTO_TEST_CASE(test1)
{
	boost::random::mt19937 rng;
	boost::random::uniform_int_distribution<> dist(0, 255);
	GrayImage input(QSize(1000, 1000));
	rasterOpGeneric([&dist, &rng](uint8_t& px) {
		px = dist(rng);
	}, input);

#if LOG_PERFORMANCE
	PerformanceTimer ptimer1;
#endif

	GrayImage const control = imageproc::savGolFilter(input, QSize(5, 5), 3, 3);

#if LOG_PERFORMANCE
	for (int i = 0; i < 99; ++i) {
		imageproc::savGolFilter(input, QSize(5, 5), 3, 3);
	}

	ptimer1.print("[sav-gol-filter x100] Non-accelerated version:");
#endif

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

#if LOG_PERFORMANCE
		PerformanceTimer ptimer2;
#endif

		GrayImage const output = savGolFilter(command_queue, program, input, QSize(5, 5), 3, 3);

#if LOG_PERFORMANCE
		for (int i = 0; i < 99; ++i) {
			savGolFilter(command_queue, program, input, QSize(5, 5), 3, 3);
		}

		ptimer2.print(("[sav-gol-filter x100] "+device.getInfo<CL_DEVICE_NAME>() + ":").c_str());
#endif

		int max_err = 0;
		int max_err_x = 0;
		int max_err_y = 0;
		rasterOpGenericXY(
			[&max_err, &max_err_x, &max_err_y](int px1, int px2, int x, int y) {
				int const err = std::abs(px1 - px2);
				if (err > max_err) {
					max_err = err;
					max_err_x = x;
					max_err_y = y;
				}
			},
			output, control
		);

		//BOOST_TEST_MESSAGE("SavGolFilter max error at: (" << max_err_x << ", " << max_err_y << ")");
		BOOST_CHECK_LE(max_err, 1);

	} // for (device)
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace opencl
