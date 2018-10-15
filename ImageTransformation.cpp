/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "ImageTransformation.h"
#include "settings/globalstaticsettings.h"
#include <QPointF>
#include <QLineF>
#include <algorithm>

ImageTransformation::ImageTransformation(
    QRectF const& orig_image_rect, Dpi const& orig_dpi, Level pageLevel)
:	m_postRotation(0.0),
	m_origRect(orig_image_rect),
	m_resultingRect(orig_image_rect),
    m_origDpi(orig_dpi),
    m_pageLevel(pageLevel)
{
	preScaleToEqualizeDpi();
}

ImageTransformation::~ImageTransformation()
{
}

void
ImageTransformation::preScaleToDpi(Dpi const& dpi)
{
	if (m_origDpi.isNull() || dpi.isNull()) {
		return;
	}
	
	m_preScaledDpi = dpi;
	
	double const xscale = (double)dpi.horizontal() / m_origDpi.horizontal();
	double const yscale = (double)dpi.vertical() / m_origDpi.vertical();
	
	QSizeF const new_pre_scaled_image_size(
		m_origRect.width() * xscale, m_origRect.height() * yscale
	);
	
	// Undo's for the specified steps.
    QTransform const undo21(m_preRotateXformPage.inverted() * m_preScaleXform.inverted());
    QTransform const undo4321(m_postRotateXform.inverted() * preCropXform().inverted() * undo21);
	
	// Update transform #1: pre-scale.
	m_preScaleXform.reset();
	m_preScaleXform.scale(xscale, yscale);

	// Update transform #2: pre-rotate.
    m_preRotateXformPage = preRotation().transform(new_pre_scaled_image_size);

	// Update transform #3: pre-crop.
    QTransform const redo12(m_preScaleXform * m_preRotateXformPage);
    setPreCropArea((undo21 * redo12).map(preCropArea()));
    setPreCropXform(calcCropXform(preCropArea()));

	// Update transform #4: post-rotate.
	m_postRotateXform = calcPostRotateXform(m_postRotation);

	// Update transform #5: post-crop.
    QTransform const redo1234(redo12 * preCropXform() * m_postRotateXform);
	m_postCropArea = (undo4321 * redo1234).map(m_postCropArea);
	m_postCropXform = calcCropXform(m_postCropArea);

	// Update transform #6: post-scale.
	m_postScaleXform = calcPostScaleXform(m_postScaledDpi);
	
	update();
}

void
ImageTransformation::preScaleToEqualizeDpi()
{
	int const min_dpi = std::min(m_origDpi.horizontal(), m_origDpi.vertical());
	preScaleToDpi(Dpi(min_dpi, min_dpi));
}

void
ImageTransformation::setPreRotation(OrthogonalRotation const rotation)
{
    if (m_pageLevel == ImageLevel) {
        m_preRotationImage = rotation;
        m_preRotateXformImage = m_preRotationImage.transform(m_origRect.size());
//        m_preRotationPage = m_preRotationImage;
//        m_preRotateXformPage = m_preRotateXformImage;
        resetPreCropArea();
        resetPostRotation();
        resetPostCrop();
        resetPostScale();
        update();
    } else {
        m_preRotationPage = rotation;
        m_preRotateXformPage = m_preRotationPage.transform(m_origRect.size());

        QPolygonF new_preCropAreaPage;
        if (m_preRotateXformPage != m_preRotateXformImage) {
            new_preCropAreaPage = m_preRotateXformPage.map(
                        m_preRotateXformImage.inverted().map(m_preCropAreaImage)
                        );
        } else {
            new_preCropAreaPage = m_preCropAreaImage;
        }
        setPreCropArea(new_preCropAreaPage);
    }


}


void
ImageTransformation::setPreCropArea(QPolygonF const& area)
{
    if (m_pageLevel == ImageLevel) {
        m_preCropAreaImage = area;
        m_preCropXformImage = calcCropXform(area);
        resetPostRotation();
        resetPostCrop();
    } else {
        m_preCropAreaPage = area;
        m_preCropXformPage = calcCropXform(area);
    }


    resetPostScale();
    update();
}

void
ImageTransformation::setPostRotation(double const degrees)
{
	m_postRotateXform = calcPostRotateXform(degrees);
	m_postRotation = degrees;
	resetPostCrop();
	resetPostScale();
	update();
}

void
ImageTransformation::setPostCropArea(QPolygonF const& area)
{
	m_postCropArea = area;
	m_postCropXform = calcCropXform(area);
	resetPostScale();
	update();
}

void
ImageTransformation::postScaleToDpi(Dpi const& dpi)
{
	m_postScaledDpi = dpi;
	m_postScaleXform = calcPostScaleXform(dpi);
	update();
}

QTransform
ImageTransformation::calcCropXform(QPolygonF const& area)
{
	QRectF const bounds(area.boundingRect());
	QTransform xform;
	xform.translate(-bounds.x(), -bounds.y());
	return xform;
}

QTransform
ImageTransformation::calcPostRotateXform(double const degrees)
{
	QTransform xform;
	if (degrees != 0.0) {
        QPointF const origin(preCropArea().boundingRect().center());
		xform.translate(-origin.x(), -origin.y());
		xform *= QTransform().rotate(degrees);
		xform *= QTransform().translate(origin.x(), origin.y());
		
		// Calculate size changes.
        QPolygonF const pre_rotate_poly(preCropXform().map(preCropArea()));
		QRectF const pre_rotate_rect(pre_rotate_poly.boundingRect());
		QPolygonF const post_rotate_poly(xform.map(pre_rotate_poly));
		QRectF const post_rotate_rect(post_rotate_poly.boundingRect());
		
		xform *= QTransform().translate(
			pre_rotate_rect.left() - post_rotate_rect.left(),
			pre_rotate_rect.top() - post_rotate_rect.top()
		);
	}
	return xform;
}

QTransform
ImageTransformation::calcPostScaleXform(Dpi const& target_dpi)
{
	if (target_dpi.isNull()) {
		return QTransform();
	}

	// We are going to measure the effective DPI after the previous transforms.
	// Normally m_preScaledDpi would be symmetric, so we could just
	// use that, but just in case ...

	QTransform const to_orig(m_postScaleXform * m_transform.inverted());
	// IMPORTANT: in the above line we assume post-scale is the last transform.

	QLineF const hor_unit(QPointF(0, 0), QPointF(1, 0));
	QLineF const vert_unit(QPointF(0, 0), QPointF(0, 1));
	QLineF const orig_hor_unit(to_orig.map(hor_unit));
	QLineF const orig_vert_unit(to_orig.map(vert_unit));
	
	double const xscale = target_dpi.horizontal() * orig_hor_unit.length() / m_origDpi.horizontal();
	double const yscale = target_dpi.vertical() * orig_vert_unit.length() / m_origDpi.vertical();
	QTransform xform;
	xform.scale(xscale, yscale);
	return xform;
}

void
ImageTransformation::resetPreCropArea()
{
    if (m_pageLevel == ImageLevel) {
        m_preCropAreaImage.clear();
        m_preCropXformImage.reset();
    }
    m_preCropAreaPage.clear();
    m_preCropXformPage.reset();
}

void
ImageTransformation::resetPostRotation()
{
	m_postRotation = 0.0;
	m_postRotateXform.reset();
}

void
ImageTransformation::resetPostCrop()
{
	m_postCropArea.clear();
	m_postCropXform.reset();
}

void
ImageTransformation::resetPostScale()
{
	m_postScaledDpi = Dpi();
	m_postScaleXform.reset();
}

void
ImageTransformation::changeLevel(Level pageLevel)
{
    if (!GlobalStaticSettings::m_drawDeskewOrientFix) {
        return;
    }

    if (pageLevel != m_pageLevel) {
        m_pageLevel = pageLevel;
        if (m_pageLevel == PageLevel) {
            m_preRotationPage = m_preRotationImage;
            m_preRotateXformPage = m_preRotateXformImage;
            m_preCropAreaPage = m_preCropAreaImage;
            m_preCropXformPage = m_preCropXformImage;
        }
    }
    update();
}

void
ImageTransformation::update()
{
    QTransform const pre_scale_then_pre_rotate(m_preScaleXform * preRotateXform()); // 12
    QTransform const pre_crop_then_post_rotate(preCropXform() * m_postRotateXform); // 34
	QTransform const post_crop_then_post_scale(m_postCropXform * m_postScaleXform); // 56
	QTransform const pre_crop_and_further(pre_crop_then_post_rotate * post_crop_then_post_scale); // 3456
	m_transform = pre_scale_then_pre_rotate * pre_crop_and_further;
	m_invTransform = m_transform.inverted();
    if (preCropArea().empty()) {
        setPreCropArea(pre_scale_then_pre_rotate.map(m_origRect));
	}
	if (m_postCropArea.empty()) {
        m_postCropArea = pre_crop_then_post_rotate.map(preCropArea());
	}
    m_resultingPreCropArea = pre_crop_and_further.map(preCropArea());
	m_resultingPostCropArea = post_crop_then_post_scale.map(m_postCropArea);
	m_resultingRect = m_resultingPostCropArea.boundingRect();
}
