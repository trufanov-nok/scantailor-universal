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

#include "OpenCLAcceleratedOperations.h"
#include "OpenCLGrid.h"
#include "OpenCLGaussBlur.h"
#include "OpenCLTextFilterBank.h"
#include "VecNT.h"
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QDebug>
#include <deque>
#include <new>
#include <stdexcept>
#include <cstddef>
#include <utility>

/**
 * @note This constructor reports problems by throwing an exception.
 */
OpenCLAcceleratedOperations::OpenCLAcceleratedOperations(
	cl::Context const& context,
	std::shared_ptr<AcceleratableOperations> const& fallback)
:	m_context(context)
,	m_devices(context.getInfo<CL_CONTEXT_DEVICES>())
,	m_commandQueue(m_context, m_devices.front(), 0)
,	m_ptrFallback(fallback)
{
	// The order of these source files may be important, some
	// sources may be using entities declared / defined in other
	// source files. Note that we don't use #include directives
	// in OpenCL kernel code.
	static char const* const device_sources[] = {
		"fill_float_grid.cl",
		"copy_1px_padding.cl",
		"gauss_blur.cl",
		"text_filter_bank_combine.cl"
	};

	std::deque<QByteArray> sources;
	cl::Program::Sources source_accessors;

	for (char const* file_name : device_sources) {
		QString resource_name(":/device_code/");
		resource_name += QLatin1String(file_name);
		QFile file(resource_name);
		if (!file.open(QIODevice::ReadOnly)) {
			throw std::runtime_error("Failed to read file: "+resource_name.toStdString());
		}
		sources.push_back(file.readAll());
		QByteArray const& data = sources.back();
		source_accessors.push_back(std::make_pair(data.data(), data.size()));
	}

	cl::Program program(m_context, source_accessors);

	try {
		program.build();
	} catch (cl::Error const&) {
		qDebug() << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(m_devices.front()).c_str();
		throw std::runtime_error("Failed to build OpenCL program");
	}

	m_program = std::move(program);
}

OpenCLAcceleratedOperations::~OpenCLAcceleratedOperations()
{
}

Grid<float>
OpenCLAcceleratedOperations::anisotropicGaussBlur(
	Grid<float> const& src, float dir_x, float dir_y,
	float dir_sigma, float ortho_dir_sigma) const
{
	try {
		return anisotropicGaussBlurUnguarded(src, dir_x, dir_y, dir_sigma, ortho_dir_sigma);
	} catch (cl::Error const& e) {
		if (e.err() == CL_OUT_OF_HOST_MEMORY) {
			throw std::bad_alloc();
		}
		qDebug() << "OpenCL error: " << e.what();
		return m_ptrFallback->anisotropicGaussBlur(src, dir_x, dir_y, dir_sigma, ortho_dir_sigma);
	}
}

Grid<float>
OpenCLAcceleratedOperations::anisotropicGaussBlurUnguarded(
	Grid<float> const& src, float dir_x, float dir_y,
	float dir_sigma, float ortho_dir_sigma) const
{
	if (src.isNull()) {
		// OpenCL doesn't like zero-size buffers.
		return Grid<float>();
	}

	std::vector<cl::Event> deps;
	cl::Event evt;

	cl::Buffer const src_buffer(m_context, CL_MEM_READ_ONLY, src.totalBytes());
	OpenCLGrid<float> src_grid(src_buffer, src);

	m_commandQueue.enqueueWriteBuffer(
		src_grid.buffer(), CL_FALSE, 0, src.totalBytes(), src.paddedData(), &deps, &evt
	);
	deps.clear();
	deps.push_back(std::move(evt));

	auto dst_grid = ::anisotropicGaussBlur(
		m_commandQueue, m_program, src_grid,
		dir_x, dir_y, dir_sigma, ortho_dir_sigma, &deps, &evt
	);

	Grid<float> dst(dst_grid.toUninitializedHostGrid());

	m_commandQueue.enqueueReadBuffer(
		dst_grid.buffer(), CL_FALSE, 0, dst_grid.totalBytes(), dst.paddedData(), &deps, &evt
	);

	evt.wait();

	return std::move(dst);
}

Grid<float>
OpenCLAcceleratedOperations::textFilterBank(
	Grid<float> const& src, std::vector<Vec2f> const& directions,
	std::vector<Vec2f> const& sigmas, float shoulder_length) const
{
	try {
		return textFilterBankUnguarded(src, directions, sigmas, shoulder_length);
	} catch (cl::Error const& e) {
		if (e.err() == CL_OUT_OF_HOST_MEMORY) {
			throw std::bad_alloc();
		}
		qDebug() << "OpenCL error: " << e.what();
		return m_ptrFallback->textFilterBank(src, directions, sigmas, shoulder_length);
	}
}

Grid<float>
OpenCLAcceleratedOperations::textFilterBankUnguarded(
	Grid<float> const& src, std::vector<Vec2f> const& directions,
	std::vector<Vec2f> const& sigmas, float shoulder_length) const
{
	if (src.isNull()) {
		// OpenCL doesn't like zero-size buffers.
		return Grid<float>();
	}

	std::vector<cl::Event> deps;
	cl::Event evt;

	cl::Buffer const src_buffer(m_context, CL_MEM_READ_ONLY, src.totalBytes());
	OpenCLGrid<float> src_grid(src_buffer, src);

	m_commandQueue.enqueueWriteBuffer(
		src_grid.buffer(), CL_FALSE, 0, src.totalBytes(), src.paddedData(), &deps, &evt
	);
	deps.clear();
	deps.push_back(std::move(evt));

	auto dst_grid = ::textFilterBank(
		m_commandQueue, m_program, src_grid,
		directions, sigmas, shoulder_length, &deps, &evt
	);

	Grid<float> dst(dst_grid.toUninitializedHostGrid());

	m_commandQueue.enqueueReadBuffer(
		dst_grid.buffer(), CL_FALSE, 0, dst_grid.totalBytes(), dst.paddedData(), &deps, &evt
	);

	evt.wait();

	return std::move(dst);
}
