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

#include "Copy.h"
#include "Utils.h"
#include <cstddef>
#include <cstdint>
#include <algorithm>

namespace opencl
{

OpenCLGrid<float> copy(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<float> const& src_grid, int dst_padding,
	std::vector<cl::Event>* wait_for, cl::Event* event)
{
	int const width = src_grid.width();
	int const height = src_grid.height();
	cl::Context const context = command_queue.getInfo<CL_QUEUE_CONTEXT>();
	cl::Device const device = command_queue.getInfo<CL_QUEUE_DEVICE>();
	size_t const max_wg_size = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
	size_t const cacheline_size = device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>();

	size_t const h_wg_size = std::min<size_t>(max_wg_size, cacheline_size / sizeof(float));
	size_t const v_wg_size = max_wg_size / h_wg_size;

	cl::Buffer dst_buffer(
		context, CL_MEM_READ_WRITE, src_grid.totalBytesWithDifferentPadding(dst_padding)
	);
	OpenCLGrid<float> dst_grid(src_grid.withDifferentPadding(dst_buffer, dst_padding));

	cl::Kernel kernel(program, "copy_float_grid");
	int idx = 0;
	kernel.setArg(idx++, src_grid.width());
	kernel.setArg(idx++, src_grid.height());
	kernel.setArg(idx++, src_grid.buffer());
	kernel.setArg(idx++, src_grid.offset());
	kernel.setArg(idx++, src_grid.stride());
	kernel.setArg(idx++, dst_grid.buffer());
	kernel.setArg(idx++, dst_grid.offset());
	kernel.setArg(idx++, dst_grid.stride());

	command_queue.enqueueNDRangeKernel(
		kernel,
		cl::NullRange,
		cl::NDRange(thisOrNextMultipleOf(width, h_wg_size), thisOrNextMultipleOf(height, v_wg_size)),
		cl::NDRange(h_wg_size, v_wg_size),
		wait_for,
		event
	);

	return dst_grid;
}

} // namespace opencl
