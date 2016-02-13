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

#include "BinaryFill.h"
#include "OpenCLGrid.h"
#include "Utils.h"
#include "../Utils.h"
#include "PerformanceTimer.h"
#include "imageproc/BinaryImage.h"
#include <QRect>
#include <QPoint>
#include <CL/cl2.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <string>
#include <vector>

namespace opencl
{

namespace tests
{

class BinaryFillFixture : protected DeviceListFixture, protected ProgramBuilderFixture
{
public:
	BinaryFillFixture() {
		addSource("binary_word_mask.cl");
		addSource("binary_fill_rect.cl");
	}
};

BOOST_FIXTURE_TEST_SUITE(BinaryFillTestSuite, BinaryFillFixture)

#define LOG_PERFORMANCE 0

BOOST_AUTO_TEST_CASE(test_fill_rect_correctness)
{
	imageproc::BinaryImage const orig_image(randomBinaryImage(1001, 999));

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

		std::vector<cl::Event> events;

		boost::random::mt19937 rng;
		boost::random::uniform_int_distribution<> dist4(0, 4);
		boost::random::uniform_int_distribution<> dist31(0, 31);

		for (int iteration = 0; iteration < 1000; ++iteration) {
			OpenCLGrid<uint32_t> grid(binaryImageToOpenCLGrid(orig_image, command_queue));

			imageproc::BWColor const fill_color = dist4(rng) & 1
					? imageproc::BLACK : imageproc::WHITE;
			int const left = dist4(rng) * 32 + dist31(rng);
			int const right = orig_image.width() - 1 - dist4(rng) * 32 - dist31(rng);
			QRect const rect(QPoint(left, 1), QPoint(right, grid.height() - 1));

			imageproc::BinaryImage control(orig_image);
			control.fill(rect, fill_color);

			binaryFillRect(command_queue, program, grid, rect, fill_color, nullptr, &events);
			cl::WaitForEvents(events);

			imageproc::BinaryImage const result(
				openCLGridToBinaryImage(grid, orig_image.width(), command_queue)
			);

			BOOST_REQUIRE(result == control);

		} // for (iteration)

	} // for (device)
}

BOOST_AUTO_TEST_CASE(test_fill_rect_performance)
{
	int const width = 4000;
	int const height = 5000;
	imageproc::BinaryImage binary_image(width, height);
	QRect const fill_rect(binary_image.rect().adjusted(1, 1, -1, -1));
	imageproc::BWColor const fill_color = imageproc::BLACK;

#if LOG_PERFORMANCE
	PerformanceTimer ptimer1;
	for (int i = 0; i < 100; ++i) {
		binary_image.fill(fill_rect, fill_color);
	}
	ptimer1.print("[binary-fill-rect x100] Non-accelerated version:");
#endif

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

		std::vector<cl::Event> events;

		cl::Buffer const buf(context, CL_MEM_READ_WRITE, (height * (width + 31) / 32) * sizeof(uint32_t));
		OpenCLGrid<uint32_t> grid(buf, (width + 31) / 32, height, 0);

		// The first call will cause kernel compilation, so don't include it in time measurement.
		binaryFillRect(command_queue, program, grid, fill_rect, fill_color, nullptr, &events);
		cl::WaitForEvents(events);

#if LOG_PERFORMANCE
		PerformanceTimer ptimer2;
		for (int i = 0; i < 100; ++i) {
			binaryFillRect(command_queue, program, grid, fill_rect, fill_color, &events, &events);
		}
		cl::WaitForEvents(events);
		ptimer2.print(("[binary-fill-rect x100] "+device.getInfo<CL_DEVICE_NAME>() + ": ").c_str());
#endif
	} // for (device)
}

BOOST_AUTO_TEST_CASE(test_fill_frame_correctness)
{
	imageproc::BinaryImage const orig_image(randomBinaryImage(1001, 999));

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

		std::vector<cl::Event> events;

		boost::random::mt19937 rng;
		boost::random::uniform_int_distribution<> dist4(0, 4);
		boost::random::uniform_int_distribution<> dist31(0, 31);

		for (int iteration = 0; iteration < 1000; ++iteration) {
			OpenCLGrid<uint32_t> grid(binaryImageToOpenCLGrid(orig_image, command_queue));

			imageproc::BWColor const fill_color = dist4(rng) & 1
					? imageproc::BLACK : imageproc::WHITE;

			int adj[8];
			for (int i = 0; i < 8; ++i) {
				adj[i] = dist4(rng) * 32 + dist31(rng);
			}

			QRect outer_rect(orig_image.rect());
			outer_rect.adjust(adj[0], adj[1], -adj[2], -adj[3]);

			QRect inner_rect(outer_rect);
			inner_rect.adjust(adj[4], adj[5], -adj[6], -adj[7]);

			imageproc::BinaryImage control(orig_image);
			control.fillFrame(outer_rect, inner_rect, fill_color);

			binaryFillFrame(
				command_queue, program, grid, outer_rect,
				inner_rect, fill_color, nullptr, &events
			);
			cl::WaitForEvents(events);

			imageproc::BinaryImage const result(
				openCLGridToBinaryImage(grid, orig_image.width(), command_queue)
			);

			BOOST_REQUIRE(result == control);

		} // for (iteration)

	} // for (device)
}

BOOST_AUTO_TEST_CASE(test_fill_frame_performance)
{
	int const width = 4000;
	int const height = 5000;
	imageproc::BinaryImage binary_image(width, height);
	QRect const outer_rect(binary_image.rect().adjusted(1, 1, -1, -1));
	QRect const inner_rect(outer_rect.center(), outer_rect.center());
	imageproc::BWColor const fill_color = imageproc::BLACK;

#if LOG_PERFORMANCE
	PerformanceTimer ptimer1;
	for (int i = 0; i < 100; ++i) {
		binary_image.fillFrame(outer_rect, inner_rect, fill_color);
	}
	ptimer1.print("[binary-fill-frame x100] Non-accelerated version:");
#endif

	for (cl::Device const& device : m_devices) {
		if (isDodgyDevice(device)) {
			continue;
		}
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

		std::vector<cl::Event> events;

		cl::Buffer const buf(context, CL_MEM_READ_WRITE, (height * (width + 31) / 32) * sizeof(uint32_t));
		OpenCLGrid<uint32_t> grid(buf, (width + 31) / 32, height, 0);

		// The first call will cause kernel compilation, so don't include it in time measurement.
		binaryFillFrame(
			command_queue, program, grid, outer_rect,
			inner_rect, fill_color, nullptr, &events
		);
		cl::WaitForEvents(events);

#if LOG_PERFORMANCE
		PerformanceTimer ptimer2;
		for (int i = 0; i < 100; ++i) {
			binaryFillFrame(
				command_queue, program, grid, outer_rect,
				inner_rect, fill_color, &events, &events
			);
		}
		cl::WaitForEvents(events);
		ptimer2.print(("[binary-fill-frame x100] "+device.getInfo<CL_DEVICE_NAME>() + ": ").c_str());
#endif
	} // for (device)
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace opencl
