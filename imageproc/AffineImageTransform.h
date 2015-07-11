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

#ifndef IMAGEPROC_AFFINE_IMAGE_TRANSFORM_H_
#define IMAGEPROC_AFFINE_IMAGE_TRANSFORM_H_

#include "imageproc_config.h"
#include "AbstractImageTransform.h"
#include <QImage>
#include <QTransform>
#include <QPolygonF>
#include <QSize>
#include <Qt>
#include <QtGlobal>
#include <memory>
#include <functional>

class QPointF;
class QSizeF;
class QColor;
class QString;

namespace imageproc
{

class IMAGEPROC_EXPORT AffineImageTransform : public AbstractImageTransform
{
public:
	// Member-wise copying is OK.

	AffineImageTransform(QSize const& orig_size);

	virtual ~AffineImageTransform();

	virtual std::unique_ptr<AbstractImageTransform> clone() const;

	virtual bool isAffine() const { return true; }

	virtual QString fingerprint() const;

	virtual QSize const& origSize() const { return m_origSize; }

	virtual QPolygonF const& origCropArea() const { return m_origCropArea; }

	void setOrigCropArea(QPolygonF const& area) { m_origCropArea = area; }

	QTransform const& transform() const { return m_transform; }

	void setTransform(QTransform const& transform);

	virtual QPolygonF transformedCropArea() const;

	void adjustForScaledOrigImage(QSize const& orig_size);

	/**
	 * Adds translation to the current transformation such that
	 * \p transformed_pt moves to position \p target_pos.
	 */
	void translateSoThatPointBecomes(QPointF const& transformed_pt, QPointF const& target_pos);

	virtual QTransform scale(qreal xscale, qreal yscale);

	void scaleTo(QSizeF const& size, Qt::AspectRatioMode mode);

	void rotate(qreal degrees);

	/**
	 * Returns a version of this transformation modified by a client-provided
	 * adjuster.
	 *
	 * The @p adjuster will be called like this:
	 * @code
	 * AffineImageTransform xform(*this);
	 * adjuster(xform);
	 * @endcode
	 */
	template<typename T>
	AffineImageTransform adjusted(T adjuster) const;

	virtual AffineTransformedImage toAffine(
		QImage const& image, QColor const& outside_color,
		std::shared_ptr<AcceleratableOperations> const& accel_ops) const;

	virtual AffineImageTransform toAffine() const;

	virtual QImage materialize(QImage const& image,
		QRect const& target_rect, QColor const& outside_color,
		std::shared_ptr<AcceleratableOperations> const& accel_ops) const;

	virtual std::function<QPointF(QPointF const&)> forwardMapper() const;

	virtual std::function<QPointF(QPointF const&)> backwardMapper() const;
private:
	QSize m_origSize;
	QPolygonF m_origCropArea;
	QTransform m_transform;
};


template<typename T>
AffineImageTransform
AffineImageTransform::adjusted(T adjuster) const
{
	AffineImageTransform xform(*this);
	adjuster(xform);
	return xform;
}

} // namespace imageproc

#endif
