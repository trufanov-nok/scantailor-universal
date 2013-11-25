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

#include "MatchSizeMode.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

namespace page_layout
{

MatchSizeMode::MatchSizeMode(QDomElement const& el)
{
	QString const mode(el.attribute("mode"));
	
	if (mode == "disabled") {
		m_mode = DISABLED;
	} else if (mode == "scale") {
		m_mode = SCALE;
	} else {
		// Default mode for backwards compatibility.
		m_mode = GROW_MARGINS;
	}
}

QDomElement
MatchSizeMode::toXml(QDomDocument& doc, QString const& name) const
{
	char const* mode = "";

	switch (m_mode) {
		case DISABLED:
			mode = "disabled";
			break;
		case GROW_MARGINS:
			mode = "margins";
			break;
		case SCALE:
			mode = "scale";
			break;
	}
	
	QDomElement el(doc.createElement(name));
	el.setAttribute("mode", QLatin1String(mode));

	return el;
}

} // namespace page_layout

