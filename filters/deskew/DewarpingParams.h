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

#ifndef DESKEW_DEWARPING_PARAMS_H_
#define DESKEW_DEWARPING_PARAMS_H_

#include "AutoManualMode.h"
#include "dewarping/DistortionModel.h"
#include "dewarping/DepthPerception.h"

class QDomDocument;
class QDomElement;
class QString;

namespace deskew
{

class DewarpingParams
{
	// Member-wise copying is OK.
public:
	/** Defaults to invalid state with MODE_AUTO. */
	DewarpingParams();
	
	DewarpingParams(QDomElement const& el);

	~DewarpingParams();

	QDomElement toXml(QDomDocument& doc, QString const& name) const;

	bool isValid() const;

	void invalidate();

	AutoManualMode mode() const { return m_mode; }

	void setMode(AutoManualMode mode) { m_mode = mode; }

	dewarping::DistortionModel const& distortionModel() const {
		return m_distortionModel;
	}

	void setDistortionModel(dewarping::DistortionModel const& distortion_model) {
		m_distortionModel = distortion_model;
	}

	dewarping::DepthPerception const& depthPerception() const {
		return m_depthPerception;
	}

	void setDepthPerception(dewarping::DepthPerception const& depth_perception) {
		m_depthPerception = depth_perception;
	}
private:
	dewarping::DistortionModel m_distortionModel;
	dewarping::DepthPerception m_depthPerception;
	AutoManualMode m_mode;
};

} // namespace deskew

#endif
