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

#include "BinaryRasterOp.h"
#include "Utils.h"
#include <QDebug>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <cassert>

namespace opencl
{

namespace
{

} // anonymous namespace

void binaryRasterOp(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	std::string const& kernel_name_base,
	OpenCLGrid<uint32_t> const& src_grid, QRect const& src_pixel_rect,
	OpenCLGrid<uint32_t> const& dst_grid, QRect const& dst_pixel_rect,
	std::vector<cl::Event> const* dependencies,
	std::vector<cl::Event>* completion_set)
{
	if (src_pixel_rect.size() != dst_pixel_rect.size()) {
		throw std::invalid_argument("opencl::binaryRasterOp(): Rectangles have different size");
	}

	if (src_pixel_rect.isEmpty()) {
		indicateCompletion(completion_set, dependencies);
		return;
	}

	QRect const src_word_rect(pixelRectToWordRect(src_pixel_rect));
	QRect const dst_word_rect(pixelRectToWordRect(dst_pixel_rect));

	if (!src_grid.rect().contains(src_word_rect)) {
		throw std::invalid_argument(
			"opencl::binaryRasterOp(): Source rectangle reaches outside of source grid"
		);
	}
	if (!dst_grid.rect().contains(dst_word_rect)) {
		throw std::invalid_argument(
			"opencl::binaryRasterOp(): Destination rectangle reaches outside of destination grid"
		);
	}

	cl::Device const device = command_queue.getInfo<CL_QUEUE_DEVICE>();
	uint64_t const local_mem_size = device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>();
	size_t const cacheline_size = device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>();

	cl::Event completion_evt;

	int const src_first_bit = src_pixel_rect.left() & 31;
	int const dst_first_bit = dst_pixel_rect.left() & 31;

	if (src_first_bit == dst_first_bit) {
		// Aligned case.
		cl::Kernel kernel(program, (kernel_name_base+"_aligned").c_str());
		size_t const max_wg_items = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);

		// Try to access a cacheline worth of data horizontally.
		// Note that some devices report zero cacheline_size.
		size_t h_wg_size = std::max<size_t>(64, cacheline_size) / sizeof(uint32_t);

		// Do we exceed max_wg_items?
		h_wg_size = std::min(h_wg_size, max_wg_items);

		// Maximum possible vertical size.
		size_t v_wg_size = max_wg_items / h_wg_size;

		// For some reason, limiting workgroup size helps with performance.
		v_wg_size = std::min<size_t>(v_wg_size, 16);

		int idx = 0;
		kernel.setArg(idx++, cl_int(dst_word_rect.width()));
		kernel.setArg(idx++, cl_int(dst_word_rect.height()));
		kernel.setArg(idx++, src_grid.buffer());
		kernel.setArg(idx++, cl_int(src_grid.offset() +
			src_grid.stride() * src_word_rect.top() + src_word_rect.left()));
		kernel.setArg(idx++, src_grid.stride());
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, cl_int(dst_grid.offset() +
			dst_grid.stride() * dst_word_rect.top() + dst_word_rect.left()));
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, computeLeftRightEdgeMasks(dst_pixel_rect));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(
				thisOrNextMultipleOf(dst_word_rect.width(), h_wg_size),
				thisOrNextMultipleOf(dst_word_rect.height(), v_wg_size)
			),
			cl::NDRange(h_wg_size, v_wg_size),
			dependencies, &completion_evt
		);

	} else {
		// Misaligned case.
		char const* const kernel_suffix = src_first_bit < dst_first_bit ? "_src_dst" : "_dst_src";
		cl::Kernel kernel(program, (kernel_name_base + kernel_suffix).c_str());

		size_t const max_wg_items = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);

		// Try to access a cacheline worth of data horizontally.
		// Note that some devices report zero cacheline_size.
		size_t h_wg_size = std::max<size_t>(64, cacheline_size) / sizeof(uint32_t);

		// Do we exceed max_wg_items?
		h_wg_size = std::min(h_wg_size, max_wg_items);

		// Do we exceed local memory? We'll be using (h_wg_size+1)*(v_wg_size)*sizeof(uint32_t)
		// bytes of local memory.
		h_wg_size = std::min<size_t>(h_wg_size, local_mem_size / sizeof(uint32_t) - 1);

		// Maximum possible vertical size.
		size_t v_wg_size = max_wg_items / h_wg_size;

		// Do we exceed local memory?
		v_wg_size = std::min<size_t>(v_wg_size, local_mem_size / ((h_wg_size + 1) * sizeof(uint32_t)));

		// For some reason, limiting workgroup size helps with performance.
		v_wg_size = std::min<size_t>(v_wg_size, 16);

		int idx = 0;
		if (src_first_bit > dst_first_bit) {
			kernel.setArg(idx++, cl_int(src_word_rect.width()));
		}
		kernel.setArg(idx++, cl_int(dst_word_rect.width()));
		kernel.setArg(idx++, cl_int(dst_word_rect.height()));
		kernel.setArg(idx++, src_grid.buffer());
		kernel.setArg(idx++, cl_int(src_grid.offset() +
			src_grid.stride() * src_word_rect.top() + src_word_rect.left()));
		kernel.setArg(idx++, src_grid.stride());
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, cl_int(dst_grid.offset() +
			dst_grid.stride() * dst_word_rect.top() + dst_word_rect.left()));
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, computeLeftRightEdgeMasks(dst_pixel_rect));
		kernel.setArg(idx++, cl_int((dst_first_bit - src_first_bit) & 31));
		kernel.setArg(idx++, cl::Local((h_wg_size + 1) * v_wg_size * sizeof(uint32_t)));
		kernel.setArg(idx++, cl_int(h_wg_size + 1));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(
				thisOrNextMultipleOf(dst_word_rect.width(), h_wg_size),
				thisOrNextMultipleOf(dst_word_rect.height(), v_wg_size)
			),
			cl::NDRange(h_wg_size, v_wg_size),
			dependencies, &completion_evt
		);
	}

	indicateCompletion(completion_set, completion_evt);
}

} // namespace opencl
