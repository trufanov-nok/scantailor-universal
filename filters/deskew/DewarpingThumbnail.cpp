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

#include "DewarpingThumbnail.h"
#include "dewarping/Curve.h"
#include "dewarping/DistortionModel.h"
#include "Utils.h"
#include <QPointF>
#include <QRectF>
#include <QLineF>
#include <QTransform>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QVector>
#include <QLineF>
#include <stdexcept>

using namespace imageproc;
using namespace dewarping;

namespace deskew
{

DewarpingThumbnail::DewarpingThumbnail(
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	QSizeF const& max_display_size, PageId const& page_id,
	AffineImageTransform const& full_size_image_transform,
	std::vector<QPointF> const& top_curve,
	std::vector<QPointF> const& bottom_curve,
	dewarping::DepthPerception const& depth_perception)
:	ThumbnailBase(
		thumbnail_cache, max_display_size,
		page_id, full_size_image_transform
	)
,	m_topCurve(top_curve)
,	m_bottomCurve(bottom_curve)
,	m_depthPerception(depth_perception)
{
	dewarping::DistortionModel distortion_model;
	distortion_model.setTopCurve(Curve(m_topCurve));
	distortion_model.setBottomCurve(Curve(m_bottomCurve));
	m_isValidModel = distortion_model.isValid();
}

void
DewarpingThumbnail::paintOverImage(
	QPainter& painter, QTransform const& transformed_to_display,
	QTransform const& thumb_to_display)
{
	if (!m_isValidModel) {
		return;
	}
	
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setTransform(fullSizeImageTransform().toAffine().transform() * transformed_to_display);

	QPen pen(QColor(0, 0, 255, 150));
	pen.setWidthF(1.0);
	pen.setCosmetic(true);
	painter.setPen(pen);

	unsigned const num_horizontal_curves = 15;
	unsigned const num_vertical_lines = 10;
	std::vector<std::vector<QPointF>> horizontal_curves;
	std::vector<QLineF> vertical_lines;

	try {
		Utils::buildWarpVisualization(
			m_topCurve, m_bottomCurve, m_depthPerception,
			num_horizontal_curves, num_vertical_lines,
			horizontal_curves, vertical_lines
		);

		for (auto const& curve : horizontal_curves) {
			painter.drawPolyline(curve.data(), curve.size());
		}

		for (auto const& line : vertical_lines) {
			painter.drawLine(line);
		}
	} catch (std::runtime_error const&) {
		// Invalid model?
	}
}

} // namespace deskew
