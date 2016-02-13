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

#ifndef OPENCL_UTILS_H_
#define OPENCL_UTILS_H_

#include <QRect>
#include <CL/cl2.hpp>
#include <cstddef>
#include <vector>

namespace opencl
{

/**
 * We mark devices known to have buggy drivers as dodgy. For simplicity, if a certain
 * operation is known to have issues with any dodgy device, it's not performed on all
 * devices marked as dodgy.
 */
bool isDodgyDevice(cl::Device const& device);

/**
 * Functions that enqueue OpenCL kernels and let the client code wait for their completion
 * should take the following parameters as part of their signature:
 * @code
 * std::vector<cl::Event> const* dependencies = nullptr, std::vector<cl::Event>* completion_set
 * @endcode
 * The kernels enqueued by such a function will depend on @p dependencies while the client
 * code will wait for events in @p completion_set. It's allowed to pass the same vector as
 * both @p dependencies and @p completion_set.
 *
 * On every return path (even on a no-op return) such functions should call one of the
 * overloads of indicateCompletion().
 *
 * This overload copies a single event into @p completion_set, if @p completion_set is not null.
 * Pre-existng events in @p completion_set are removed.
 */
void indicateCompletion(
	std::vector<cl::Event>* completion_set, cl::Event const& single_event);

/**
 * This overload copies @p event_set into @p completion_set, if @p completion set is not null.
 * Pre-existng events in @p completion_set are removed.
 */
void indicateCompletion(
	std::vector<cl::Event>* completion_set, std::vector<cl::Event> const& event_set);

/**
 * This overload moves @p event_set into @p completion_set, if @p completion set is not null.
 * Pre-existng events in @p completion_set are removed.
 */
void indicateCompletion(
	std::vector<cl::Event>* completion_set, std::vector<cl::Event>&& event_set);

/**
 * This overload is useful for no-op returns. In such a case, a function passes its
 * @p dependencies argument to indicateCompletion() as @p event_set. The events from
 * @p event_set will be copied to @p completion_set if both are not null. Pre-existng
 * events in @p completion_set are removed, no matter if @p event_set is provided or not.
 */
void indicateCompletion(
	std::vector<cl::Event>* completion_set, std::vector<cl::Event> const* event_set);

/**
 * If @p value is a multiple of @p factor, @p value is returned.
 * Otherwise, the smallest multiple of @p factor that's larger than @p value is returned.
 */
size_t thisOrNextMultipleOf(size_t value, size_t factor);

/**
 * If @p value is a multiple of @p factor, @p value is returned.
 * Otherwise, the largest multiple of @p factor that's smaller than @p value is returned.
 */
size_t thisOrPrevMultipleOf(size_t value, size_t factor);

/**
 * Binary images pack pixels as bits in 32-bit words. On CPU side we use
 * imageproc::BinaryImage while on GPU side we use OpenCLGrid<uint32_t>
 * to store binary images. Given a rectangle in pixel coordinates, this
 * function returns a rectangle in word coordinates so that edge words
 * are the ones containing edge pixels. By edge pixels we mean pixels
 * that are on the edge but still within @p pixel_rect. Edge words mean
 * the same thing with respect to the rectangle returned by this function.
 */
QRect pixelRectToWordRect(QRect const& pixel_rect);

/**
 * When we want to modify pixels in a certain rectangle (in pixel coordinates)
 * in a binary image, we generally have to preserve some of the bits in lefmost
 * and rightmost words of the corresponding rectangle in word coordinates.
 * This function returns two masks corresponding to leftmost and rightmost
 * words respectively. Bits that are set correspond to pixels we are allowed
 * to modify.
 *
 * @see pixelRectToWordRect()
 */
cl_uint2 computeLeftRightEdgeMasks(QRect const& pixel_rect);

} // namespace opencl

#endif
