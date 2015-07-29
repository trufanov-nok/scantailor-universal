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

#include "ImageView.h"
#include "ImagePresentation.h"
#include "imageproc/AffineImageTransform.h"
#include "imageproc/AffineTransformedImage.h"
#include <QRect>
#include <QRectF>
#include <QPolygonF>

namespace fix_orientation
{

ImageView::ImageView(
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	imageproc::AffineTransformedImage const& rotated_image,
	ImagePixmapUnion const& downscaled_image)
:	ImageViewBase(
		accel_ops, rotated_image.origImage(), downscaled_image,
		ImagePresentation(
			rotated_image.xform().transform(),
			rotated_image.xform().transformedCropArea()
		)
	),
	m_dragHandler(*this),
	m_zoomHandler(*this),
	m_origImageSize(rotated_image.origImage().size())
{
	rootInteractionHandler().makeLastFollower(m_dragHandler);
	rootInteractionHandler().makeLastFollower(m_zoomHandler);
}

ImageView::~ImageView()
{
}

void
ImageView::setPreRotation(OrthogonalRotation const rotation)
{
	QRectF const rect(QPointF(0, 0), rotation.rotate(m_origImageSize));

	// This should call update() by itself.
	updateTransform(ImagePresentation(rotation.transform(m_origImageSize), rect));
}

} // namespace fix_orientation
