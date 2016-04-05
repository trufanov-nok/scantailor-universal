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

#include "OpenCLSavGolFilter.h"
#include "Utils.h"
#include "imageproc/SavGolKernel.h"
#include <stdexcept>
#include <string>
#include <cmath>
#include <cstddef>
#include <array>

namespace opencl
{

namespace
{

int calcNumTerms(int const hor_degree, int const vert_degree)
{
	return (hor_degree + 1) * (vert_degree + 1);
}

} // anonymous namespace

imageproc::GrayImage savGolFilter(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	imageproc::GrayImage const& src, QSize const& window_size,
	int const hor_degree, int const vert_degree)
{
	if (hor_degree < 0 || vert_degree < 0) {
		throw std::invalid_argument("savGolFilter: invalid polynomial degree");
	}
	if (window_size.isEmpty()) {
		throw std::invalid_argument("savGolFilter: invalid window size");
	}

	if (calcNumTerms(hor_degree, vert_degree)
			> window_size.width() * window_size.height()) {
		throw std::invalid_argument(
			"savGolFilter: order is too big for that window");
	}

	if (src.width() < window_size.width() || src.height() < window_size.height()) {
		return src;
	}

	cl::Context const context = command_queue.getInfo<CL_QUEUE_CONTEXT>();
	cl::Device const device = command_queue.getInfo<CL_QUEUE_DEVICE>();

	auto const cl_format_byte = cl::ImageFormat(CL_R, CL_UNORM_INT8);
	auto const cl_format_float = cl::ImageFormat(CL_R, CL_FLOAT);

	// Create two images on the device.
	cl::Image2D byte_texture(context, CL_MEM_READ_WRITE, cl_format_byte, src.width(), src.height());
	cl::Image2D float_texture(context, CL_MEM_READ_WRITE, cl_format_float, src.width(), src.height());

	std::vector<cl::Event> events;
	cl::Event evt;

	// Copy the source image to byte_texture.
	std::array<size_t, 3> const origin{0, 0, 0};
	std::array<size_t, 3> const region{(size_t)src.width(), (size_t)src.height(), 1};
	command_queue.enqueueWriteImage(
		byte_texture, CL_FALSE, origin, region, src.stride(), 0, (void*)src.data(), &events, &evt
	);
	indicateCompletion(&events, evt);

	cl::Kernel kernel(program, "sav_gol_filter_1d");
	size_t const max_wg_items = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);

	size_t const v_wg_size = static_cast<size_t>(std::sqrt((double)max_wg_items));
	size_t const h_wg_size = max_wg_items / v_wg_size;

	// Horizontal pass: byte_texture -> float_texture
	{
		int const k_origin_x = window_size.width() >> 1;
		imageproc::SavGolKernel const h_kernel(
			QSize(window_size.width(), 1), QPoint(k_origin_x, 0), hor_degree, 0
		);

		cl::Buffer coeffs_buffer(
			context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
			window_size.width() * sizeof(float), (void*)h_kernel.data()
		);

		int idx = 0;
		kernel.setArg(idx++, byte_texture);
		kernel.setArg(idx++, float_texture);
		kernel.setArg(idx++, cl_float2{1.f / (src.width() - 1), 1.f / (src.height() - 1)});
		kernel.setArg(idx++, cl_float2{1.f / (src.width() - 1), 0.f                     });
		kernel.setArg(idx++, coeffs_buffer);
		kernel.setArg(idx++, cl_int(-k_origin_x));
		kernel.setArg(idx++, cl_int(window_size.width() - 1 - k_origin_x));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(
				thisOrNextMultipleOf(src.width(), h_wg_size),
				thisOrNextMultipleOf(src.height(), v_wg_size)
			),
			cl::NDRange(h_wg_size, v_wg_size), &events, &evt
		);
		indicateCompletion(&events, evt);
	}

	// Vertical pass: float_texture -> byte_texture
	{
		int const k_origin_y = window_size.height() >> 1;
		imageproc::SavGolKernel const v_kernel(
			QSize(1, window_size.height()), QPoint(0, k_origin_y), 0, vert_degree
		);

		cl::Buffer coeffs_buffer(
			context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
			window_size.height() * sizeof(float), (void*)v_kernel.data()
		);

		int idx = 0;
		kernel.setArg(idx++, float_texture);
		kernel.setArg(idx++, byte_texture);
		kernel.setArg(idx++, cl_float2{1.f / (src.width() - 1), 1.f / (src.height() - 1)});
		kernel.setArg(idx++, cl_float2{                    0.f, 1.f / (src.height() - 1)});
		kernel.setArg(idx++, coeffs_buffer);
		kernel.setArg(idx++, cl_int(-k_origin_y));
		kernel.setArg(idx++, cl_int(window_size.height() - 1 - k_origin_y));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(
				thisOrNextMultipleOf(src.width(), h_wg_size),
				thisOrNextMultipleOf(src.height(), v_wg_size)
			),
			cl::NDRange(h_wg_size, v_wg_size), &events, &evt
		);
		indicateCompletion(&events, evt);
	}

	// byte_texture -> destination image
	imageproc::GrayImage dst(src.size());
	command_queue.enqueueReadImage(
		byte_texture, CL_FALSE, origin, region, dst.stride(), 0, dst.data(), &events, &evt
	);
	indicateCompletion(&events, evt);

	cl::WaitForEvents(events);

	return dst;
}

} // namespace opencl
