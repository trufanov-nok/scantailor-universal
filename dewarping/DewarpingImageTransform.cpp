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
#include "RoundingHasher.h"
#include "ToVec.h"
#include "ToLineProjector.h"
#include "LineIntersectionScalar.h"
#include "imageproc/AffineImageTransform.h"
#include "imageproc/AffineTransformedImage.h"
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
#include <boost/optional.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <algorithm>
#include <utility>
#include <memory>
#include <cmath>
#include <cassert>

using namespace Eigen;
using namespace imageproc;

namespace dewarping
{

DewarpingImageTransform::DewarpingImageTransform(
	QSize const& orig_size,
	QPolygonF const& orig_crop_area,
	std::vector<QPointF> const& top_curve,
	std::vector<QPointF> const& bottom_curve,
	DepthPerception const& depth_perception)
:	m_origSize(orig_size)
,	m_topPolyline(top_curve)
,	m_bottomPolyline(bottom_curve)
,	m_depthPerception(depth_perception)
,	m_dewarper(top_curve, bottom_curve, depth_perception.value())
,	m_intrinsicScaleX(1.0)
,	m_intrinsicScaleY(1.0)
,	m_userScaleX(1.0)
,	m_userScaleY(1.0)
{
	// These two lines don't depend on each other and therefore can go in any order.
	m_origCropArea = constrainCropArea(orig_crop_area);
	setupIntrinsicScale();
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

	hash << INTRINSIC_SCALE_ALGO_VERSION;
	hash << m_userScaleX << m_userScaleY;

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
	m_userScaleX *= xscale;
	m_userScaleY *= yscale;

	QTransform scaling_transform;
	scaling_transform.scale(xscale, yscale);
	return scaling_transform;
}

AffineTransformedImage
DewarpingImageTransform::toAffine(
	QImage const& image, QColor const& outside_color,
	std::shared_ptr<AcceleratableOperations> const& accel_ops) const
{
	assert(!image.isNull());

	QPolygonF const transformed_crop_area(transformedCropArea());
	QRectF const dewarped_rect(transformed_crop_area.boundingRect());
	QSize const dst_size(dewarped_rect.toRect().size());
	QRectF const model_domain(
		-dewarped_rect.topLeft(),
		QSizeF(m_intrinsicScaleX * m_userScaleX, m_intrinsicScaleY * m_userScaleY)
	);

	QImage const dewarped_image = accel_ops->dewarp(
		image, dst_size, m_dewarper, model_domain, outside_color
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
	QRect const& target_rect, QColor const& outside_color,
	std::shared_ptr<AcceleratableOperations> const& accel_ops) const
{
	assert(!image.isNull());
	assert(!target_rect.isEmpty());

	QRectF model_domain(0, 0, m_intrinsicScaleX * m_userScaleX, m_intrinsicScaleY * m_userScaleY);
	model_domain.translate(-target_rect.topLeft());

	return accel_ops->dewarp(
		image, target_rect.size(), m_dewarper, model_domain, outside_color
	);
}

std::function<QPointF(QPointF const&)>
DewarpingImageTransform::forwardMapper() const
{
	auto dewarper(std::make_shared<dewarping::CylindricalSurfaceDewarper>(m_dewarper));
	QTransform post_transform;
	post_transform.scale(m_intrinsicScaleX * m_userScaleX, m_intrinsicScaleY * m_userScaleY);

	return [=](QPointF const& pt) {
		return post_transform.map(dewarper->mapToDewarpedSpace(pt));
	};
}

std::function<QPointF(QPointF const&)>
DewarpingImageTransform::backwardMapper() const
{
	auto dewarper(std::make_shared<dewarping::CylindricalSurfaceDewarper>(m_dewarper));
	QTransform pre_transform;
	qreal const xscale = m_intrinsicScaleX * m_userScaleX;
	qreal const yscale = m_intrinsicScaleY * m_userScaleY;
	pre_transform.scale(1.0 / xscale, 1.0 / yscale);

	return [=](QPointF const& pt) {
		return dewarper->mapToWarpedSpace(pre_transform.map(pt));
	};
}

QPointF
DewarpingImageTransform::postScale(QPointF const& pt) const
{
	qreal const xscale = m_intrinsicScaleX * m_userScaleX;
	qreal const yscale = m_intrinsicScaleY * m_userScaleY;
	return QPointF(pt.x() * xscale, pt.y() * yscale);
}

/**
 * m_intrinsicScale[XY] factors don't participate in transform fingerprint calculation
 * due to them being derived from the distortion model and associated problems with
 * RoundingHasher. Still, the way we derive them from the distortion model may change
 * in the future and we'd like to reflect such changes in transform fingerprint.
 * This value is to be incremented each time we change the algorithm used to calculate
 * the intrinsic scale factors.
 */
int const DewarpingImageTransform::INTRINSIC_SCALE_ALGO_VERSION = 1;

/**
 * Initializes m_intrinsicScaleX and m_intrinsicScaleY in such a way that pixel
 * density near the "closest to the camera" corner of dewarping quadrilateral matches
 * the corresponding pixel density in a transformed image. Since we don't know
 * the physical dimensions of original image, we express pixel density in
 * "pixels / ABSTRACT_PHYSICAL_UNIT" units, where ABSTRACT_PHYSICAL_UNIT is
 * a hardcoded small fraction of either the physical width or the physical
 * height of dewarping quadrilateral.
 * Unfortunately, the approach above results in huge dewarped images in the presence
 * of a strong perspective distortion. Therefore, we additionally scale m_intrinsicScaleX
 * and m_intrinsicScaleY in such a way that the area (in pixels) of the curved quadrilateral
 * equals the area of the corresponding dewarped rectangle.
 */
void
DewarpingImageTransform::setupIntrinsicScale()
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

	// Now we are ready to calculate the initial scale.
	// We still need to normalize the area though.
	m_intrinsicScaleX = warped_h_density / dewarped_h_density;
	m_intrinsicScaleY = warped_v_density / dewarped_v_density;

	// Area of convex polygon formula taken from:
	// http://mathworld.wolfram.com/PolygonArea.html
	double area = 0.0;
	QPointF prev_pt = m_bottomPolyline.front();

	for (QPointF const pt : m_topPolyline) {
		area += prev_pt.x() * pt.y() - pt.x() * prev_pt.y();
		prev_pt = pt;
	}

	for (QPointF const pt : boost::adaptors::reverse(m_bottomPolyline)) {
		area += prev_pt.x() * pt.y() - pt.x() * prev_pt.y();
		prev_pt = pt;
	}

	area = 0.5 * std::abs(area);

	// m_intrinsicScaleX * s * m_intrinsicScaleY * s = area
	double const s = std::sqrt(area / (m_intrinsicScaleX * m_intrinsicScaleY));
	m_intrinsicScaleX *= s;
	m_intrinsicScaleY *= s;
}

QPolygonF
DewarpingImageTransform::constrainCropArea(QPolygonF const& orig_crop_area) const
{
	// The point where the vertical bounds intersect is a vanishing point that
	// can't be mapped into dewarped coordinates. We want to exclude it as well
	// as anything beyond it from the crop area.
	QLineF const left_bound(m_topPolyline.front(), m_bottomPolyline.front());
	QLineF const right_bound(m_topPolyline.back(), m_bottomPolyline.back());
	boost::optional<QPointF> vanishing_point;
	double left_bound_isect_scalar = 0.0;
	if (lineIntersectionScalar(left_bound, right_bound, left_bound_isect_scalar)) {
		vanishing_point = left_bound.pointAt(left_bound_isect_scalar);
	}

	// Points on left_bound will have the x coordinate of 0 in normalized dewarped coordinates.
	// Points on img_right_bound will have x coordinate of 1. We want to allow some area
	// outside of those bounds to be inside the crop region, yet we don't want to allow
	// the area too far outside of those bounds.
	CylindricalSurfaceDewarper::State state;
	QLineF const extended_left_bound = m_dewarper.mapGeneratrix(-1.0, state).imgLine;
	QLineF const mid_line = m_dewarper.mapGeneratrix(0.5, state).imgLine;
	QLineF const extended_right_bound = m_dewarper.mapGeneratrix(2.0, state).imgLine;

	ToLineProjector const mid_line_projector(mid_line);
	double max_proj = -std::numeric_limits<double>::max();
	double min_proj = std::numeric_limits<double>::max();

	for (QPointF const& pt : orig_crop_area) {
		double const proj = mid_line_projector.projectionScalar(pt);
		min_proj = std::min<double>(proj, min_proj);
		max_proj = std::max<double>(proj, max_proj);
	}

	if (vanishing_point) {
		// Any point that's safe to include into the crop area.
		QPointF const safe_point(0.5 * (left_bound.pointAt(0.5) + right_bound.pointAt(0.5)));

		double const vanishing_point_proj = mid_line_projector.projectionScalar(*vanishing_point);
		double const safe_point_proj = mid_line_projector.projectionScalar(safe_point);

		// We exclude the vanishing point and some surrounding area. To be more precise,
		// we exclude a certain fraction of the distance between the vanishing point and
		// the furthest away point in the direction of safe_point.
		double const safety_fraction = 0.04;
		if (vanishing_point_proj < safe_point_proj) {
			min_proj = std::max<double>(
				min_proj, vanishing_point_proj + safety_fraction * (max_proj - vanishing_point_proj)
			);
		} else {
			max_proj = std::min<double>(
				max_proj, vanishing_point_proj + safety_fraction * (min_proj - vanishing_point_proj)
			);
		}
	}

	QPolygonF extra_crop_area(4);
	QLineF mid_line_normal(mid_line.normalVector());

	mid_line_normal.translate(mid_line.pointAt(min_proj) - mid_line_normal.p1());
	mid_line_normal.intersect(extended_left_bound, &extra_crop_area[0]);
	mid_line_normal.intersect(extended_right_bound, &extra_crop_area[1]);

	mid_line_normal.translate(mid_line.pointAt(max_proj) - mid_line_normal.p1());
	mid_line_normal.intersect(extended_right_bound, &extra_crop_area[2]);
	mid_line_normal.intersect(extended_left_bound, &extra_crop_area[3]);

	QPolygonF intersection(orig_crop_area.intersected(extra_crop_area));
	if (intersection.isEmpty()) {
		// Try to workaround QTBUG-48003.
		intersection = extra_crop_area.intersected(orig_crop_area);
	}
	return intersection;
}

} // namespace dewarping
