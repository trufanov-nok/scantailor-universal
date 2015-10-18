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

#ifndef DEWARPING_RASTER_DEWARPER_H_
#define DEWARPING_RASTER_DEWARPER_H_

#include "dewarping_config.h"
#include <QSizeF>

class QImage;
class QSize;
class QRectF;
class QColor;

namespace dewarping
{

class CylindricalSurfaceDewarper;

class DEWARPING_EXPORT RasterDewarper
{
public:
	/**
	 * @brief Performs a dewarping operation.
	 *
	 * @param src The warped image. Can't be null.
	 * @param dst_size The dimensions of the output image.
	 * @param distortion_model A curved quadrilateral in source image coordinates.
	 * @param model_domain The rectangle in output image coordinates that the
	 *        curved quadrilateral will map to.
	 * @param background_color The color to use for pixels outside of source image boundaries.
	 * @param min_density The minimum allowed ratio of a distance in src image coordinates
	 *        over the corresponding distance in normalized dewarped coordinates.
	 *        For interpretation of normalized dewarped coordinates, see comments at the top
	 *        of CylindricalSurfaceDewarper.cpp. Dewarped pixels where density is less than
	 *        min_density will be treated the same way as pixels mapping outside of the
	 *        warped image.
	 * @param max_density The maximum allowed ratio of a distance in src image coordinates
	 *        over the corresponding distance in normalized dewarped coordinates.
	 *        For interpretation of normalized dewarped coordinates, see comments at the top
	 *        of CylindricalSurfaceDewarper.cpp. Dewarped pixels where density is more than
	 *        max_density will be treated the same way as pixels mapping outside of the
	 *        warped image.
	 * @param min_mapping_area Defines the minimum rectangle in the source image
	 *        that maps to a destination pixel.  This can be used to control
	 *        smoothing.
	 * @return The dewarped image.
	 */
	static QImage dewarp(
		QImage const& src, QSize const& dst_size,
		CylindricalSurfaceDewarper const& distortion_model,
		QRectF const& model_domain, QColor const& background_color,
		float min_density, float max_density,
		QSizeF const& min_mapping_area = QSizeF(0.9, 0.9));
};

} // namespace dewarping

#endif
