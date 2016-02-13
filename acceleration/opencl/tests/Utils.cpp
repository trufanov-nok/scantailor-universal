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

#include "Utils.h"
#include <QString>
#include <QFile>
#include <QDebug>
#include <QtGlobal>
#include <array>
#include <stdexcept>
#include <utility>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <algorithm>

static void initQtResources()
{
	// Q_INIT_RESOURCE has to be invoked from global namespace.
	Q_INIT_RESOURCE(resources);
}

static void cleanupQtResources()
{
	// Q_CLEANUP_RESOURCE has to be invoked from global namespace.
	Q_CLEANUP_RESOURCE(resources);
}

namespace opencl
{

namespace tests
{

DeviceListFixture::DeviceListFixture()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);
	for (cl::Platform const& platform : platforms) {
		std::vector<cl::Device> devices;
		platform.getDevices(CL_DEVICE_TYPE_ALL, &devices);
		m_devices.insert(m_devices.end(), devices.begin(), devices.end());
	}
}

ProgramBuilderFixture::ProgramBuilderFixture()
{
	initQtResources();
}

ProgramBuilderFixture::~ProgramBuilderFixture()
{
	cleanupQtResources();
}

void
ProgramBuilderFixture::addSource(char const* source_fname)
{
	QString resource_name(":/device_code/");
	resource_name += source_fname;
	QFile file(resource_name);
	if (!file.open(QIODevice::ReadOnly)) {
		throw std::runtime_error("Failed to read file: "+resource_name.toStdString());
	}
	m_sources.push_back(file.readAll().toStdString());
}

cl::Program
ProgramBuilderFixture::buildProgram(cl::Context const& context) const
{
	cl::Program program(context, m_sources);

	try {
		program.build();
	} catch (cl::Error const&) {
		auto devices = context.getInfo<CL_CONTEXT_DEVICES>();
		qDebug() << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices.front()).c_str();
		throw std::runtime_error("Failed to build OpenCL program");
	}

	return program;
}

imageproc::BinaryImage randomBinaryImage(int width, int height)
{
	imageproc::BinaryImage image(width, height);
	uint32_t* pword = image.data();
	uint32_t* const end = pword + image.height() * image.wordsPerLine();
	for (; pword != end; ++pword) {
		uint32_t const w1 = rand() % (1 << 16);
		uint32_t const w2 = rand() % (1 << 16);
		*pword = (w1 << 16) | w2;
	}
	return image;
}

OpenCLGrid<uint32_t> binaryImageToOpenCLGrid(
	imageproc::BinaryImage const& image, cl::CommandQueue const& command_queue)
{
	cl::Context const context = command_queue.getInfo<CL_QUEUE_CONTEXT>();
	size_t const totalBytes = image.height() * image.wordsPerLine() * sizeof(uint32_t);

	cl::Buffer const buf(context, CL_MEM_READ_WRITE, totalBytes);
	OpenCLGrid<uint32_t> grid(buf, image.wordsPerLine(), image.height(), 0);
	command_queue.enqueueWriteBuffer(buf, CL_TRUE, 0, totalBytes, image.data());

	return grid;
}

imageproc::BinaryImage openCLGridToBinaryImage(
	OpenCLGrid<uint32_t> const& grid, int pixel_width,
	cl::CommandQueue const& command_queue)
{
	imageproc::BinaryImage dst(pixel_width, grid.height());

	std::array<size_t, 3> const zero_offset{0, 0, 0};
	std::array<size_t, 3> region;
	region[0] = std::min(dst.wordsPerLine(), grid.width()) * sizeof(uint32_t);
	region[1] = dst.height();
	region[2] = 1;
	command_queue.enqueueReadBufferRect(
		grid.buffer(), CL_TRUE, zero_offset, zero_offset, region,
		grid.stride() * sizeof(uint32_t), 0, dst.wordsPerLine() * sizeof(uint32_t), 0, dst.data()
	);

	return dst;
}

} // namespace tests

} // namespace opencl
