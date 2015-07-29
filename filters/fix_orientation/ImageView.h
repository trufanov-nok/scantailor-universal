/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef FIX_ORIENTATION_IMAGEVIEW_H_
#define FIX_ORIENTATION_IMAGEVIEW_H_

#include "ImageViewBase.h"
#include "ImagePixmapUnion.h"
#include "OrthogonalRotation.h"
#include "DragHandler.h"
#include "ZoomHandler.h"
#include "acceleration/AcceleratableOperations.h"
#include <QSizeF>
#include <memory>

namespace imageproc
{
	class AffineTransformedImage;
}

namespace fix_orientation
{

class ImageView : public ImageViewBase
{
	Q_OBJECT
public:
	ImageView(
		std::shared_ptr<AcceleratableOperations> const& accel_ops,
		imageproc::AffineTransformedImage const& rotated_image,
		ImagePixmapUnion const& downscaled_image);
	
	virtual ~ImageView();
public slots:
	void setPreRotation(OrthogonalRotation rotation);
private:
	DragHandler m_dragHandler;
	ZoomHandler m_zoomHandler;
	QSizeF m_origImageSize;
};

} // namespace fix_orientation

#endif
