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

#include "BinaryRasterOp.h"
#include "OpenCLGrid.h"
#include "Utils.h"
#include "PerformanceTimer.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/RasterOp.h"
#include <QSize>
#include <QRect>
#include <QPoint>
#include <CL/cl2.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/test/auto_unit_test.hpp>
#include <string>
#include <vector>

using namespace imageproc;

namespace opencl
{

namespace tests
{

class BinaryRasterOpFixture : protected DeviceListFixture, protected ProgramBuilderFixture
{
public:
	BinaryRasterOpFixture() {
		addSource("binary_word_mask.cl");
		addSource("binary_raster_op.cl");
	}
};

BOOST_FIXTURE_TEST_SUITE(BinaryRasterOpTestSuite, BinaryRasterOpFixture)

#define LOG_PERFORMANCE 0

BOOST_AUTO_TEST_CASE(test_rop_correctness)
{
	BinaryImage const orig_src(randomBinaryImage(1001, 999));
	BinaryImage const orig_dst(randomBinaryImage(999, 1001));

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

		std::vector<cl::Event> events;

		boost::random::mt19937 rng;
		boost::random::uniform_int_distribution<> dist4(0, 4);
		boost::random::uniform_int_distribution<> dist31(0, 31);

		for (int iteration = 0; iteration < 1000; ++iteration) {
			OpenCLGrid<uint32_t> const src_grid(binaryImageToOpenCLGrid(orig_src, command_queue));
			OpenCLGrid<uint32_t> const dst_grid(binaryImageToOpenCLGrid(orig_dst, command_queue));

			BWColor const fill_color = dist4(rng) & 1 ? BLACK : WHITE;

			int offset[6];
			for (int i = 0; i < 6; ++i) {
				offset[i] = dist4(rng) * 32 + dist31(rng);
			}

			QSize const size(500 + offset[0], 500 + offset[1]);
			QRect const src_rect(QPoint(offset[2], offset[3]), size);
			QRect const dst_rect(QPoint(offset[4], offset[5]), size);

			BinaryImage control(orig_dst);
			rasterOp<RopXor<RopSrc, RopDst>>(control, dst_rect, orig_src, src_rect.topLeft());

			binaryRasterOp(
				command_queue, program, "binary_rop_kernel_xor",
				src_grid, src_rect, dst_grid, dst_rect, nullptr, &events
			);
			cl::WaitForEvents(events);

			BinaryImage const result(
				openCLGridToBinaryImage(dst_grid, orig_dst.width(), command_queue)
			);

			BOOST_REQUIRE(result == control);

		} // for (iteration)

	} // for (device)
}

BOOST_AUTO_TEST_CASE(test_rop_performance)
{
	int const width = 5000;
	int const height = 5000;
	QRect const src_rect(1, 0, width - 1, height);
	QRect const dst_rect(0, 0, width - 1, height);
	BinaryImage const src_image(width, height);
	BinaryImage dst_image(width, height);

	PerformanceTimer ptimer1;
	for (int i = 0; i < 100; ++i) {
		rasterOp<RopXor<RopSrc, RopDst>>(dst_image, dst_rect, src_image, src_rect.topLeft());
	}
#if LOG_PERFORMANCE
	ptimer1.print("[binary-raster-op x100] Non-accelerated version:");
#endif

	std::vector<cl::Event> events;

	for (cl::Device const& device : m_devices) {
		cl::Context context(device);
		cl::CommandQueue command_queue(context, device);
		cl::Program program(buildProgram(context));

		OpenCLGrid<uint32_t> const src_grid(binaryImageToOpenCLGrid(src_image, command_queue));
		OpenCLGrid<uint32_t> const dst_grid(binaryImageToOpenCLGrid(dst_image, command_queue));

		// The first call will cause kernel compilation, so don't include it in time measurement.
		binaryRasterOp(
			command_queue, program, "binary_rop_kernel_xor",
			src_grid, src_rect, dst_grid, dst_rect, nullptr, &events
		);
		cl::WaitForEvents(events);

		PerformanceTimer ptimer2;
		for (int i = 0; i < 100; ++i) {
			binaryRasterOp(
				command_queue, program, "binary_rop_kernel_xor",
				src_grid, src_rect, dst_grid, dst_rect, nullptr, &events
			);
			cl::WaitForEvents(events);
		}
#if LOG_PERFORMANCE
		ptimer2.print(("[binary-raster-op x100] "+device.getInfo<CL_DEVICE_NAME>() + ": ").c_str());
#endif
	} // for (device)
}

BOOST_AUTO_TEST_SUITE_END();

} // namespace tests

} // namespace opencl
