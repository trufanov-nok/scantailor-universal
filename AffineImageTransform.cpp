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

#include "AffineImageTransform.h"
#include "AffineTransformedImage.h"
#include "RoundingHasher.h"
#include "imageproc/Transform.h"
#include <QSizeF>
#include <QRectF>
#include <QLineF>
#include <QPointF>
#include <QString>
#include <QCryptographicHash>
#include <assert.h>

AffineImageTransform::AffineImageTransform(QSize const& orig_size)
:	m_origSize(orig_size)
,	m_origCropArea(QRectF(QPointF(0, 0), orig_size))
,	m_transform()
{
}

AffineImageTransform::~AffineImageTransform()
{
}

QString
AffineImageTransform::fingerprint() const
{
	RoundingHasher hash(QCryptographicHash::Sha1);
	hash << "AffineImageTransform";
	hash << m_origSize << m_origCropArea;
	hash << m_transform.m11() << m_transform.m21() << m_transform.dx();
	hash << m_transform.m12() << m_transform.m22() << m_transform.dy();
	return QString::fromUtf8(hash.result().toHex());
}

std::unique_ptr<AbstractImageTransform>
AffineImageTransform::clone() const
{
	return std::unique_ptr<AbstractImageTransform>(new AffineImageTransform(*this));
}

void
AffineImageTransform::setTransform(QTransform const& transform)
{
	m_transform = transform;
}

QPolygonF
AffineImageTransform::transformedCropArea() const
{
	return m_transform.map(m_origCropArea);
}

void
AffineImageTransform::adjustForScaledOrigImage(QSize const& orig_size)
{
	double const old2new_xscale = double(orig_size.width()) / m_origSize.width();
	double const old2new_yscale = double(orig_size.height()) / m_origSize.height();

	m_transform.scale(1.0 / old2new_xscale, 1.0 / old2new_yscale);
	for (QPointF& pt : m_origCropArea) {
		pt.rx() *= old2new_xscale;
		pt.ry() *= old2new_yscale;
	}
	m_origSize = orig_size;
}

void
AffineImageTransform::translateSoThatPointBecomes(
	QPointF const& transformed_pt, QPointF const& target_pos)
{
	QPointF const delta(target_pos - transformed_pt);
	QTransform translation;
	translation.translate(delta.x(), delta.y());
	m_transform *= translation;
}

QTransform
AffineImageTransform::scale(qreal xscale, qreal yscale)
{
	QTransform scaling_transform;
	scaling_transform.scale(xscale, yscale);

	m_transform *= scaling_transform;

	return scaling_transform;
}

void
AffineImageTransform::scaleTo(QSizeF const& size, Qt::AspectRatioMode mode)
{
	assert(!size.isEmpty());

	QSizeF const transformed_size(transformedCropArea().boundingRect().size());
	QSizeF const desired_size(transformed_size.scaled(size, mode));

	double const xscale = desired_size.width() / transformed_size.width();
	double const yscale = desired_size.height() / transformed_size.height();

	m_transform *= QTransform().scale(xscale, yscale);
}

void
AffineImageTransform::rotate(qreal degrees)
{
	m_transform *= QTransform().rotate(degrees);
}

AffineTransformedImage
AffineImageTransform::toAffine(QImage const& image, QColor const&) const
{
	assert(!image.isNull());
	assert(image.size() == m_origSize);
	return AffineTransformedImage(image, *this);
}

AffineImageTransform
AffineImageTransform::toAffine() const
{
	return *this;
}

QImage
AffineImageTransform::materialize(QImage const& image,
	QRect const& target_rect, QColor const& outside_color) const
{
	assert(!image.isNull());
	assert(!target_rect.isEmpty());

	return imageproc::transform(
		image, m_transform, target_rect,
		imageproc::OutsidePixels::assumeColor(outside_color)
	);
}

std::function<QPointF(QPointF const&)>
AffineImageTransform::forwardMapper() const
{
	QTransform transform(m_transform);
	return [=](QPointF const& pt) {
		return transform.map(pt);
	};
}

std::function<QPointF(QPointF const&)>
AffineImageTransform::backwardMapper() const
{
	QTransform inverted(m_transform.inverted());
	return [=](QPointF const& pt) {
		return inverted.map(pt);
	};
}
