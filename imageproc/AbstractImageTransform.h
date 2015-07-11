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

#ifndef IMAGEPROC_ABSTRACT_IMAGE_TRANSFORM_H_
#define IMAGEPROC_ABSTRACT_IMAGE_TRANSFORM_H_

#include "imageproc_config.h"
#include "acceleration/AcceleratableOperations.h"
#include <QtGlobal>
#include <memory>
#include <functional>

class QColor;
class QImage;
class QSize;
class QRect;
class QPointF;
class QPolygonF;
class QTransform;
class QString;

namespace imageproc
{

class AffineImageTransform;
class AffineTransformedImage;

class IMAGEPROC_EXPORT AbstractImageTransform
{
	// Member-wise copying is OK.
public:
	virtual ~AbstractImageTransform() {}

	virtual std::unique_ptr<AbstractImageTransform> clone() const = 0;

	virtual bool isAffine() const = 0;

	/**
	 * @brief Produces a hash of the transform.
	 *
	 * The hash has to be resistent to loss of accuracy of floating point
	 * values that happens when saving a project to an XML based format.
	 * Implementations should use RoundingHasher to address this problem.
	 */
	virtual QString fingerprint() const = 0;

	/**
	 * @brief Dimensions of original image.
	 */
	virtual QSize const& origSize() const = 0;

	/**
	 * @brief The crop area in original image coordinates.
	 */
	virtual QPolygonF const& origCropArea() const = 0;

	/**
	 * @brief The crop area in transformed coordinates.
	 */
	virtual QPolygonF transformedCropArea() const = 0;

	/**
	 * @brief Applies additional scaling to transformed space.
	 *
	 * A point (x, y) in transformed space before this call becomes point
	 * (x * xscale, y * yscale) after this call. For convenience, returns
	 * a QTransform that maps points in transformed space before scaling
	 * to corresponding points after scaling.
	 */
	virtual QTransform scale(qreal xscale, qreal yscale) = 0;

	/**
	 * If the concrete class implementing this interface is AffineImageTransform,
	 * the image and the transform are wrapped into AffineTransformedImage
	 * as they are. Otherwise, the transform is applied on the image provided,
	 * resulting in a new image and a new affine transform. The new transform
	 * will be a translation-only transform, to ensure that:
	 * @code
	 * transform.transformedCropArea() == transform.toAffine().transformedCropArea()
	 * @endcode
	 */
	virtual AffineTransformedImage toAffine(
		QImage const& image, QColor const& outside_color,
		std::shared_ptr<AcceleratableOperations> const& accel_ops) const = 0;

	/**
	 * This version of toAffine() can be viewed as a dry run for the full version.
	 */
	virtual AffineImageTransform toAffine() const = 0;

	/**
	 * Similar to a full version of toAffine(), except instead of producing some
	 * intermediate image plus a follow-up affine transformation, this one
	 * produces an image that represents the specified area of transformed space
	 * exactly, without requiring a follow-up transformation.
	 */
	virtual QImage materialize(QImage const& image,
		QRect const& target_rect, QColor const& outside_color,
		std::shared_ptr<AcceleratableOperations> const& accel_ops) const = 0;

	/**
	 * @brief Returns a function for mapping points from original image coordinates
	 *        to transformed space.
	 */
	virtual std::function<QPointF(QPointF const&)> forwardMapper() const = 0;

	/**
	 * @brief Returns a function for mapping points from transformed space
	 *        to original image coordinates.
	 */
	virtual std::function<QPointF(QPointF const&)> backwardMapper() const = 0;
};

} // namespace imageproc

#endif
