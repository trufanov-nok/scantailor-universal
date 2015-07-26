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

#ifndef PAGE_LAYOUT_PAGE_LAYOUT_H_
#define PAGE_LAYOUT_PAGE_LAYOUT_H_

#include <QRectF>

class QSizeF;
class RelativeMargins;

namespace imageproc
{
	class AbstractImageTransform;
}

namespace page_layout
{

class Alignment;
class MatchSizeMode;

class PageLayout
{
public:
	PageLayout(
		QRectF const& unscaled_content_rect, QSizeF const& aggregate_hard_size,
		MatchSizeMode const& match_size_mode, Alignment const& alignment,
		RelativeMargins const& margins);

	QRectF const& innerRect() const { return m_innerRect; }

	QRectF const& middleRect() const { return m_middleRect; }

	QRectF const& outerRect() const { return m_outerRect; }

	/**
	 * If match_size_mode passed into the constructor was set to SCALE,
	 * the 3 rectangles already incorporate the appropriate scaling factor.
	 * This method incorporates that scaling factor into the provided transform.
	 */
	void absorbScalingIntoTransform(imageproc::AbstractImageTransform& transform) const;
private:
	/** Content rectangle, in transformed coordinates. */
	QRectF m_innerRect;

	/** Rectangle around content plus hard margins, in transformed coordinates. */
	QRectF m_middleRect;

	/** Rectangle around content plus hard and soft margins, in transformed coordinates. */
	QRectF m_outerRect;

	/** Scaling applied as a result of MatchSizeMode::SCALE. */
	qreal m_scaleFactor;
};

} // namespace page_layout

#endif
