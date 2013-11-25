/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

class QImage;
class QSize;
class QRectF;
class QColor;

namespace dewarping
{

class CylindricalSurfaceDewarper;

class RasterDewarper
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
	 * @return The dewarped image.
	 */
	static QImage dewarp(
		QImage const& src, QSize const& dst_size,
		CylindricalSurfaceDewarper const& distortion_model,
		QRectF const& model_domain, QColor const& background_color);
};

} // namespace dewarping

#endif
