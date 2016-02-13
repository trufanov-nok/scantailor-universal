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

#ifndef OPENCL_TESTS_UTILS_H_
#define OPENCL_TESTS_UTILS_H_

#include "OpenCLGrid.h"
#include "imageproc/BinaryImage.h"
#include <CL/cl2.hpp>
#include <vector>
#include <deque>

namespace opencl
{

namespace tests
{

class DeviceListFixture
{
public:
	DeviceListFixture();
protected:
	std::vector<cl::Device> m_devices;
};

class ProgramBuilderFixture
{
public:
	ProgramBuilderFixture();

	~ProgramBuilderFixture();
protected:
	void addSource(char const* source_fname);

	cl::Program buildProgram(cl::Context const& context) const;
private:
	cl::Program::Sources m_sources;
};

imageproc::BinaryImage randomBinaryImage(int width, int height);

OpenCLGrid<uint32_t> binaryImageToOpenCLGrid(
	imageproc::BinaryImage const& image, cl::CommandQueue const& command_queue);

imageproc::BinaryImage openCLGridToBinaryImage(
	OpenCLGrid<uint32_t> const& grid, int pixel_width,
	cl::CommandQueue const& command_queue);

} // namespace tests

} // namespace opencl

#endif
