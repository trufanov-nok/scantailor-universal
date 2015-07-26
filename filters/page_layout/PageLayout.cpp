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

#include "PageLayout.h"
#include "MatchSizeMode.h"
#include "Alignment.h"
#include "RelativeMargins.h"
#include "Utils.h"
#include "imageproc/AbstractImageTransform.h"
#include <QTransform>
#include <QtGlobal>
#include <QMarginsF>
#include <algorithm>

using namespace imageproc;

namespace page_layout
{

PageLayout::PageLayout(
	QRectF const& unscaled_content_rect, QSizeF const& aggregate_hard_size,
	MatchSizeMode const& match_size_mode, Alignment const& alignment,
	RelativeMargins const& margins)
{
	m_innerRect = unscaled_content_rect;
	m_scaleFactor = 1.0;

	// An empty unscaled_content_rect is a special case to indicate
	// a missing content box. In this case, we want the geometry
	// we would get with zero hard margins and MatchSizeMode::GROW_MARGINS.
	bool const have_content_box = !unscaled_content_rect.isEmpty();

	if (have_content_box && match_size_mode.get() == MatchSizeMode::SCALE) {
		// aggregate_size = content_size * scale + margins * width * scale
		// Solving for scale:
		// scale = aggregate_size / (content_size + margins * width)
		qreal const x_scale = aggregate_hard_size.width() /
			(m_innerRect.width() + (margins.left() + margins.right()) * m_innerRect.width());
		qreal const y_scale = aggregate_hard_size.height() /
			(m_innerRect.height() + (margins.top() + margins.bottom()) * m_innerRect.width());

		if (x_scale > 1.0 && y_scale > 1.0) {
			m_scaleFactor = std::min(x_scale, y_scale);
		} else if (x_scale < 1.0 && y_scale < 1.0) {
			m_scaleFactor = std::max(x_scale, y_scale);
		}

		// The rectangle needs to be both shifted and scaled,
		// as that's what AbstractImageTransform::scale() does,
		// which we call in absorbScalingIntoTransform().
		m_innerRect = QRectF(
			m_innerRect.topLeft() * m_scaleFactor,
			m_innerRect.bottomRight() * m_scaleFactor
		);
	}

	if (have_content_box) {
		m_middleRect = margins.extendContentRect(m_innerRect);
	} else {
		m_middleRect = m_innerRect;
	}

	QMarginsF const soft_margins(
		Utils::calcSoftMarginsPx(
			m_middleRect.size(), aggregate_hard_size, match_size_mode, alignment
		)
	);

	m_outerRect = m_middleRect.adjusted(
		-soft_margins.left(), -soft_margins.top(),
		soft_margins.right(), soft_margins.bottom()
	);
}

void
PageLayout::absorbScalingIntoTransform(AbstractImageTransform& transform) const
{
	if (m_scaleFactor != 1.0) {
		transform.scale(m_scaleFactor, m_scaleFactor);
	}
}

} // namespace page_layout
