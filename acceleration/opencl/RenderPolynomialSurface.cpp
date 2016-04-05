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

#include "RenderPolynomialSurface.h"
#include "Utils.h"
#include "AlignedArray.h"
#include <cstddef>
#include <array>
#include <algorithm>
#include <vector>

using namespace imageproc;

namespace opencl
{

imageproc::GrayImage renderPolynomialSurface(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	int const width, int const height, Eigen::MatrixXd const& coeffs)
{
	if (width <= 0 || height <= 0) {
		return GrayImage();
	}

	cl::Context const context = command_queue.getInfo<CL_QUEUE_CONTEXT>();
	cl::Device const device = command_queue.getInfo<CL_QUEUE_DEVICE>();
	size_t const cacheline_size = device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>();

	std::vector<float> flattened_coeffs(coeffs.cols() * coeffs.rows());
	for (int i = 0; i < coeffs.rows(); ++i) {
		for (int j = 0; j < coeffs.cols(); ++j) {
			flattened_coeffs[i * coeffs.cols() + j] = static_cast<float>(coeffs(i, j));
		}
	}

	cl::Buffer coeffs_buffer(
		context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
		flattened_coeffs.size() * sizeof(float), flattened_coeffs.data()
	);

	size_t dst_buffer_stride = width;
	if (cacheline_size) {
		// This improves performance on Intel integrated graphics.
		dst_buffer_stride = thisOrNextMultipleOf(width, cacheline_size);
	}
	cl::Buffer dst_buffer(
		context, CL_MEM_WRITE_ONLY, dst_buffer_stride * height
	);

	cl::Kernel kernel(program, "render_polynomial_surface");
	size_t const max_wg_items = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);

	// Try to access a cacheline worth of data horizontally.
	// Note that some devices report zero cacheline_size.
	size_t h_wg_size = std::max<size_t>(64, cacheline_size);

	// Do we exceed max_wg_items?
	h_wg_size = std::min(h_wg_size, max_wg_items);

	// Maximum possible vertical size.
	size_t v_wg_size = max_wg_items / h_wg_size;

	int idx = 0;
	kernel.setArg(idx++, cl_int(width));
	kernel.setArg(idx++, cl_int(height));
	kernel.setArg(idx++, dst_buffer);
	kernel.setArg(idx++, cl_int(dst_buffer_stride));
	kernel.setArg(idx++, coeffs_buffer);
	kernel.setArg(idx++, cl_int(coeffs.cols()));
	kernel.setArg(idx++, cl_int(coeffs.rows()));

	cl::Event evt;

	command_queue.enqueueNDRangeKernel(
		kernel,
		cl::NullRange,
		cl::NDRange(thisOrNextMultipleOf(width, h_wg_size), thisOrNextMultipleOf(height, v_wg_size)),
		cl::NDRange(h_wg_size, v_wg_size),
		nullptr,
		&evt
	);

	std::vector<cl::Event> deps{evt};

	GrayImage dst(QSize(width, height));

	std::array<size_t, 3> const region{(size_t)width, (size_t)height, 1};
	std::array<size_t, 3> const zero_offset{0, 0, 0};
	command_queue.enqueueReadBufferRect(
		dst_buffer, CL_TRUE, zero_offset, zero_offset,
		region, dst_buffer_stride, 0, dst.stride(), 0, dst.data(), &deps
	);

	return dst;
}

} // namespace opencl
