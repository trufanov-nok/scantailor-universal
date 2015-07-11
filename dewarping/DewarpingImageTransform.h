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

#ifndef DEWARPING_DEWARPING_IMAGE_TRANSFORM_H_
#define DEWARPING_DEWARPING_IMAGE_TRANSFORM_H_

#include "dewarping_config.h"
#include "CylindricalSurfaceDewarper.h"
#include "DepthPerception.h"
#include "imageproc/AbstractImageTransform.h"
#include <QSize>
#include <QPointF>
#include <QPolygonF>
#include <QtGlobal>
#include <memory>
#include <functional>
#include <utility>
#include <vector>

class QImage;
class QColor;
class QString;

namespace imageproc
{
	class AffineImageTransform;
	class AffineTransformedImage;
}

namespace dewarping
{

class DEWARPING_EXPORT DewarpingImageTransform : public imageproc::AbstractImageTransform
{
	// Member-wise copying is OK.
public:
	DewarpingImageTransform(
		QSize const& orig_size,
		QPolygonF const& orig_crop_area,
		std::vector<QPointF> const& top_curve,
		std::vector<QPointF> const& bottom_curve,
		dewarping::DepthPerception const& depth_perception);

	virtual ~DewarpingImageTransform();

	virtual std::unique_ptr<AbstractImageTransform> clone() const;

	virtual bool isAffine() const { return false; }

	virtual QString fingerprint() const;

	virtual QSize const& origSize() const { return m_origSize; }

	virtual QPolygonF const& origCropArea() const { return m_origCropArea; }

	void setOrigCropArea(QPolygonF const& area) { m_origCropArea = area; }

	virtual QPolygonF transformedCropArea() const;

	virtual QTransform scale(qreal xscale, qreal yscale);

	virtual imageproc::AffineTransformedImage toAffine(
		QImage const& image, QColor const& outside_color,
		std::shared_ptr<AcceleratableOperations> const& accel_ops) const;

	virtual imageproc::AffineImageTransform toAffine() const;

	virtual QImage materialize(QImage const& image,
		QRect const& target_rect, QColor const& outside_color,
		std::shared_ptr<AcceleratableOperations> const& accel_ops) const;

	virtual std::function<QPointF(QPointF const&)> forwardMapper() const;

	virtual std::function<QPointF(QPointF const&)> backwardMapper() const;
private:
	void setupPostScale();

	QPointF postScale(QPointF const& pt) const;

	QSize m_origSize;
	QPolygonF m_origCropArea;
	std::vector<QPointF> m_topPolyline;
	std::vector<QPointF> m_bottomPolyline;
	dewarping::DepthPerception m_depthPerception;
	dewarping::CylindricalSurfaceDewarper m_dewarper;

	/**
	 * CylindricalSurfaceDewarper maps a curved quadrilateral
	 * into a unit square. Post scaling is applied to map that unit square
	 * to its final size.
	 */
	qreal m_postScaleX;
	qreal m_postScaleY;
};

} // namespace dewarping

#endif
