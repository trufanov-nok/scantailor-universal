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

#ifndef PAGE_LAYOUT_PARAMS_H_
#define PAGE_LAYOUT_PARAMS_H_

#include "RelativeMargins.h"
#include "MatchSizeMode.h"
#include "Alignment.h"
#include <QSizeF>

class QDomDocument;
class QDomElement;
class QString;

namespace page_layout
{

class Params
{
	// Member-wise copying is OK.
public:
	Params(RelativeMargins const& hard_margins,
		QSizeF const& content_size,
		MatchSizeMode const& match_size_mode,
		Alignment const& alignment);
	
	Params(QDomElement const& el);
	
	RelativeMargins const& hardMargins() const { return m_hardMargins; }
	
	QSizeF const& contentSize() const { return m_contentSize; }
	
	MatchSizeMode matchSizeMode() const { return m_matchSizeMode; }

	Alignment const& alignment() const { return m_alignment; }
	
	QDomElement toXml(QDomDocument& doc, QString const& name) const;
private:
	RelativeMargins m_hardMargins;
	QSizeF m_contentSize;
	MatchSizeMode m_matchSizeMode;
	Alignment m_alignment;
};

} // namespace page_layout

#endif
