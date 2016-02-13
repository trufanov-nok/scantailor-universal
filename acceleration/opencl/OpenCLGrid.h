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

#ifndef OPENCL_GRID_H_
#define OPENCL_GRID_H_

#include "Grid.h"
#include <QSize>
#include <QRect>
#include <CL/cl2.hpp>

namespace opencl
{

template<typename Node>
class OpenCLGrid
{
	// Member-wise copying is OK (does a shallow copy).
public:
	/**
	 * The most general constructor.
	 */
	OpenCLGrid(cl::Buffer const& buffer, cl_int width, cl_int height, cl_int padding);

	/**
	 * A convenience constructor taking the metadata (but not copying
	 * the data itself) from a host-side Grid.
	 */
	OpenCLGrid(cl::Buffer const& buffer, Grid<Node> const& prototype);

	/**
	 * Returns a host-side Grid with identical structure but without copying the data.
	 */
	Grid<Node> toUninitializedHostGrid() const;

	/**
	 * The buffer pointing to the beginning of padding data.
	 */
	cl::Buffer const& buffer() const { return m_buffer; }

	/**
	 * The width of the grid, not counting any padding layers.
	 */
	cl_int width() const { return m_width; }

	/**
	 * The height of the grid, not counting any padding layers.
	 */
	cl_int height() const { return m_height; }

	/**
	 * The distance between a pointer to a node and the pointer to the node
	 * directly below it.
	 */
	cl_int stride() const { return m_stride; }

	/**
	 * The number of padding layers at each side.
	 */
	cl_int padding() const { return m_padding; }

	/**
	 * The distance between the pointer the first padding Node and the pointer
	 * to the first non-padding one.
	 */
	cl_int offset() const { return m_padding * m_stride + m_padding; }

	/**
	 * Total bytes occupied by this grid in memory.
	 */
	cl_int totalBytes() const { return totalBytesWithDifferentPadding(m_padding); }

	QSize size() const { return QSize(m_width, m_height); }

	QRect rect() const { return QRect(0, 0, m_width, m_height); }

	/**
	 * Returns the number of bytes this grid would occupy if its padding was different.
	 *
	 * This function assumes the new stride will be the minimum possible value.
	 */
	template<typename OtherNode = Node>
	cl_int totalBytesWithDifferentPadding(cl_int padding) const;

	/**
	 * Creates a new instance of OpenCLGrid that takes its type
	 * and dimensions (except padding) from this instance.
	 */
	template<typename OtherNode = Node>
	OpenCLGrid<OtherNode> withDifferentPadding(cl::Buffer const& buffer, int padding) const;
private:
	cl::Buffer m_buffer;
	cl_int m_width;
	cl_int m_height;
	cl_int m_stride;
	cl_int m_padding;
};

template<typename Node>
OpenCLGrid<Node>::OpenCLGrid(
	cl::Buffer const& buffer, cl_int width, cl_int height, cl_int padding)
:	m_buffer(buffer)
,	m_width(width)
,	m_height(height)
,	m_stride(width + padding * 2)
,	m_padding(padding)
{
}

template<typename Node>
OpenCLGrid<Node>::OpenCLGrid(cl::Buffer const& buffer, Grid<Node> const& prototype)
:	m_buffer(buffer)
,	m_width(prototype.width())
,	m_height(prototype.height())
,	m_stride(prototype.stride())
,	m_padding(prototype.padding())
{
}

template<typename Node>
Grid<Node>
OpenCLGrid<Node>::toUninitializedHostGrid() const
{
	// Here we rely on both Grid and OpenCLGrid always choosing the tightest stride.
	return Grid<Node>(m_width, m_height, m_padding);
}

template<typename Node>
template<typename OtherNode>
cl_int
OpenCLGrid<Node>::totalBytesWithDifferentPadding(cl_int padding) const
{
	return sizeof(OtherNode) * (m_height + padding * 2) * (m_width + padding * 2);
}

template<typename Node>
template<typename OtherNode>
OpenCLGrid<OtherNode>
OpenCLGrid<Node>::withDifferentPadding(cl::Buffer const& buffer, int padding) const
{
	return OpenCLGrid<OtherNode>(buffer, m_width, m_height, padding);
}

} // namespace opencl

#endif
