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

#include "DistortionType.h"
#include <QString>

namespace deskew
{

namespace str
{

static char const NONE[] = "none";
static char const ROTATION[] = "rotation";
static char const PERSPECTIVE[] = "perspective";
static char const WARP[] = "warp";

}

DistortionType::DistortionType(Type type)
:	m_type(type)
{
}

DistortionType::DistortionType(QString const& from_string)
{
	if (from_string == QLatin1String(str::ROTATION)) {
		m_type = ROTATION;
	} else if (from_string == QLatin1String(str::PERSPECTIVE)) {
		m_type = PERSPECTIVE;
	} else if (from_string == QLatin1String(str::WARP)) {
		m_type = WARP;
	} else {
		m_type = NONE;
	}
}

QString
DistortionType::toString() const
{
	char const* s = str::NONE;

	switch (m_type) {
		case NONE:
			s = str::NONE;
			break;
		case ROTATION:
			s = str::ROTATION;
			break;
		case PERSPECTIVE:
			s = str::PERSPECTIVE;
			break;
		case WARP:
			s = str::WARP;
			break;
	}

	return QLatin1String(s);
}

} // namespace deskew
