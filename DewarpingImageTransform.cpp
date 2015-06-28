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

#include "DewarpingImageTransform.h"
#include "AffineImageTransform.h"
#include "AffineTransformedImage.h"
#include "RoundingHasher.h"
#include "ToVec.h"
#include "dewarping/RasterDewarper.h"
#include <Eigen/Core>
#include <Eigen/LU>
#include <QImage>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QPolygonF>
#include <QLineF>
#include <QSize>
#include <QSizeF>
#include <QTransform>
#include <QColor>
#include <QString>
#include <algorithm>
#include <utility>
#include <memory>
#include <math.h>
#include <assert.h>

using namespace Eigen;
using namespace dewarping;

DewarpingImageTransform::DewarpingImageTransform(
	QSize const& orig_size,
	QPolygonF const& orig_crop_area,
	std::vector<QPointF> const& top_curve,
	std::vector<QPointF> const& bottom_curve,
	DepthPerception const& depth_perception)
:	m_origSize(orig_size)
,	m_origCropArea(orig_crop_area)
,	m_topPolyline(top_curve)
,	m_bottomPolyline(bottom_curve)
,	m_depthPerception(depth_perception)
,	m_dewarper(top_curve, bottom_curve, depth_perception.value())
,	m_postScaleX(1.0)
,	m_postScaleY(1.0)
{
	setupPostScale();
}

DewarpingImageTransform::~DewarpingImageTransform()
{
}

QString
DewarpingImageTransform::fingerprint() const
{
	RoundingHasher hash(QCryptographicHash::Sha1);
	hash << "DewarpingImageTransform";
	hash << m_origSize << m_origCropArea << m_depthPerception.value();
	for (QPointF const& pt : m_topPolyline) {
		hash << pt;
	}
	for (QPointF const& pt : m_bottomPolyline) {
		hash << pt;
	}
	hash << m_postScaleX << m_postScaleY;
	return QString::fromUtf8(hash.result().toHex());
}

std::unique_ptr<AbstractImageTransform>
DewarpingImageTransform::clone() const
{
	return std::unique_ptr<AbstractImageTransform>(
		new DewarpingImageTransform(*this)
	);
}

QPolygonF
DewarpingImageTransform::transformedCropArea() const
{
	QPolygonF poly(m_origCropArea);
	for (QPointF& pt : poly) {
		pt = postScale(m_dewarper.mapToDewarpedSpace(pt));
	}
	return poly;
}

QTransform
DewarpingImageTransform::scale(qreal xscale, qreal yscale)
{
	m_postScaleX *= xscale;
	m_postScaleY *= yscale;

	QTransform scaling_transform;
	scaling_transform.scale(xscale, yscale);
	return scaling_transform;
}

AffineTransformedImage
DewarpingImageTransform::toAffine(
	QImage const& image, QColor const& outside_color) const
{
	assert(!image.isNull());

	QPolygonF const transformed_crop_area(transformedCropArea());
	QRectF const dewarped_rect(transformed_crop_area.boundingRect());
	QSize const dst_size(dewarped_rect.toRect().size());
	QRectF const model_domain(
		-dewarped_rect.topLeft(), QSizeF(m_postScaleX, m_postScaleY)
	);

	QImage const dewarped_image(
		RasterDewarper::dewarp(
			image, dst_size, m_dewarper, model_domain, outside_color
		)
	);

	AffineImageTransform affine_transform(dst_size);
	affine_transform.setOrigCropArea(
		transformed_crop_area.translated(-dewarped_rect.topLeft())
	);

	// Translation is necessary to ensure that
	// transformedCropArea() == toAffine().transformedCropArea()
	affine_transform.setTransform(
		QTransform().translate(dewarped_rect.x(), dewarped_rect.y())
	);

	return AffineTransformedImage(dewarped_image, affine_transform);
}

AffineImageTransform
DewarpingImageTransform::toAffine() const
{
	QPolygonF const transformed_crop_area(transformedCropArea());
	QRectF const dewarped_rect(transformed_crop_area.boundingRect());
	QSize const dst_size(dewarped_rect.toRect().size());

	AffineImageTransform affine_transform(dst_size);
	affine_transform.setOrigCropArea(
		transformed_crop_area.translated(-dewarped_rect.topLeft())
	);

	// Translation is necessary to ensure that
	// transformedCropArea() == toAffine().transformedCropArea()
	affine_transform.setTransform(
		QTransform().translate(dewarped_rect.x(), dewarped_rect.y())
	);

	return affine_transform;
}

QImage
DewarpingImageTransform::materialize(QImage const& image,
	QRect const& target_rect, QColor const& outside_color) const
{
	assert(!image.isNull());
	assert(!target_rect.isEmpty());

	QRectF model_domain(0, 0, m_postScaleX, m_postScaleY);
	model_domain.translate(-target_rect.topLeft());

	return RasterDewarper::dewarp(
		image, target_rect.size(), m_dewarper, model_domain, outside_color
	);
}

std::function<QPointF(QPointF const&)>
DewarpingImageTransform::forwardMapper() const
{
	auto dewarper(std::make_shared<dewarping::CylindricalSurfaceDewarper>(m_dewarper));
	QTransform post_transform;
	post_transform.scale(m_postScaleX, m_postScaleY);

	return [=](QPointF const& pt) {
		return post_transform.map(dewarper->mapToDewarpedSpace(pt));
	};
}

std::function<QPointF(QPointF const&)>
DewarpingImageTransform::backwardMapper() const
{
	auto dewarper(std::make_shared<dewarping::CylindricalSurfaceDewarper>(m_dewarper));
	QTransform pre_transform;
	pre_transform.scale(1.0 / m_postScaleX, 1.0 / m_postScaleY);

	return [=](QPointF const& pt) {
		return dewarper->mapToWarpedSpace(pre_transform.map(pt));
	};
}

QPointF
DewarpingImageTransform::postScale(QPointF const& pt) const
{
	return QPointF(pt.x() * m_postScaleX, pt.y() * m_postScaleY);
}

/**
 * Initializes m_postScaleX and m_postScaleY in such a way that pixel density
 * near the "closest to the camera" corner of dewarping quadrilateral matches
 * the corresponding pixel density in a transformed image. Since we don't know
 * the physical dimensions of original image, we express pixel density in
 * "pixels / ABSTRACT_PHYSICAL_UNIT" units, where ABSTRACT_PHYSICAL_UNIT is
 * a hardcoded small fraction of either the physical width or the physical
 * height of dewarping quadrilateral.
 */
void
DewarpingImageTransform::setupPostScale()
{
	// Fraction of width or height of dewarping quardilateral.
	double const epsilon = 0.01;

	Vector2d const top_left_p1(toVec(m_topPolyline.front()));
	Vector2d const top_left_p2(toVec(m_dewarper.mapToWarpedSpace(QPointF(epsilon, 0))));
	Vector2d const top_left_p3(toVec(m_dewarper.mapToWarpedSpace(QPointF(0, epsilon))));

	Vector2d const top_right_p1(toVec(m_topPolyline.back()));
	Vector2d const top_right_p2(toVec(m_dewarper.mapToWarpedSpace(QPointF(1 - epsilon, 0))));
	Vector2d const top_right_p3(toVec(m_dewarper.mapToWarpedSpace(QPointF(1, epsilon))));

	Vector2d const bottom_left_p1(toVec(m_bottomPolyline.front()));
	Vector2d const bottom_left_p2(toVec(m_dewarper.mapToWarpedSpace(QPointF(epsilon, 1))));
	Vector2d const bottom_left_p3(toVec(m_dewarper.mapToWarpedSpace(QPointF(0, 1 - epsilon))));

	Vector2d const bottom_right_p1(toVec(m_bottomPolyline.back()));
	Vector2d const bottom_right_p2(toVec(m_dewarper.mapToWarpedSpace(QPointF(1 - epsilon, 1))));
	Vector2d const bottom_right_p3(toVec(m_dewarper.mapToWarpedSpace(QPointF(1, 1 - epsilon))));

	Matrix2d corners[4];
	corners[0] << top_left_p2 - top_left_p1, top_left_p3 - top_left_p1;
	corners[1] << top_right_p2 - top_right_p1, top_right_p3 - top_right_p1;
	corners[2] << bottom_left_p2 - bottom_left_p1, bottom_left_p3 - bottom_left_p1;
	corners[3] << bottom_right_p2 - bottom_right_p1, bottom_right_p3 - bottom_right_p1;

	// We assume a small square at a corner of a dewarped image maps
	// to a lozenge-shaped area in warped coordinates. That's not necessarily
	// the case, but let's assume it is. First of all, let's select a corner
	// with the highest lozenge area.
	int best_corner = 0;
	double largest_area = -1;
	for (int i = 0; i < 4; ++i)
	{
		double const area = fabs(corners[i].determinant());
		if (area > largest_area) {
			largest_area = area;
			best_corner = i;
		}
	}

	// See the comments in the beginning of the function on how
	// we define pixel densities.
	double const warped_h_density = corners[best_corner].col(0).norm();
	double const warped_v_density = corners[best_corner].col(1).norm();

	// CylindricalSurfaceDewarper maps a curved quadrilateral into a unit square.
	// Therefore, without post scaling, dewarped pixel density is exactly
	// epsilon x epsilon.
	double const dewarped_h_density = epsilon;
	double const dewarped_v_density = epsilon;

	// Now we are ready to calculate post scale.
	m_postScaleX = warped_h_density / dewarped_h_density;
	m_postScaleY = warped_v_density / dewarped_v_density;
}
