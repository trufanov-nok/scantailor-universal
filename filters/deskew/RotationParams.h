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

#ifndef DESKEW_ROTATION_PARAMS_H_
#define DESKEW_ROTATION_PARAMS_H_

#include "AutoManualMode.h"

class QDomDocument;
class QDomElement;
class QString;

namespace deskew
{

class RotationParams
{
	// Member-wise copying is OK.
public:
	/**
	 * Constructs RotationParams with MODE_AUTO and an invalid compensation angle.
	 */
	RotationParams();
	
	RotationParams(QDomElement const& el);

	bool isValid() const { return m_isValid; }

	void invalidate();

	AutoManualMode mode() const { return m_mode; }

	void setMode(AutoManualMode mode) { m_mode = mode; }

	double compensationAngleDeg() const { return m_compensationAngleDeg; }

	void setCompensationAngleDeg(double angle_deg);

	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	double m_compensationAngleDeg;
	AutoManualMode m_mode;
	bool m_isValid;
};

} // namespace deskew

#endif
