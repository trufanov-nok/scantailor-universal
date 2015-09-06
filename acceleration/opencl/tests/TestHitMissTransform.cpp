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

#include "HitMissTransform.h"
#include "OpenCLGrid.h"
#include "Utils.h"
#include "../Utils.h"
#include "PerformanceTimer.h"
#include "GridAccessor.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/Morphology.h"
#include <QSize>
#include <QRect>
#include <QPoint>
#include <CL/cl.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <string>
#include <vector>
#include <cstdint>

using namespace imageproc;

namespace opencl
{

namespace tests
{

class HitMissTransformFixture : protected DeviceListFixture, protected ProgramBuilderFixture
{
public:
	HitMissTransformFixture() {
		addSource("binary_word_mask.cl");
		addSource("binary_fill_rect.cl");
		addSource("binary_raster_op.cl");
	}
};

BOOST_FIXTURE_TEST_SUITE(HitMissTransformTestSuite, HitMissTransformFixture)

#define LOG_PERFORMANCE 0

BOOST_AUTO_TEST_CASE(test_hit_miss_replace_correctness)
{
	static int const inp[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 1, 0, 0, 0,
		0, 0, 1, 1, 1, 0, 1, 1, 0,
		0, 0, 1, 0, 1, 1, 1, 1, 0,
		0, 0, 1, 1, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0
	};

	static int const control[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 1, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 1, 1, 1, 1, 1, 1, 0,
		0, 0, 1, 0, 1, 1, 1, 1, 0,
		0, 0, 1, 1, 0, 1, 1, 1, 0,
		0, 0, 1, 0, 0, 1, 1, 1, 0,
		0, 0, 0, 0, 0, 1, 1, 1, 0
	};

	static char const pattern[] =
		" - "
		"X+X"
		"XXX";

	BinaryImage inp_image(9, 9);
	rasterOpGeneric(
		[](BWPixelProxy dst, uint32_t src) {
			dst = src;
		},
		inp_image, GridAccessor<int const>{inp, 9, 9, 9}
	);

	BinaryImage control_image(9, 9);
	rasterOpGeneric(
		[](BWPixelProxy dst, uint32_t src) {
			dst = src;
		},
		control_image, GridAccessor<int const>{control, 9, 9, 9}
	);

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

		std::vector<cl::Event> events;

		OpenCLGrid<uint32_t> const work_grid(binaryImageToOpenCLGrid(inp_image, command_queue));
		OpenCLGrid<uint32_t> const tmp_grid(binaryImageToOpenCLGrid(inp_image, command_queue));

		opencl::hitMissReplaceInPlace(
			command_queue, program, work_grid, inp_image.width(),
			WHITE, tmp_grid, pattern, 3, 3, nullptr, &events
		);
		cl::WaitForEvents(events);

		BinaryImage const result_image(
			openCLGridToBinaryImage(work_grid, inp_image.width(), command_queue)
		);

		BOOST_REQUIRE(result_image == control_image);

	} // for (device)
}

BOOST_AUTO_TEST_CASE(test_hit_miss_replace_performance)
{
	static char const pattern[] =
		" - "
		"X+X"
		"XXX";

	int const width = 4000;
	int const height = 5000;
	BinaryImage image(width, height);

	PerformanceTimer ptimer1;
	for (int i = 0; i < 100; ++i) {
		imageproc::hitMissReplaceInPlace(image, WHITE, pattern, 3, 3);
	}
#if LOG_PERFORMANCE
	ptimer1.print("[hit-miss-replace x100] Non-accelerated version:");
#endif

	std::vector<cl::Event> events;

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		if (isDodgyDevice(device)) {
			continue;
		}

		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

		OpenCLGrid<uint32_t> const grid(binaryImageToOpenCLGrid(image, command_queue));
		OpenCLGrid<uint32_t> const tmp(binaryImageToOpenCLGrid(image, command_queue));

		// The first call will cause kernel compilation, so don't include it in time measurement.
		opencl::hitMissReplaceInPlace(
			command_queue, program, grid, image.width(),
			WHITE, tmp, pattern, 3, 3, nullptr, &events
		);
		cl::WaitForEvents(events);

		PerformanceTimer ptimer2;
		for (int i = 0; i < 100; ++i) {
			opencl::hitMissReplaceInPlace(
				command_queue, program, grid, image.width(),
				WHITE, tmp, pattern, 3, 3, &events, &events
			);
		}
		cl::WaitForEvents(events);
#if LOG_PERFORMANCE
		ptimer2.print(("[hit-miss-replace x100] "+device.getInfo<CL_DEVICE_NAME>() + ": ").c_str());
#endif
	} // for (device)
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace opencl
