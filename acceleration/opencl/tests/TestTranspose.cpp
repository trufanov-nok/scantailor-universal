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

#include "Transpose.h"
#include "Grid.h"
#include "OpenCLGrid.h"
#include "Utils.h"
#include "PerformanceTimer.h"
#include <CL/cl.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <string>
#include <vector>

namespace opencl
{

namespace tests
{

class TransposeFixture : protected DeviceListFixture, protected ProgramBuilderFixture
{
public:
	TransposeFixture() {
		addSource("transpose_grid.cl");
	}
};

BOOST_FIXTURE_TEST_SUITE(TransposeTestSuite, TransposeFixture);

#define LOG_PERFORMANCE 0

BOOST_AUTO_TEST_CASE(test)
{
	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));
		try {
			program.build();
		} catch (cl::Error const&) {
			BOOST_FAIL(program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
		}

		Grid<float> input(1001, 999);
		for (int y = 0; y < input.height(); ++y) {
			for (int x = 0; x < input.width(); ++x) {
				input(x, y) = float(x * y);
			}
		}

		std::vector<cl::Event> deps;
		cl::Event evt;

		// Copy input to device.
		cl::Buffer const src_buffer(context, CL_MEM_READ_ONLY, input.totalBytes());
		OpenCLGrid<float> src_grid(src_buffer, input);
		command_queue.enqueueWriteBuffer(
			src_grid.buffer(), CL_FALSE, 0, input.totalBytes(), input.paddedData(), &deps, &evt
		);
		deps.clear();
		deps.push_back(evt);

#if LOG_PERFORMANCE
		evt.wait();
		PerformanceTimer ptimer;
#endif

		OpenCLGrid<float> dst_grid = transpose(
			command_queue, program, src_grid, /*dst_padding=*/0, &deps, &evt
		);
		deps.clear();
		deps.push_back(evt);

#if LOG_PERFORMANCE
		evt.wait();
		for (int i = 0; i < 99; ++i) {
			transpose(
				command_queue, program, src_grid, /*dst_padding=*/0, &deps, &evt
			);
			deps.clear();
			deps.push_back(evt);
			evt.wait();
		}

		ptimer.print(("[transpose x100] "+device.getInfo<CL_DEVICE_NAME>() + ": ").c_str());
#endif

		// Copy output from device.
		Grid<float> output(dst_grid.toUninitializedHostGrid());
		command_queue.enqueueReadBuffer(
			dst_grid.buffer(), CL_FALSE, 0, dst_grid.totalBytes(), output.paddedData(), &deps, &evt
		);
		deps.clear();
		deps.push_back(evt);

		BOOST_REQUIRE_EQUAL(output.width(), input.height());
		BOOST_REQUIRE_EQUAL(output.height(), input.width());

		evt.wait();

		bool correct = true;
		for (int y = 0; y < input.height() && correct; ++y) {
			for (int x = 0; x < input.width(); ++x) {
				if (input(x, y) != output(y, x)) {
					correct = false;
					break;
				}
			}
		}
		BOOST_CHECK(correct);

	} // for (device)
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace opencl
