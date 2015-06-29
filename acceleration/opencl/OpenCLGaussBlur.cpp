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
#include "imageproc/GaussBlur.h"
#include <QDebug>

using namespace imageproc::gauss_blur_impl;

OpenCLGrid<float> gaussBlur(
	cl::CommandQueue const& command_queue,
	cl::Program const& program,
	OpenCLGrid<float> const& src_grid,
	float h_sigma, float v_sigma,
	std::vector<cl::Event>* wait_for,
	cl::Event* event)
{
	int const width = src_grid.width();
	int const height = src_grid.height();
	cl::Context const context = command_queue.getInfo<CL_QUEUE_CONTEXT>();

	std::vector<cl::Event> deps;
	if (wait_for) {
		deps.insert(deps.end(), wait_for->begin(), wait_for->end());
	}

	cl::Event evt;

	// dst_buffer is used both as a horizontal pass destination and
	// as a final destination.
	cl::Buffer dst_buffer(
		context, CL_MEM_READ_WRITE, src_grid.totalBytesWithDifferentPadding(0)
	);
	OpenCLGrid<float> dst_grid(src_grid.withDifferentPadding(dst_buffer, 0));

	{
		// Horizontal pass.
		FilterParams const p(h_sigma);

		cl::Kernel kernel(program, "gauss_blur_1d");
		int idx = 0;
		kernel.setArg(idx++, cl_int(width));
		kernel.setArg(idx++, src_grid.buffer());
		kernel.setArg(idx++, src_grid.offset());
		kernel.setArg(idx++, src_grid.stride());
		kernel.setArg(idx++, cl_int(1));
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, dst_grid.offset());
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, cl_int(1));
		kernel.setArg(idx++, cl_float3{p.a1, p.a2, p.a3});
		kernel.setArg(idx++, cl_float(1.f / p.B));
		kernel.setArg(idx++, cl_float(p.B * p.B));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(height),
			cl::NullRange,
			&deps,
			&evt
		);
		deps.clear();
		deps.push_back(std::move(evt));
	}

	{
		// Vertical pass, from dst to itself.
		FilterParams const p(v_sigma);

		cl::Kernel kernel(program, "gauss_blur_1d");
		int idx = 0;
		kernel.setArg(idx++, cl_int(height));
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, dst_grid.offset());
		kernel.setArg(idx++, cl_int(1));
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, dst_grid.offset());
		kernel.setArg(idx++, cl_int(1));
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, cl_float3{p.a1, p.a2, p.a3});
		kernel.setArg(idx++, cl_float(1.f / p.B));
		kernel.setArg(idx++, cl_float(p.B * p.B));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(width),
			cl::NullRange,
			&deps,
			&evt
		);
		deps.clear();
		deps.push_back(std::move(evt));
	}

	if (event) {
		*event = std::move(evt);
	}

	return dst_grid;
}

OpenCLGrid<float> anisotropicGaussBlur(
	cl::CommandQueue const& command_queue,
	cl::Program const& program,
	OpenCLGrid<float> const& src_grid,
	float const dir_x, float const dir_y,
	float const dir_sigma, float const ortho_dir_sigma,
	std::vector<cl::Event>* wait_for, cl::Event* event)
{
	int const width = src_grid.width();
	int const height = src_grid.height();
	cl::Context const context = command_queue.getInfo<CL_QUEUE_CONTEXT>();

	std::vector<cl::Event> deps;
	if (wait_for) {
		deps.insert(deps.end(), wait_for->begin(), wait_for->end());
	}

	cl::Event evt;

	// dst_buffer is used both as a horizontal pass destination and
	// as a final destination.
	cl::Buffer dst_buffer(
		context, CL_MEM_READ_WRITE, src_grid.totalBytesWithDifferentPadding(1)
	);
	OpenCLGrid<float> dst_grid(src_grid.withDifferentPadding(dst_buffer, 1));

	HorizontalDecompositionParams const hdp(dir_x, dir_y, dir_sigma, ortho_dir_sigma);
	VerticalDecompositionParams const vdp(dir_x, dir_y, dir_sigma, ortho_dir_sigma);
	bool const horizontal_decomposition = hdp.sigma_x > vdp.sigma_y;

	if (horizontal_decomposition) {
		// Horizontal pass.
		FilterParams const p(hdp.sigma_x);

		cl::Kernel kernel(program, "gauss_blur_1d");
		int idx = 0;
		kernel.setArg(idx++, cl_int(width));
		kernel.setArg(idx++, src_grid.buffer());
		kernel.setArg(idx++, src_grid.offset());
		kernel.setArg(idx++, src_grid.stride());
		kernel.setArg(idx++, cl_int(1));
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, dst_grid.offset());
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, cl_int(1));
		kernel.setArg(idx++, cl_float3{p.a1, p.a2, p.a3});
		kernel.setArg(idx++, cl_float(1.f / p.B));
		kernel.setArg(idx++, cl_float(p.B * p.B));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(height),
			cl::NullRange,
			&deps,
			&evt
		);
		deps.clear();
		deps.push_back(std::move(evt));
	} else {
		// Vertical pass.
		FilterParams const p(vdp.sigma_y);

		cl::Kernel kernel(program, "gauss_blur_1d");
		int idx = 0;
		kernel.setArg(idx++, cl_int(height));
		kernel.setArg(idx++, src_grid.buffer());
		kernel.setArg(idx++, src_grid.offset());
		kernel.setArg(idx++, cl_int(1));
		kernel.setArg(idx++, src_grid.stride());
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, dst_grid.offset());
		kernel.setArg(idx++, cl_int(1));
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, cl_float3{p.a1, p.a2, p.a3});
		kernel.setArg(idx++, cl_float(1.f / p.B));
		kernel.setArg(idx++, cl_float(p.B * p.B));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(width),
			cl::NullRange,
			&deps,
			&evt
		);
		deps.clear();
		deps.push_back(std::move(evt));
	}

	{ // Padding of dst grid scope.

		cl::Kernel kernel(program, "copy_1px_padding");
		int idx = 0;
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, dst_grid.width());
		kernel.setArg(idx++, dst_grid.height());
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, dst_grid.offset());

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange((width + height)*2 + 4),
			cl::NullRange,
			&deps,
			&evt
		);
		deps.clear();
		deps.push_back(std::move(evt));
	}

#if 0
	// Here we try to choose whether to traverse a skewed line one x unit or one y unit
	// at a time based on whether the line is horizontally or vertically leaning.
	// Unfortunately, it produces larger errors in impulse response compared to a
	// more simple approach below.
	if ((horizontal_decomposition && std::abs(hdp.cot_phi) <= 1.0f) ||
		(!horizontal_decomposition && std::abs(vdp.tan_phi) > 1.0f)){
#else
	if (horizontal_decomposition) {
#endif
		// Traverse the phi-oriented line with a unit step in y direction.

		float dx; // The step in x direction.
		float sigma_phi;
		if (horizontal_decomposition) {
			dx = hdp.cot_phi;
			sigma_phi = hdp.sigma_phi;
		} else {
			dx = 1.f / vdp.tan_phi;
			sigma_phi = vdp.sigma_phi;
		}

		float const adjusted_sigma_phi = sigma_phi / std::sqrt(1.0f + dx*dx);
		FilterParams const p(adjusted_sigma_phi);

		// In the CPU version of this code we build a reference skewed line,
		// which is an array of InterpolatedCoord structures indexed by y.
		// In OpenCL version we don't construct that array explicitly, but
		// instead build its elements on demand (see get_interpolated_coord()
		// in gauss_blur.cl). We then "move" that line by adding values in
		// [min_x_offset, max_x_offset] range to InterpolatedCoord::lower_bound.
		// Now let's calculate that range.
		int min_x_offset, max_x_offset; // These don't change once computed.
		{
			int const x0 = 0;
			int const x1 = static_cast<int>(std::floor(height - 1) * dx);
			if (x0 < x1) {
				// '\'-like skewed line

				// x1 + min_x_offset == -1
				min_x_offset = -1 - x1;

				// x0 + 1 + max_x_offset == width
				max_x_offset = width - 1 - x0;
			} else {
				// '/'-like skewed line

				// x0 + min_x_offset == -1
				min_x_offset = -1 - x0;

				// x1 + 1 + max_x_offset == width
				max_x_offset = width - 1 - x1;
			}
		}

		cl::Buffer intermediate_buffer(
			context, CL_MEM_READ_WRITE, src_grid.totalBytesWithDifferentPadding(1)
		);
		OpenCLGrid<float> intermediate_grid(src_grid.withDifferentPadding(intermediate_buffer, 1));

		cl::Kernel kernel(program, "gauss_blur_h_decomp_stage1");
		int idx = 0;
		kernel.setArg(idx++, cl_int(width));
		kernel.setArg(idx++, cl_int(height));
		kernel.setArg(idx++, dst_grid.buffer()); // dst_grid plays the role of a source here.
		kernel.setArg(idx++, dst_grid.offset());
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, intermediate_grid.buffer());
		kernel.setArg(idx++, intermediate_grid.offset());
		kernel.setArg(idx++, intermediate_grid.stride());
		kernel.setArg(idx++, cl_int(min_x_offset));
		kernel.setArg(idx++, cl_float(dx));
		kernel.setArg(idx++, cl_float3{p.a1, p.a2, p.a3});
		kernel.setArg(idx++, cl_float(1.f / p.B));
		kernel.setArg(idx++, cl_float(p.B * p.B));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(max_x_offset - min_x_offset + 1),
			cl::NullRange,
			&deps,
			&evt
		);
		deps.clear();
		deps.push_back(std::move(evt));

		kernel = cl::Kernel(program, "gauss_blur_h_decomp_stage2");
		idx = 0;
		kernel.setArg(idx++, intermediate_grid.buffer());
		kernel.setArg(idx++, intermediate_grid.offset());
		kernel.setArg(idx++, intermediate_grid.stride());
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, dst_grid.offset());
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, cl_float(dx));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(width, height),
			cl::NullRange,
			&deps,
			&evt
		);
	} else {
		// Traverse the phi-oriented line with a unit step in x direction.

		float dy; // The step in y direction.
		float sigma_phi;
		if (horizontal_decomposition) {
			dy = 1.f / hdp.cot_phi;
			sigma_phi = hdp.sigma_phi;
		} else {
			dy = vdp.tan_phi;
			sigma_phi = vdp.sigma_phi;
		}

		float const adjusted_sigma_phi = sigma_phi / std::sqrt(1.0f + dy*dy);
		FilterParams const p(adjusted_sigma_phi);

		// In the CPU version of this code we build a reference skewed line,
		// which is an array of InterpolatedCoord structures indexed by x.
		// In OpenCL version we don't construct that array explicitly, but
		// instead build its elements on demand (see get_interpolated_coord()
		// in gauss_blur.cl). We then "move" that line by adding values in
		// [min_y_offset, max_y_offset] range to InterpolatedCoord::lower_bound.
		// Now let's calculate that range.
		int min_y_offset, max_y_offset; // These don't change once computed.
		{
			int const y0 = 0;
			int const y1 = static_cast<int>(std::floor(width - 1) * dy);
			if (y0 < y1) {
				// '\'-like skewed line

				// y1 + min_y_offset == -1
				min_y_offset = -1 - y1;

				// y0 + 1 + max_y_offset == height
				max_y_offset = height - 1 - y0;
			} else {
				// '/'-like skewed line

				// y0 + min_y_offset == -1
				min_y_offset = -1 - y0;

				// y1 + 1 + max_y_offset == height
				max_y_offset = height - 1 - y1;
			}
		}

		cl::Buffer intermediate_buffer(
			context, CL_MEM_READ_WRITE, src_grid.totalBytesWithDifferentPadding(1)
		);
		OpenCLGrid<float> intermediate_grid(src_grid.withDifferentPadding(intermediate_buffer, 1));

		cl::Kernel kernel(program, "gauss_blur_v_decomp_stage1");
		int idx = 0;
		kernel.setArg(idx++, cl_int(width));
		kernel.setArg(idx++, cl_int(height));
		kernel.setArg(idx++, dst_grid.buffer()); // dst_grid plays the role of a source here.
		kernel.setArg(idx++, dst_grid.offset());
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, intermediate_grid.buffer());
		kernel.setArg(idx++, intermediate_grid.offset());
		kernel.setArg(idx++, intermediate_grid.stride());
		kernel.setArg(idx++, cl_int(min_y_offset));
		kernel.setArg(idx++, cl_float(dy));
		kernel.setArg(idx++, cl_float3{p.a1, p.a2, p.a3});
		kernel.setArg(idx++, cl_float(1.f / p.B));
		kernel.setArg(idx++, cl_float(p.B * p.B));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(max_y_offset - min_y_offset + 1),
			cl::NullRange,
			&deps,
			&evt
		);
		deps.clear();
		deps.push_back(std::move(evt));

		kernel = cl::Kernel(program, "gauss_blur_v_decomp_stage2");
		idx = 0;
		kernel.setArg(idx++, intermediate_grid.buffer());
		kernel.setArg(idx++, intermediate_grid.offset());
		kernel.setArg(idx++, intermediate_grid.stride());
		kernel.setArg(idx++, dst_grid.buffer());
		kernel.setArg(idx++, dst_grid.offset());
		kernel.setArg(idx++, dst_grid.stride());
		kernel.setArg(idx++, cl_float(dy));

		command_queue.enqueueNDRangeKernel(
			kernel,
			cl::NullRange,
			cl::NDRange(width, height),
			cl::NullRange,
			&deps,
			&evt
		);
	}

	if (event) {
		*event = std::move(evt);
	}

	return dst_grid;
}
