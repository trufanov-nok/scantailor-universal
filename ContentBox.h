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

#ifndef SELECT_CONTENT_CONTENT_BOX_H_
#define SELECT_CONTENT_CONTENT_BOX_H_

#include <QPointF>

class QDomDocument;
class QDomElement;
class QString;
class QRectF;

namespace imageproc
{
	class AbstractImageTransform;
}

/**
 * @brief A rectangle in arbitrarily transformed space.
 *
 * A rectangle in arbitrarily transformed space is represented as 4 midpoints
 * of its edges, back-projected to the original space.
 */
class ContentBox
{
	// Member-wise copying is OK.
public:
	/** Constructs an invalid ContentBox. */
	ContentBox();

	/**
	 * @param transform Pre-transformation of the original image.
	 * @param transformed_rect Content box in transformed space.
	 */
	ContentBox(imageproc::AbstractImageTransform const& transform,
		QRectF const& transformed_rect);
	
	ContentBox(QDomElement const& el);
	
	~ContentBox();

	bool isValid() const;
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	QPointF const& topEdgeMidPoint() const { return m_topEdgeMidPoint; }

	QPointF const& bottomEdgeMidPoint() const { return m_bottomEdgeMidPoint; }

	QPointF const& leftEdgeMidPoint() const { return m_leftEdgeMidPoint; }

	QPointF const& rightEdgeMidPoint() const { return m_rightEdgeMidPoint; }

	/**
	 * @brief Convert the internal representation to a rectangle
	 *        in transformed space.
	 */
	QRectF toTransformedRect(imageproc::AbstractImageTransform const& transform) const;
private:
	QPointF m_topEdgeMidPoint;
	QPointF m_bottomEdgeMidPoint;
	QPointF m_leftEdgeMidPoint;
	QPointF m_rightEdgeMidPoint;
};

#endif
