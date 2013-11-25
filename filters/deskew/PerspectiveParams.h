/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef DESKEW_PERSPECTIVE_PARAMS_H_
#define DESKEW_PERSPECTIVE_PARAMS_H_

#include "AutoManualMode.h"
#include <QPointF>

class QDomDocument;
class QDomElement;
class QString;

namespace deskew
{

/**
 * PerspectiveParams holds a set points in original image coordinates
 * that are to be mapped to a rectangle in destination image coordinates.
 */
class PerspectiveParams
{
	// Member-wise copying is OK.
public:
	enum Corner { TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT };

	/**
	 * Defaults to all corners at origin (making isValid() return false)
	 * and MODE_AUTO.
	 */
	PerspectiveParams();
	
	PerspectiveParams(QDomElement const& el);

	bool isValid() const;

	void invalidate();

	AutoManualMode mode() const { return m_mode; }

	void setMode(AutoManualMode mode) { m_mode = mode; }

	QPointF const& corner(Corner idx) const { return m_corners[idx]; }

	void setCorner(Corner idx, QPointF const& corner) { m_corners[idx] = corner; }

	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	QPointF m_corners[4];
	AutoManualMode m_mode;
};

} // namespace deskew

#endif
