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

#include "PerspectiveParams.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "dewarping/DistortionModel.h"
#include "dewarping/Curve.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>
#include <QPointF>
#include <vector>

namespace deskew
{

PerspectiveParams::PerspectiveParams()
:	m_mode(MODE_AUTO)
{
}

PerspectiveParams::PerspectiveParams(QDomElement const& el)
:	m_mode(el.attribute("mode") == QLatin1String("manual") ? MODE_MANUAL : MODE_AUTO)
{
	m_corners[TOP_LEFT] = XmlUnmarshaller::pointF(el.namedItem("tl").toElement());
	m_corners[TOP_RIGHT] = XmlUnmarshaller::pointF(el.namedItem("tr").toElement());
	m_corners[BOTTOM_RIGHT] = XmlUnmarshaller::pointF(el.namedItem("br").toElement());
	m_corners[BOTTOM_LEFT] = XmlUnmarshaller::pointF(el.namedItem("bl").toElement());
}

bool
PerspectiveParams::isValid() const
{
	dewarping::DistortionModel distortion_model;
	distortion_model.setTopCurve(
		std::vector<QPointF>{m_corners[TOP_LEFT], m_corners[TOP_RIGHT]}
	);
	distortion_model.setBottomCurve(
		std::vector<QPointF>{m_corners[BOTTOM_LEFT], m_corners[BOTTOM_RIGHT]}
	);
	return distortion_model.isValid();
}

void
PerspectiveParams::invalidate()
{
	*this = PerspectiveParams();
}

QDomElement
PerspectiveParams::toXml(QDomDocument& doc, QString const& name) const
{
	if (!isValid()) {
		return QDomElement();
	}

	XmlMarshaller marshaller(doc);

	QDomElement el(doc.createElement(name));
	el.setAttribute("mode", m_mode == MODE_MANUAL ? "manual" : "auto");
	el.appendChild(marshaller.pointF(m_corners[TOP_LEFT], "tl"));
	el.appendChild(marshaller.pointF(m_corners[TOP_RIGHT], "tr"));
	el.appendChild(marshaller.pointF(m_corners[BOTTOM_RIGHT], "br"));
	el.appendChild(marshaller.pointF(m_corners[BOTTOM_LEFT], "bl"));
	return el;
}

} // namespace deskew
