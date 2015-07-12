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

#ifndef BASICIMAGEVIEW_H_
#define BASICIMAGEVIEW_H_

#include "ImageViewBase.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "ImagePixmapUnion.h"
#include "acceleration/AcceleratableOperations.h"
#include <QMarginsF>
#include <memory>

class QImage;

namespace imageproc
{
	class AffineTransformedImage;
}

class BasicImageView : public ImageViewBase
{
	Q_OBJECT
public:
	BasicImageView(
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		QImage const& full_size_image,
		ImagePixmapUnion const& downscaled_image = ImagePixmapUnion(),
		QMarginsF const& margins = QMarginsF());

	BasicImageView(
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		imageproc::AffineTransformedImage const& full_size_image,
		ImagePixmapUnion const& downscaled_image = ImagePixmapUnion(),
		QMarginsF const& margins = QMarginsF());
	
	virtual ~BasicImageView();
private:
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;
};

#endif
