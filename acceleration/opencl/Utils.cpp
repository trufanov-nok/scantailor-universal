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
#include <QPoint>
#include <utility>

namespace opencl
{

namespace
{

template<typename T>
T lsbBits(int bits)
{
	T val = ~T();

	int const shift = sizeof(T) * 8 - bits;
	val <<= shift;
	val >>= shift;

	return val;
}

template<typename T>
T msbBits(int bits)
{
	T val = ~T();

	int const shift = sizeof(T) * 8 - bits;
	val >>= shift;
	val <<= shift;

	return val;
}

} // anonymous namespace

bool isDodgyDevice(cl::Device const& device)
{
	cl_device_type const device_type = device.getInfo<CL_DEVICE_TYPE>();
	cl::Platform const platform(device.getInfo<CL_DEVICE_PLATFORM>());
	std::string const platform_name = platform.getInfo<CL_PLATFORM_NAME>();

	if (device_type == CL_DEVICE_TYPE_GPU && platform_name.find("Intel") != std::string::npos) {
		// Intel GPUs are marked as dodgy at least until the following bugs are fixed:
		// https://software.intel.com/en-us/forums/opencl/topic/560875
		// https://software.intel.com/en-us/forums/opencl/topic/591931
		return true;
	}

	return false;
}

void indicateCompletion(
	std::vector<cl::Event>* completion_set, cl::Event const& single_event)
{
	if (completion_set) {
		completion_set->reserve(1);
		completion_set->clear();
		completion_set->push_back(single_event);
	}
}

void indicateCompletion(
	std::vector<cl::Event>* completion_set, std::vector<cl::Event> const& event_set)
{
	if (completion_set) {
		*completion_set = event_set;
	}
}

void indicateCompletion(
	std::vector<cl::Event>* completion_set, std::vector<cl::Event>&& event_set)
{
	if (completion_set) {
		*completion_set = std::move(event_set);
	}
}

void indicateCompletion(
	std::vector<cl::Event>* completion_set, std::vector<cl::Event> const* event_set)
{
	if (completion_set) {
		if (event_set) {
			*completion_set = *event_set;
		} else {
			completion_set->clear();
		}
	}
}

size_t thisOrNextMultipleOf(size_t value, size_t factor)
{
	return thisOrPrevMultipleOf(value + factor - 1, factor);
}

size_t thisOrPrevMultipleOf(size_t value, size_t factor)
{
	return value - value % factor;
}

QRect pixelRectToWordRect(QRect const& pixel_rect)
{
	return QRect(
		QPoint(pixel_rect.left() >> 5, pixel_rect.top()),
		QPoint(pixel_rect.right() >> 5, pixel_rect.bottom())
	);
}

cl_uint2 computeLeftRightEdgeMasks(QRect const& pixel_rect)
{
	cl_uint left_mask = lsbBits<cl_uint>(32 - (pixel_rect.left() & 31));
	cl_uint right_mask = msbBits<cl_uint>((pixel_rect.right() & 31) + 1);

	if (pixel_rect.left() >> 5 == pixel_rect.right() >> 5) {
		// Left and right edge map to the same word.
		left_mask &= right_mask;
		right_mask = left_mask;
	}

	return cl_uint2{left_mask, right_mask};
}

} // namespace opencl
