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

#ifndef IMAGEPROC_WATERSHED_SEGMENTATION_H_
#define IMAGEPROC_WATERSHED_SEGMENTATION_H_

#include "imageproc_config.h"
#include "Grid.h"
#include "Connectivity.h"
#include <QSize>
#include <stdint.h>

class QImage;

namespace imageproc
{

class GrayImage;

/**
 * \brief Labels pixels into groups according to the watershed principle.
 */
class IMAGEPROC_EXPORT WatershedSegmentation
{
public:
	/**
	 * \brief Constructs a null (zero size) segmentation.
	 */
	WatershedSegmentation();

	/**
	 * \brief Builds a segmentation from a grayscale image.
	 *
	 * Grayscale values are interpreted as an altitude. The water flows
	 * from higher (light) pixels to smaller (dark) ones. Label 0 is reserved
	 * and won't be used in the segmentation.
	 */
	explicit WatershedSegmentation(GrayImage const& image, Connectivity conn);
	
	void swap(WatershedSegmentation& other);

	GridAccessor<uint32_t const> accessor() const { return m_grid.accessor(); }

	GridAccessor<uint32_t> accessor() { return m_grid.accessor(); }
	
	/**
	 * \brief Returns a pointer to the top-left corner of label grid.
	 *
	 * The data is stored in row-major order, and may be padded,
	 * so moving to the next line requires adding stride() rather
	 * than size().width().
	 */
	uint32_t const* data() const { return m_grid.data(); }
	
	/**
	 * \brief Returns a pointer to the top-left corner of label grid.
	 *
	 * The data is stored in row-major order, and may be padded,
	 * so moving to the next line requires adding stride() rather
	 * than size().width().
	 */
	uint32_t* data() { return m_grid.data(); }
	
	/**
	 * \brief Returns the dimensions of the label grid.
	 */
	QSize size() const { return QSize(m_grid.width(), m_grid.height()); }
	
	/**
	 * \brief Returns the number of units on a line of labels.
	 *
	 * This number may be larger than size.width(). Adding this number
	 *to a data pointer will move it one line down.
	 */
	int stride() const { return m_grid.stride(); }
	
	/**
	 * \brief Returns the maximum label present in the segmentation.
	 *
	 * The label of zero is not used in segmentation, though a null
	 * (empty) segmentation will report its maximum label as zero.
	 */
	uint32_t maxLabel() const { return m_maxLabel; }
	
	/**
	 * \brief Visualizes each label with a different color.
	 */
	QImage visualized() const;
private:
	Grid<uint32_t> m_grid;
	uint32_t m_maxLabel;
};


inline void swap(WatershedSegmentation& o1, WatershedSegmentation& o2)
{
	o1.swap(o2);
}

} // namespace imageproc

#endif
