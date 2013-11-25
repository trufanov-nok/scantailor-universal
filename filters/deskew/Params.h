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

#ifndef DESKEW_PARAMS_H_
#define DESKEW_PARAMS_H_

#include "DistortionType.h"
#include "RotationParams.h"
#include "PerspectiveParams.h"
#include "DewarpingParams.h"
#include "Dependencies.h"
#include <QString>
#include <QPolygonF>

class QDomDocument;
class QDomElement;

namespace deskew
{

class Params
{
	// Member-wise copying is OK.
public:
	Params(Dependencies const& deps);
	
	Params(QDomElement const& deskew_el);
	
	~Params();

	static DistortionType defaultDistortionType();

	DistortionType distortionType() const { return m_distortionType; }

	void setDistortionType(DistortionType type) { m_distortionType = type; }

	RotationParams& rotationParams() { return m_rotationParams; }

	RotationParams const& rotationParams() const { return m_rotationParams; }

	PerspectiveParams& perspectiveParams() { return m_perspectiveParams; }

	PerspectiveParams const& perspectiveParams() const { return m_perspectiveParams; }

	DewarpingParams& dewarpingParams() { return m_dewarpingParams; }

	DewarpingParams const& dewarpingParams() const { return m_dewarpingParams; }

	Dependencies const& dependencies() const { return m_deps; }

	/**
	 * Looks up the mode associated with the current distortion type.
	 * DistortionType::NONE always produces MODE_MANUAL.
	 */
	AutoManualMode mode() const;

	/**
	 * We may have enough / valid data for some distortion types but
	 * not for others.
	 */
	bool validForDistortionType(DistortionType const& distortion_type) const;

	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	void takeManualSettingsFrom(Params const& other);
private:
	DistortionType m_distortionType;
	RotationParams m_rotationParams;
	PerspectiveParams m_perspectiveParams;
	DewarpingParams m_dewarpingParams;
	Dependencies m_deps;
};

} // namespace deskew

#endif
