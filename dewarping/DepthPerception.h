/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef DEWARPING_DEPTH_PERCEPTION_H_
#define DEWARPING_DEPTH_PERCEPTION_H_

#include "dewarping_config.h"
#include <QString>

namespace dewarping
{

/**
 * \see CylindricalSurfaceDewarper
 */
class DEWARPING_EXPORT DepthPerception
{
public:
	DepthPerception();

	DepthPerception(double value);

	explicit DepthPerception(QString const& from_string);
	
	QString toString() const;

	void setValue(double value);

	double value() const { return m_value; }

	static double minValue() { return 1.0; }

	static double defaultValue() { return 2.0; }

	static double maxValue() { return 3.0; }
private:
	double m_value;
};

} // namespace dewarping

#endif
