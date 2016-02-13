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

#include "OpenCLAcceleratedOperations.h"
#include "OpenCLGrid.h"
#include "OpenCLGaussBlur.h"
#include "OpenCLTextFilterBank.h"
#include "OpenCLDewarp.h"
#include "OpenCLAffineTransform.h"
#include "OpenCLSavGolFilter.h"
#include "RenderPolynomialSurface.h"
#include "HitMissTransform.h"
#include "Utils.h"
#include "VecNT.h"
#include <QFile>
#include <QString>
#include <QByteArray>
#include <QDebug>
#include <new>
#include <algorithm>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
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
	// The order of these source files may be important, as some
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
		"dewarp.cl",
		"render_polynomial_surface.cl",
		"sav_gol_filter.cl",
		"binary_word_mask.cl",
		"binary_fill_rect.cl",
		"binary_raster_op.cl"
	};

	cl::Program::Sources sources;
	sources.reserve(std::distance(std::begin(device_sources), std::end(device_sources)));

	for (char const* file_name : device_sources) {
		QString resource_name(":/device_code/");
		resource_name += QLatin1String(file_name);
		QFile file(resource_name);
		if (!file.open(QIODevice::ReadOnly)) {
			throw std::runtime_error("Failed to read file: "+resource_name.toStdString());
		}
		sources.push_back(file.readAll().toStdString());
	}

	cl::Program program(m_context, sources);

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

	std::vector<cl::Event> events;
	cl::Event evt;

	cl::Buffer const src_buffer(m_context, CL_MEM_READ_ONLY, src.totalBytes());
	OpenCLGrid<float> src_grid(src_buffer, src);

	m_commandQueue.enqueueWriteBuffer(
		src_grid.buffer(), CL_FALSE, 0, src.totalBytes(), src.paddedData(), &events, &evt
	);
	indicateCompletion(&events, evt);

	auto dst_grid = opencl::gaussBlur(
		m_commandQueue, m_program, src_grid, h_sigma, v_sigma, &events, &events
	);

	Grid<float> dst(dst_grid.toUninitializedHostGrid());
	m_commandQueue.enqueueReadBuffer(
		dst_grid.buffer(), CL_FALSE, 0, dst_grid.totalBytes(), dst.paddedData(), &events, &evt
	);
	indicateCompletion(&events, evt);

	cl::WaitForEvents(events);

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

	std::vector<cl::Event> events;
	cl::Event evt;

	cl::Buffer const src_buffer(m_context, CL_MEM_READ_ONLY, src.totalBytes());
	OpenCLGrid<float> src_grid(src_buffer, src);

	m_commandQueue.enqueueWriteBuffer(
		src_grid.buffer(), CL_FALSE, 0, src.totalBytes(), src.paddedData(), &events, &evt
	);
	indicateCompletion(&events, evt);

	auto dst_grid = opencl::anisotropicGaussBlur(
		m_commandQueue, m_program, src_grid,
		dir_x, dir_y, dir_sigma, ortho_dir_sigma, &events, &events
	);

	Grid<float> dst(dst_grid.toUninitializedHostGrid());
	m_commandQueue.enqueueReadBuffer(
		dst_grid.buffer(), CL_FALSE, 0, dst_grid.totalBytes(), dst.paddedData(), &events, &evt
	);
	indicateCompletion(&events, evt);

	cl::WaitForEvents(events);

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

	std::vector<cl::Event> events;
	cl::Event evt;

	cl::Buffer const src_buffer(m_context, CL_MEM_READ_ONLY, src.totalBytes());
	OpenCLGrid<float> src_grid(src_buffer, src);

	m_commandQueue.enqueueWriteBuffer(
		src_grid.buffer(), CL_FALSE, 0, src.totalBytes(), src.paddedData(), &events, &evt
	);
	indicateCompletion(&events, evt);

	std::pair<OpenCLGrid<float>, OpenCLGrid<uint8_t>> dst = opencl::textFilterBank(
		m_commandQueue, m_program, src_grid,
		directions, sigmas, shoulder_length, &events, &events
	);

	Grid<float> accum(dst.first.toUninitializedHostGrid());
	m_commandQueue.enqueueReadBuffer(
		dst.first.buffer(), CL_FALSE, 0, accum.totalBytes(), accum.paddedData(), &events, &evt
	);
	indicateCompletion(&events, evt);

	Grid<uint8_t> direction_map(dst.second.toUninitializedHostGrid());
	m_commandQueue.enqueueReadBuffer(
		dst.second.buffer(), CL_FALSE, 0, direction_map.totalBytes(),
		direction_map.paddedData(), &events, &evt
	);
	indicateCompletion(&events, evt);

	cl::WaitForEvents(events);

	return std::make_pair(std::move(accum), std::move(direction_map));
}

QImage
OpenCLAcceleratedOperations::dewarp(
	QImage const& src, QSize const& dst_size,
	dewarping::CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& background_color,
	float min_density, float max_density,
	QSizeF const& min_mapping_area) const
{
	try {
		return dewarpUnguarded(
			src, dst_size, distortion_model,
			model_domain, background_color,
			min_density, max_density, min_mapping_area
		);
	} catch (cl::Error const& e) {
		if (e.err() == CL_OUT_OF_HOST_MEMORY) {
			throw std::bad_alloc();
		}
		qDebug() << "OpenCL error: " << e.what();
		return m_ptrFallback->dewarp(
			src, dst_size, distortion_model,
			model_domain, background_color,
			min_density, max_density, min_mapping_area
		);
	}
}

QImage
OpenCLAcceleratedOperations::dewarpUnguarded(
	QImage const& src, QSize const& dst_size,
	dewarping::CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& background_color,
	float min_density, float max_density,
	QSizeF const& min_mapping_area) const
{
	return opencl::dewarp(
		m_commandQueue, m_program, src, dst_size,
		distortion_model, model_domain, background_color,
		min_density, max_density, min_mapping_area
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

imageproc::GrayImage
OpenCLAcceleratedOperations::renderPolynomialSurface(
	imageproc::PolynomialSurface const& surface, int width, int height)
{
	try {
		return renderPolynomialSurfaceUnguarded(surface, width, height);
	} catch (cl::Error const& e) {
		if (e.err() == CL_OUT_OF_HOST_MEMORY) {
			throw std::bad_alloc();
		}
		qDebug() << "OpenCL error: " << e.what();
		return m_ptrFallback->renderPolynomialSurface(surface, width, height);
	}
}

imageproc::GrayImage
OpenCLAcceleratedOperations::renderPolynomialSurfaceUnguarded(
	imageproc::PolynomialSurface const& surface, int width, int height)
{
	return opencl::renderPolynomialSurface(
		m_commandQueue, m_program, width, height, surface.coeffs()
	);
}

imageproc::GrayImage
OpenCLAcceleratedOperations::savGolFilter(
	imageproc::GrayImage const& src, QSize const& window_size,
	int hor_degree, int vert_degree)
{
	try {
		return savGolFilterUnguarded(src, window_size, hor_degree, vert_degree);
	} catch (cl::Error const& e) {
		if (e.err() == CL_OUT_OF_HOST_MEMORY) {
			throw std::bad_alloc();
		}
		qDebug() << "OpenCL error: " << e.what();
		return m_ptrFallback->savGolFilter(src, window_size, hor_degree, vert_degree);
	}
}

imageproc::GrayImage
OpenCLAcceleratedOperations::savGolFilterUnguarded(
	imageproc::GrayImage const& src, QSize const& window_size,
	int hor_degree, int vert_degree)
{
	if (m_devices.front().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU) {
		// The OpenCL implementation of Savitzly-Golay filter is significantly
		// slower when executed on CPU compared to a non-OpenCL version.
		// Therefore, we invoke a non-OpenCL version here.
		return m_ptrFallback->savGolFilter(src, window_size, hor_degree, vert_degree);
	}

	return opencl::savGolFilter(
		m_commandQueue, m_program, src, window_size, hor_degree, vert_degree
	);
}

void
OpenCLAcceleratedOperations::hitMissReplaceInPlace(
	imageproc::BinaryImage& img, imageproc::BWColor img_surroundings,
	std::vector<Grid<char>> const& patterns)
{
	try {
		return hitMissReplaceInPlaceUnguarded(img, img_surroundings, patterns);
	} catch (cl::Error const& e) {
		if (e.err() == CL_OUT_OF_HOST_MEMORY) {
			throw std::bad_alloc();
		}
		qDebug() << "OpenCL error: " << e.what();
		return m_ptrFallback->hitMissReplaceInPlace(img, img_surroundings, patterns);
	}
}

void
OpenCLAcceleratedOperations::hitMissReplaceInPlaceUnguarded(
	imageproc::BinaryImage& img, imageproc::BWColor img_surroundings,
	std::vector<Grid<char>> const& patterns)
{
	if (img.isNull() || patterns.empty()) {
		// OpenCL doesn't like zero-size buffers.
		return;
	}

	// A bug in Intel driver makes this function hang.
	bool force_fallback = isDodgyDevice(m_devices.front());

	// The OpenCL implementation of hit-miss transform is significantly
	// slower when executed on CPU compared to a non-OpenCL version.
	force_fallback |= m_devices.front().getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;

	if (force_fallback) {
		return m_ptrFallback->hitMissReplaceInPlace(img, img_surroundings, patterns);
	}

	std::vector<cl::Event> deps;
	cl::Event evt;

	cl::Buffer const work_buffer(
		m_context, CL_MEM_READ_WRITE, img.wordsPerLine() * img.height() * sizeof(uint32_t));
	OpenCLGrid<uint32_t> work_grid(work_buffer, img.wordsPerLine(), img.height(), /*padding=*/0);

	cl::Buffer const tmp_buffer(
		m_context, CL_MEM_READ_WRITE, img.wordsPerLine() * img.height() * sizeof(uint32_t));
	OpenCLGrid<uint32_t> tmp_grid(tmp_buffer, img.wordsPerLine(), img.height(), /*padding=*/0);

	// Copy from host memory into work_grid.
	m_commandQueue.enqueueWriteBuffer(
		work_grid.buffer(), CL_FALSE, 0, work_grid.totalBytes(), img.data(), &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	for (Grid<char> const& pattern : patterns) {
		if (pattern.stride() != pattern.width()) {
			throw std::invalid_argument(
				"NonAcceleratedOperations::hitMissReplaceInPlace: "
				"patterns with extended stride are not supported"
			);
		}

		opencl::hitMissReplaceInPlace(
			m_commandQueue, m_program, work_grid, img.width(), img_surroundings,
			tmp_grid, pattern.data(), pattern.width(), pattern.height(), &deps, &deps
		);
	}

	// Copy from work_grid to host memory.
	m_commandQueue.enqueueReadBuffer(
		work_grid.buffer(), CL_FALSE, 0, work_grid.totalBytes(), img.data(), &deps, &evt
	);
	deps.clear();
	deps.push_back(evt);

	evt.wait();
}

} // namespace opencl
