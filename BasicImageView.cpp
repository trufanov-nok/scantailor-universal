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

#include "BasicImageView.h"
#include "ImagePresentation.h"
#include "imageproc/AffineTransformedImage.h"
#include <QImage>

using namespace imageproc;

BasicImageView::BasicImageView(
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& full_size_image,
	ImagePixmapUnion const& downscaled_image, QMarginsF const& margins)
:	ImageViewBase(
		accel_ops, full_size_image, downscaled_image,
		ImagePresentation(QTransform(), QRectF(full_size_image.rect())),
		margins
	),
	m_dragHandler(*this),
	m_zoomHandler(*this)
{
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

BasicImageView::BasicImageView(
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	AffineTransformedImage const& full_size_image,
	ImagePixmapUnion const& downscaled_image, QMarginsF const& margins)
:	ImageViewBase(
		accel_ops, full_size_image.origImage(), downscaled_image,
		ImagePresentation(
			full_size_image.xform().transform(),
			full_size_image.xform().transformedCropArea()
		),
		margins
	),
	m_dragHandler(*this),
	m_zoomHandler(*this)
{
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

BasicImageView::~BasicImageView()
{
}
