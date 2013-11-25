/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "RotationParams.h"
#include "../../Utils.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <cmath>

namespace deskew
{

RotationParams::RotationParams()
:	m_compensationAngleDeg(0)
,	m_mode(MODE_AUTO)
,	m_isValid(false)
{
}

RotationParams::RotationParams(QDomElement const& el)
:	m_mode(el.attribute("mode") == QLatin1String("manual") ? MODE_MANUAL : MODE_AUTO)
{
	m_compensationAngleDeg = el.attribute("angle").toDouble(&m_isValid);
}

void
RotationParams::invalidate()
{
	*this = RotationParams();
}

void
RotationParams::setCompensationAngleDeg(double angle_deg)
{
	using namespace std;

	if (isfinite(angle_deg))
	{
		m_compensationAngleDeg = angle_deg;
		m_isValid = true;
	}
}

QDomElement
RotationParams::toXml(QDomDocument& doc, QString const& name) const
{
	if (!m_isValid) {
		return QDomElement();
	}

	QDomElement el(doc.createElement(name));
	el.setAttribute("angle", Utils::doubleToString(m_compensationAngleDeg));
	el.setAttribute("mode", m_mode == MODE_AUTO ? "auto" : "manual");
	return el;
}

} // namespace deskew
