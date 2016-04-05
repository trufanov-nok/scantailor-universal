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

#include "BinaryFill.h"
#include "Utils.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <stdexcept>

namespace opencl
{

void binaryFillRect(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<uint32_t> const& grid, QRect const& pixel_rect, imageproc::BWColor fill_color,
	std::vector<cl::Event> const* dependencies, std::vector<cl::Event>* completion_set)
{
	QRect const word_rect(pixelRectToWordRect(pixel_rect));
	if (!grid.rect().contains(word_rect)) {
		throw std::invalid_argument(
			"opencl::binaryFillRect(): Fill rectangle reaches outside of data grid"
		);
	}

	cl::Device const device = command_queue.getInfo<CL_QUEUE_DEVICE>();
	size_t const cacheline_size = device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>();
	cl_uint const fill_word = fill_color == imageproc::WHITE ? cl_uint(0) : ~cl_uint(0);

	cl::Kernel kernel(program, "binary_fill_rect");
	int idx = 0;
	kernel.setArg(idx++, cl_int(word_rect.width()));
	kernel.setArg(idx++, cl_int(word_rect.height()));
	kernel.setArg(idx++, grid.buffer());
	kernel.setArg(idx++, cl_int(grid.offset() + grid.stride() * word_rect.top() + word_rect.left()));
	kernel.setArg(idx++, grid.stride());
	kernel.setArg(idx++, fill_word);
	kernel.setArg(idx++, computeLeftRightEdgeMasks(pixel_rect));

	size_t const max_wg_items = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);

	// Try to access a cacheline worth of data horizontally.
	// Note that some devices report zero cacheline_size.
	size_t h_wg_size = std::max<size_t>(64, cacheline_size) / sizeof(uint32_t);

	// Do we exceed max_wg_items?
	h_wg_size = std::min(h_wg_size, max_wg_items);

	// Maximum possible vertical size.
	size_t v_wg_size = max_wg_items / h_wg_size;

	cl::Event evt;

	command_queue.enqueueNDRangeKernel(
		kernel,
		cl::NullRange,
		cl::NDRange(
			thisOrNextMultipleOf(word_rect.width(), h_wg_size),
			thisOrNextMultipleOf(word_rect.height(), v_wg_size)
		),
		cl::NDRange(h_wg_size, v_wg_size),
		dependencies, &evt
	);

	indicateCompletion(completion_set, evt);
}

void binaryFillFrame(
	cl::CommandQueue const& command_queue, cl::Program const& program,
	OpenCLGrid<uint32_t> const& grid, QRect const& outer_pixel_rect,
	QRect const& inner_pixel_rect, imageproc::BWColor fill_color,
	std::vector<cl::Event> const* dependencies, std::vector<cl::Event>* completion_set)
{
	if (!grid.rect().contains(pixelRectToWordRect(outer_pixel_rect))) {
		throw std::invalid_argument(
			"opencl::binaryFillFrame(): Outer rectangle reaches outside of data grid"
		);
	}

	if (!inner_pixel_rect.intersects(outer_pixel_rect)
			|| inner_pixel_rect.contains(outer_pixel_rect)) {
		indicateCompletion(completion_set, dependencies);
		return;
	}

	cl::Device const device = command_queue.getInfo<CL_QUEUE_DEVICE>();
	size_t const cacheline_size = device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE>();
	std::vector<cl::Event> events;

	cl::Kernel kernel(program, "binary_fill_rect");
	cl_uint const fill_word = fill_color == imageproc::WHITE ? cl_uint(0) : ~cl_uint(0);

	auto const enqueueKernel = [&](QRect const& pixel_rect) {

		QRect const word_rect(pixelRectToWordRect(pixel_rect));
		int idx = 0;
		kernel.setArg(idx++, cl_int(word_rect.width()));
		kernel.setArg(idx++, cl_int(word_rect.height()));
		kernel.setArg(idx++, grid.buffer());
		kernel.setArg(idx++, cl_int(grid.offset() + grid.stride() * word_rect.top() + word_rect.left()));
		kernel.setArg(idx++, grid.stride());
		kernel.setArg(idx++, fill_word);
		kernel.setArg(idx++, computeLeftRightEdgeMasks(pixel_rect));

		size_t const max_wg_items = kernel.getWorkGroupInfo<CL_KERNEL_WORK_GROUP_SIZE>(device);

		// Try to access a cacheline worth of data horizontally.
		// Note that some devices report zero cacheline_size.
		size_t h_wg_size = std::max<size_t>(64, cacheline_size) / sizeof(uint32_t);

		// Do we exceed max_wg_items?
		h_wg_size = std::min(h_wg_size, max_wg_items);

		// Maximum possible vertical size.
		size_t v_wg_size = max_wg_items / h_wg_size;

		cl::Event evt;
		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(
				thisOrNextMultipleOf(word_rect.width(), h_wg_size),
				thisOrNextMultipleOf(word_rect.height(), v_wg_size)
			),
			cl::NDRange(h_wg_size, v_wg_size),
			dependencies, &evt
		);
		events.push_back(evt);
	};

	// Top segment. Extends to left and right boundaries of outer_pixel_rect.
	if (inner_pixel_rect.top() > outer_pixel_rect.top()) {
		QRect pixel_rect(inner_pixel_rect | outer_pixel_rect);
		pixel_rect.setBottom(inner_pixel_rect.top() - 1);

		if (!pixel_rect.isEmpty()) {
			enqueueKernel(pixel_rect);
		}
	}

	// Left segment.
	if (inner_pixel_rect.left() > outer_pixel_rect.left()) {
		QRect pixel_rect(inner_pixel_rect);
		pixel_rect.setLeft(outer_pixel_rect.left());
		pixel_rect.setRight(inner_pixel_rect.left() - 1);

		if (!pixel_rect.isEmpty()) {
			enqueueKernel(pixel_rect);
		}
	}

	// Right segment.
	if (outer_pixel_rect.right() > inner_pixel_rect.right()) {
		QRect pixel_rect(inner_pixel_rect);
		pixel_rect.setLeft(inner_pixel_rect.right() + 1);
		pixel_rect.setRight(outer_pixel_rect.right());

		if (!pixel_rect.isEmpty()) {
			enqueueKernel(pixel_rect);
		}
	}

	// Bottom segment. Extends to left and right boundaries of outer_pixel_rect.
	if (outer_pixel_rect.bottom() > inner_pixel_rect.bottom()) {
		QRect pixel_rect(inner_pixel_rect | outer_pixel_rect);
		pixel_rect.setTop(inner_pixel_rect.bottom() + 1);

		if (!pixel_rect.isEmpty()) {
			enqueueKernel(pixel_rect);
		}
	}

	assert(!events.empty());
	indicateCompletion(completion_set, events);
}

} // namespace opencl
