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
#include "OpenCLDewarp.h"
#include "OpenCLAffineTransform.h"
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

namespace opencl
{

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
		"fill_grid.cl",
		"copy_grid.cl",
		"transpose_grid.cl",
		"copy_1px_padding.cl",
		"gauss_blur.cl",
		"text_filter_bank_combine.cl",
		"rgba_color_mixer.cl",
		"affine_transform.cl",
		"dewarp.cl"
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
OpenCLAcceleratedOperations::gaussBlur(
	Grid<float> const& src, float h_sigma, float v_sigma) const
{
	try {
		return gaussBlurUnguarded(src, h_sigma, v_sigma);
	} catch (cl::Error const& e) {
		if (e.err() == CL_OUT_OF_HOST_MEMORY) {
			throw std::bad_alloc();
		}
		qDebug() << "OpenCL error: " << e.what();
		return m_ptrFallback->gaussBlur(src, h_sigma, v_sigma);
	}
}

Grid<float>
OpenCLAcceleratedOperations::gaussBlurUnguarded(
	Grid<float> const& src, float h_sigma, float v_sigma) const
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
	deps.push_back(evt);

	auto dst_grid = opencl::gaussBlur(
		m_commandQueue, m_program, src_grid, h_sigma, v_sigma, &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	Grid<float> dst(dst_grid.toUninitializedHostGrid());

	m_commandQueue.enqueueReadBuffer(
		dst_grid.buffer(), CL_FALSE, 0, dst_grid.totalBytes(), dst.paddedData(), &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	evt.wait();

	return std::move(dst);
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
	deps.push_back(evt);

	auto dst_grid = opencl::anisotropicGaussBlur(
		m_commandQueue, m_program, src_grid,
		dir_x, dir_y, dir_sigma, ortho_dir_sigma, &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	Grid<float> dst(dst_grid.toUninitializedHostGrid());

	m_commandQueue.enqueueReadBuffer(
		dst_grid.buffer(), CL_FALSE, 0, dst_grid.totalBytes(), dst.paddedData(), &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	evt.wait();

	return std::move(dst);
}

std::pair<Grid<float>, Grid<uint8_t>>
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

std::pair<Grid<float>, Grid<uint8_t>>
OpenCLAcceleratedOperations::textFilterBankUnguarded(
	Grid<float> const& src, std::vector<Vec2f> const& directions,
	std::vector<Vec2f> const& sigmas, float shoulder_length) const
{
	if (src.isNull()) {
		// OpenCL doesn't like zero-size buffers.
		return std::make_pair(Grid<float>(), Grid<uint8_t>());
	}

	std::vector<cl::Event> deps;
	cl::Event evt;

	cl::Buffer const src_buffer(m_context, CL_MEM_READ_ONLY, src.totalBytes());
	OpenCLGrid<float> src_grid(src_buffer, src);

	m_commandQueue.enqueueWriteBuffer(
		src_grid.buffer(), CL_FALSE, 0, src.totalBytes(), src.paddedData(), &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	std::pair<OpenCLGrid<float>, OpenCLGrid<uint8_t>> dst = opencl::textFilterBank(
		m_commandQueue, m_program, src_grid,
		directions, sigmas, shoulder_length, &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	Grid<float> accum(dst.first.toUninitializedHostGrid());

	m_commandQueue.enqueueReadBuffer(
		dst.first.buffer(), CL_FALSE, 0, accum.totalBytes(), accum.paddedData(), &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	Grid<uint8_t> direction_map(dst.second.toUninitializedHostGrid());

	m_commandQueue.enqueueReadBuffer(
		dst.second.buffer(), CL_FALSE, 0, direction_map.totalBytes(),
		direction_map.paddedData(), &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	evt.wait();

	return std::make_pair(std::move(accum), std::move(direction_map));
}

QImage
OpenCLAcceleratedOperations::dewarp(
	QImage const& src, QSize const& dst_size,
	dewarping::CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& background_color,
	QSizeF const& min_mapping_area) const
{
	try {
		return dewarpUnguarded(
			src, dst_size, distortion_model,
			model_domain, background_color, min_mapping_area
		);
	} catch (cl::Error const& e) {
		if (e.err() == CL_OUT_OF_HOST_MEMORY) {
			throw std::bad_alloc();
		}
		qDebug() << "OpenCL error: " << e.what();
		return m_ptrFallback->dewarp(
			src, dst_size, distortion_model,
			model_domain, background_color, min_mapping_area
		);
	}
}

QImage
OpenCLAcceleratedOperations::dewarpUnguarded(
	QImage const& src, QSize const& dst_size,
	dewarping::CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& background_color,
	QSizeF const& min_mapping_area) const
{
	return opencl::dewarp(
		m_commandQueue, m_program, src, dst_size,
		distortion_model, model_domain, background_color, min_mapping_area
	);
}

QImage
OpenCLAcceleratedOperations::affineTransform(
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, imageproc::OutsidePixels const& outside_pixels,
	QSizeF const& min_mapping_area) const
{
	try {
		return affineTransformUnguarded(
			src, xform, dst_rect, outside_pixels, min_mapping_area
		);
	} catch (cl::Error const& e) {
		if (e.err() == CL_OUT_OF_HOST_MEMORY) {
			throw std::bad_alloc();
		}
		qDebug() << "OpenCL error: " << e.what();
		return m_ptrFallback->affineTransform(
			src, xform, dst_rect, outside_pixels, min_mapping_area
		);
	}
}

QImage
OpenCLAcceleratedOperations::affineTransformUnguarded(
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, imageproc::OutsidePixels const& outside_pixels,
	QSizeF const& min_mapping_area) const
{
	return opencl::affineTransform(
		m_commandQueue, m_program, src, xform, dst_rect, outside_pixels, min_mapping_area
	);
}

} // namespace opencl
