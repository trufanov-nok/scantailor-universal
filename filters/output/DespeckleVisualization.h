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

#ifndef OUTPUT_DESPECKLE_VISUALIZATION_H_
#define OUTPUT_DESPECKLE_VISUALIZATION_H_

#include "acceleration/AcceleratableOperations.h"
#include <QImage>
#include <memory>

namespace imageproc
{
	class BinaryImage;
}

namespace output
{

class DespeckleVisualization
{
public:
	/*
	 * Constructs a null visualization.
	 */
	DespeckleVisualization() {}

	/**
	 * \param accel_ops OpenCL-acceleratable operations.
	 * \param output The output file, as produced by OutputGenerator::process().
	 *        If this one is null, the visualization will be null as well.
	 * \param speckles Speckles detected in the image.
	 *        If this one is null, it is considered no speckles were detected.
	 */
	DespeckleVisualization(
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		QImage const& output, imageproc::BinaryImage const& speckles);

	bool isNull() const { return m_image.isNull(); }

	QImage const& image() const { return m_image; }

	QImage const& downscaledImage() const { return m_downscaledImage; }
private:
	static void colorizeSpeckles(
		QImage& image, imageproc::BinaryImage const& speckles);

	QImage m_image;
	QImage m_downscaledImage;
};

} // namespace output

#endif
