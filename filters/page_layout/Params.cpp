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
#include "XmlMarshaller.h"
#include "XmlUnmarshaller.h"
#include <QDomDocument>
#include <QDomElement>
#include <QString>

namespace page_layout
{

Params::Params(
	RelativeMargins const& hard_margins,
	QSizeF const& content_size,
	MatchSizeMode const& match_size_mode,
	Alignment const& alignment)
:	m_hardMargins(hard_margins),
	m_contentSize(content_size),
	m_matchSizeMode(match_size_mode),
	m_alignment(alignment)
{
}

Params::Params(QDomElement const& el)
:	m_hardMargins(
		XmlUnmarshaller::relativeMargins(
			el.namedItem("hardMargins").toElement()
		)
	),
	m_contentSize(
		XmlUnmarshaller::sizeF(
			el.namedItem("contentSize").toElement()
		)
	),
	m_matchSizeMode(el.namedItem("matchSizeMode").toElement()),
	m_alignment(el.namedItem("alignment").toElement())
{
}

QDomElement
Params::toXml(QDomDocument& doc, QString const& name) const
{
	XmlMarshaller marshaller(doc);
	
	QDomElement el(doc.createElement(name));
	el.appendChild(marshaller.relativeMargins(m_hardMargins, "hardMargins"));
	el.appendChild(marshaller.sizeF(m_contentSize, "contentSize"));
	el.appendChild(m_matchSizeMode.toXml(doc, "matchSizeMode"));
	el.appendChild(m_alignment.toXml(doc, "alignment"));
	return el;
}

} // namespace page_layout
