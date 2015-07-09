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

#ifndef IMAGEPROC_AFFINE_TRANSFORMED_IMAGE_H_
#define IMAGEPROC_AFFINE_TRANSFORMED_IMAGE_H_

#include "imageproc_config.h"
#include "AffineImageTransform.h"
#include <QImage>
#include <QSizeF>
#include <Qt>

namespace imageproc
{

class IMAGEPROC_EXPORT AffineTransformedImage
{
	// Member-wise copying is OK.
public:
	/**
	 * @param image Image to apply a transformation on. Can't be a null QImage.
	 *
	 * This version sets up an identity transformation with image.rect()
	 * being the crop area.
	 */
	explicit AffineTransformedImage(QImage const& image);

	/**
	 * @param image Image to apply a transformation on. Can't be a null QImage.
	 * @param xform Transformation to be applied to \p image.
	 *
	 * @note image.size() must be equal to xform.origSize()
	 */
	AffineTransformedImage(QImage const& image, AffineImageTransform const& xform);

	QImage const& origImage() const { return m_origImage; }

	AffineImageTransform const& xform() const { return m_xform; }

	/**
	 * Returns the same image with a transformation modified by a client-provided
	 * adjuster.
	 *
	 * The @p adjuster will be called like this:
	 * @code
	 * AffineImageTransform xform = ...;
	 * adjuster(xform);
	 * // <use the modified xform>
	 * @endcode
	 */
	template<typename T>
	AffineTransformedImage withAdjustedTransform(T adjuster) const;
private:
	QImage m_origImage;
	AffineImageTransform m_xform;
};


template<typename T>
AffineTransformedImage
AffineTransformedImage::withAdjustedTransform(T adjuster) const
{
	return AffineTransformedImage(m_origImage, m_xform.adjusted(adjuster));
}

} // namespace imageproc

#endif
