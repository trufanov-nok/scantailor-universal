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

#include "DewarpingParams.h"
#include "dewarping/DistortionModel.h"
#include "dewarping/DepthPerception.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

namespace deskew
{

DewarpingParams::DewarpingParams()
:	m_mode(MODE_AUTO)
{
}
	
DewarpingParams::DewarpingParams(QDomElement const& el)
:	m_distortionModel(el.namedItem("distortion-model").toElement())
,	m_depthPerception(el.attribute("depthPerception"))
,	m_mode(el.attribute("mode") == QLatin1String("manual") ? MODE_MANUAL : MODE_AUTO)
{
}

DewarpingParams::~DewarpingParams()
{
}

bool
DewarpingParams::isValid() const
{
	return m_distortionModel.isValid();
}

void
DewarpingParams::invalidate()
{
	*this = DewarpingParams();
}

QDomElement
DewarpingParams::toXml(QDomDocument& doc, QString const& name) const
{
	if (!isValid()) {
		return QDomElement();
	}

	QDomElement el(doc.createElement(name));
	el.appendChild(m_distortionModel.toXml(doc, "distortion-model"));
	el.setAttribute("depthPerception", m_depthPerception.toString());
	el.setAttribute("mode", m_mode == MODE_MANUAL ? "manual" : "auto");
	return el;
}

} // namespace deskew
