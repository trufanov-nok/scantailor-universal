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

#include "Params.h"
#include "../../Utils.h"
#include <QDomDocument>
#include <QDomElement>

namespace deskew
{

Params::Params(Dependencies const& deps)
:	m_distortionType(defaultDistortionType())
,	m_deps(deps)
{
}

Params::Params(QDomElement const& deskew_el)
:	m_distortionType(deskew_el.attribute("distortionType"))
,	m_rotationParams(deskew_el.namedItem("rotation").toElement())
,	m_perspectiveParams(deskew_el.namedItem("perspective").toElement())
,	m_dewarpingParams(deskew_el.namedItem("warp").toElement())
,	m_deps(deskew_el.namedItem("dependencies").toElement())
{
}

Params::~Params()
{
}

DistortionType
Params::defaultDistortionType()
{
	return DistortionType::ROTATION;
}

AutoManualMode
Params::mode() const
{
	switch (m_distortionType.get()) {
		case DistortionType::NONE:
			return MODE_MANUAL;
		case DistortionType::ROTATION:
			return m_rotationParams.mode();
		case DistortionType::PERSPECTIVE:
			return m_perspectiveParams.mode();
		case DistortionType::WARP:
			return m_dewarpingParams.mode();
	}

	assert(!"unreachable");
	return MODE_AUTO;
}

bool
Params::validForDistortionType(DistortionType const& distortion_type) const
{
	switch (distortion_type.get()) {
		case DistortionType::NONE:
			return true;
		case DistortionType::ROTATION:
			return m_rotationParams.isValid();
		case DistortionType::PERSPECTIVE:
			return m_perspectiveParams.isValid();
		case DistortionType::WARP:
			return m_dewarpingParams.isValid();
	}

	assert(!"unreachable");
	return false;
}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement el(doc.createElement(name));
	el.setAttribute("distortionType", m_distortionType.toString());
	el.appendChild(m_rotationParams.toXml(doc, "rotation"));
	el.appendChild(m_perspectiveParams.toXml(doc, "perspective"));
	el.appendChild(m_dewarpingParams.toXml(doc, "warp"));
	el.appendChild(m_deps.toXml(doc, "dependencies"));
	return el;
}

void
Params::takeManualSettingsFrom(Params const& other)
{
	// These settings are specified manually even in automatic mode,
	// so we want to preserve them after a dependency mismatch.
	m_distortionType = other.distortionType();
	m_dewarpingParams.setDepthPerception(other.dewarpingParams().depthPerception());
}

} // namespace deskew
