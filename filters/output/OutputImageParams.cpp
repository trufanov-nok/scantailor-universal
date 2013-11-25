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

#include "OutputImageParams.h"
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include "../../Utils.h"
#include <QPolygonF>
#include <QDomDocument>
#include <QDomElement>
#include <cmath>
#include <utility>

namespace output
{

OutputImageParams::OutputImageParams(
	QString const& transform_fingerprint,
	QRect const& output_image_rect, QRect const& content_rect,
	ColorParams const& color_params, DespeckleLevel despeckle_level)
:	m_transformFingerprint(transform_fingerprint),
	m_outputImageRect(output_image_rect),
	m_contentRect(content_rect),
	m_colorParams(color_params),
	m_despeckleLevel(despeckle_level)
{
}

OutputImageParams::OutputImageParams(QDomElement const& el)
:	m_transformFingerprint(
		el.namedItem(
			QStringLiteral("transform-fingerprint")
		).toElement().text().trimmed().toLatin1()
	)
,	m_outputImageRect(XmlUnmarshaller::rect(el.namedItem("output-image-rect").toElement())),
	m_contentRect(XmlUnmarshaller::rect(el.namedItem("content-rect").toElement())),
	m_colorParams(el.namedItem("color-params").toElement()),
	m_despeckleLevel(despeckleLevelFromString(el.attribute("despeckleLevel")))
{
}

QDomElement
OutputImageParams::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement fingerprint_el(
		doc.createElement(QStringLiteral("transform-fingerprint"))
	);
	fingerprint_el.appendChild(doc.createTextNode(m_transformFingerprint));

	QDomElement el(doc.createElement(name));
	el.appendChild(std::move(fingerprint_el));
	el.appendChild(marshaller.rect(m_outputImageRect, "output-image-rect"));
	el.appendChild(marshaller.rect(m_contentRect, "content-rect"));
	el.appendChild(m_colorParams.toXml(doc, "color-params"));
	el.setAttribute("despeckleLevel", despeckleLevelToString(m_despeckleLevel));
	
	return el;
}

bool
OutputImageParams::matches(OutputImageParams const& other) const
{
	if (m_outputImageRect != other.m_outputImageRect) {
		return false;
	}
	
	if (m_contentRect != other.m_contentRect) {
		return false;
	}
	
	if (!colorParamsMatch(m_colorParams, m_despeckleLevel,
			other.m_colorParams, other.m_despeckleLevel)) {
		return false;
	}
	
	return true;
}

bool
OutputImageParams::colorParamsMatch(
	ColorParams const& cp1, DespeckleLevel const dl1,
	ColorParams const& cp2, DespeckleLevel const dl2)
{
	if (cp1.colorMode() != cp2.colorMode()) {
		return false;
	}
	
	switch (cp1.colorMode()) {
		case ColorParams::COLOR_GRAYSCALE:
		case ColorParams::MIXED:
			if (cp1.colorGrayscaleOptions() != cp2.colorGrayscaleOptions()) {
				return false;
			}
			break;
		default:;
	}
	
	switch (cp1.colorMode()) {
		case ColorParams::BLACK_AND_WHITE:
		case ColorParams::MIXED:
			if (cp1.blackWhiteOptions() != cp2.blackWhiteOptions()) {
				return false;
			}
			if (dl1 != dl2) {
				return false;
			}
			break;
		default:;
	}
	
	return true;
}

} // namespace output
