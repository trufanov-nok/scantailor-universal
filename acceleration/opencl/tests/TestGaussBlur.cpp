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

#include "OpenCLGaussBlur.h"
#include "Grid.h"
#include "OpenCLGrid.h"
#include "VecNT.h"
#include "Utils.h"
#include "PerformanceTimer.h"
#include "imageproc/GaussBlur.h"
#include "imageproc/Constants.h"
#include <CL/cl.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <string>
#include <vector>
#include <cmath>

namespace opencl
{

namespace tests
{

class GaussBlurFixture : protected DeviceListFixture, protected ProgramBuilderFixture
{
public:
	GaussBlurFixture() {
		addSource("transpose_grid.cl");
		addSource("copy_1px_padding.cl");
		addSource("gauss_blur.cl");
	}
};

BOOST_FIXTURE_TEST_SUITE(GaussBlurTestSuite, GaussBlurFixture);

#define LOG_PERFORMANCE 0

BOOST_AUTO_TEST_CASE(test_axis_aligned_gauss_blur)
{
	float const h_sigma = 15.f;
	float const v_sigma = 7.f;

	boost::random::mt19937 rng;
	boost::random::uniform_int_distribution<> dist(-1000, 1000);
	Grid<float> input(1001, 999);
	for (int y = 0; y < input.height(); ++y) {
		for (int x = 0; x < input.width(); ++x) {
			input(x, y) = float(dist(rng)) / 500.f;
		}
	}

	Grid<float> control(input.width(), input.height());
	imageproc::gaussBlurGeneric(
		QSize(input.width(), input.height()), h_sigma, v_sigma,
		input.data(), input.stride(), [](float val) { return val; },
		control.data(), control.stride(), [](float& dst, float src) { dst = src; }
	);

#if LOG_PERFORMANCE
	PerformanceTimer ptimer2;

	for (int i = 0; i < 99; ++i) {
		imageproc::gaussBlurGeneric(
			QSize(input.width(), input.height()), h_sigma, v_sigma,
			input.data(), input.stride(), [](float val) { return val; },
			control.data(), control.stride(), [](float& dst, float src) { dst = src; }
		);
	}

	ptimer2.print("[aligned-gauss x100] Non-accelerated version:");
#endif

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program = buildProgram(context);

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

		OpenCLGrid<float> dst_grid = gaussBlur(
			command_queue, program, src_grid, h_sigma, v_sigma, &deps, &evt
		);
		deps.clear();
		deps.push_back(evt);

#if LOG_PERFORMANCE
		evt.wait();
		for (int i = 0; i < 99; ++i) {
			gaussBlur(command_queue, program, src_grid, h_sigma, v_sigma, &deps, &evt);
			deps.clear();
			deps.push_back(evt);
			evt.wait();
		}

		ptimer.print(("[aligned-gauss x100] "+device.getInfo<CL_DEVICE_NAME>() + ":").c_str());
#endif

		// Copy output from device.
		Grid<float> output(dst_grid.toUninitializedHostGrid());
		command_queue.enqueueReadBuffer(
			dst_grid.buffer(), CL_FALSE, 0, dst_grid.totalBytes(), output.paddedData(), &deps, &evt
		);
		deps.clear();
		deps.push_back(evt);

		BOOST_REQUIRE_EQUAL(output.width(), input.width());
		BOOST_REQUIRE_EQUAL(output.height(), input.height());

		evt.wait();

		bool correct = true;
		for (int y = 0; y < output.height() && correct; ++y) {
			for (int x = 0; x < output.width(); ++x) {
				if (std::abs(output(x, y) - control(x, y)) > 1e-3f) {
					correct = false;
					break;
				}
			}
		}
		BOOST_CHECK(correct);

	} // for (device)
}

BOOST_AUTO_TEST_CASE(test_oriented_gauss_blur)
{
	float const dir_sigma = 15.f;
	float const ortho_dir_sigma = 7.f;

	std::vector<Vec2f> directions;
	for (int angle_deg = 0; angle_deg < 360; angle_deg += 18) {
		float const angle_rad = angle_deg * imageproc::constants::DEG2RAD;
		directions.emplace_back(std::cos(angle_rad), std::sin(angle_rad));
	}

	boost::random::mt19937 rng;
	boost::random::uniform_int_distribution<> dist(-1000, 1000);
	Grid<float> input(1001, 999);
	for (int y = 0; y < input.height(); ++y) {
		for (int x = 0; x < input.width(); ++x) {
			input(x, y) = float(dist(rng)) / 500.f;
		}
	}

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program = buildProgram(context);

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

		for (Vec2f const dir : directions) {
			OpenCLGrid<float> dst_grid = anisotropicGaussBlur(
				command_queue, program, src_grid, dir[0], dir[1],
				dir_sigma, ortho_dir_sigma, &deps, &evt
			);
			deps.clear();
			deps.push_back(evt);

			// Copy output from device.
			Grid<float> output(dst_grid.toUninitializedHostGrid());
			command_queue.enqueueReadBuffer(
				dst_grid.buffer(), CL_FALSE, 0, dst_grid.totalBytes(), output.paddedData(), &deps, &evt
			);
			deps.clear();
			deps.push_back(evt);

			BOOST_REQUIRE_EQUAL(output.width(), input.width());
			BOOST_REQUIRE_EQUAL(output.height(), input.height());

			evt.wait();

			Grid<float> control(input.width(), input.height());
			imageproc::anisotropicGaussBlurGeneric(
				QSize(input.width(), input.height()), dir[0], dir[1],
				dir_sigma, ortho_dir_sigma,
				input.data(), input.stride(), [](float val) { return val; },
				control.data(), control.stride(), [](float& dst, float src) { dst = src; }
			);

			bool correct = true;
			for (int y = 0; y < output.height() && correct; ++y) {
				for (int x = 0; x < output.width(); ++x) {
					if (std::abs(output(x, y) - control(x, y)) > 1e-3f) {
						correct = false;
						break;
					}
				}
			}
			BOOST_CHECK(correct);

		} // for (directions)

#if LOG_PERFORMANCE
		PerformanceTimer ptimer;

		for (int i = 0; i < 100; ++i) {
			float const angle_rad = double(i) * 3.6 * imageproc::constants::DEG2RAD;
			Vec2f const dir(std::cos(angle_rad), std::sin(angle_rad));

			anisotropicGaussBlur(
				command_queue, program, src_grid, dir[0], dir[1],
				dir_sigma, ortho_dir_sigma, &deps, &evt
			);
			deps.clear();
			deps.push_back(evt);
			evt.wait();
		}

		ptimer.print(("[oriented-gauss x100] "+device.getInfo<CL_DEVICE_NAME>() + ":").c_str());
#endif
	} // for (device)
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace opencl
