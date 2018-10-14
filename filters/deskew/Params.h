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

#include "Dependencies.h"
#include "AutoManualMode.h"
#include "RegenParams.h"
#include <QString>
#include <cmath>
#include <algorithm>

#include "CommandLine.h"
class CommandLine;

class QDomDocument;
class QDomElement;

namespace deskew
{

class Params: public RegenParams
{
public:
	// Member-wise copying is OK.

    enum OrientationFix {
        OrientationFixNone,
        OrientationFixLeft,
        OrientationFixRight
    };

    enum RequireRecalcs {
        RecalcNone,
        RecalcAutoDeskew,
        RecalcOrientation
    };
	
	Params(double deskew_angle_deg,
        Dependencies const& deps, AutoManualMode const mode, OrientationFix pageRotation = OrientationFixNone, bool requireRecalc = false);
	
	Params(QDomElement const& deskew_el); 
	
	~Params();
	
    OrthogonalRotation pageRotation() {
        OrthogonalRotation r = m_deps.imageRotation();
        if (m_orientationFix == OrientationFixLeft) {
            r.prevClockwiseDirection();
        } else if (m_orientationFix == OrientationFixRight) {
            r.nextClockwiseDirection();
        }
        return r;
    }

    OrientationFix orientationFix() const { return m_orientationFix; }

    void setOrientationFix(OrientationFix orient) { m_orientationFix = orient; }

	double deskewAngle() const { return m_deskewAngleDeg; }

    void setDeskewAngle(double angle) { m_deskewAngleDeg = angle; }


	double deviation() const { return m_deviation; }
	void computeDeviation(double avg) { m_deviation = avg - m_deskewAngleDeg; }
	// check if skew is too large
	bool isDeviant(double std, double max_dev=CommandLine::get().getSkewDeviation()) const { return std::max(1.5*std, max_dev) < fabs(m_deviation); }

	Dependencies const& dependencies() const { return m_deps; }
	
	AutoManualMode mode() const { return m_mode; }

    void setMode(AutoManualMode const mode) { m_mode = mode; }

    bool requireRecalc() const { return m_requireRecalc; }

    void setRequireRecalc(bool flags) { m_requireRecalc = flags; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	double m_deskewAngleDeg;
	Dependencies m_deps;
	AutoManualMode m_mode;
	double m_deviation;
    OrientationFix m_orientationFix;
    bool m_requireRecalc;
};

} // namespace deskew

#endif
