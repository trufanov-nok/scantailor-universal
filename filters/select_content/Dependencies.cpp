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

#include "Dependencies.h"
#include <QDomDocument>
#include <QDomElement>
#include <utility>

namespace select_content
{

Dependencies::Dependencies()
{
}

Dependencies::Dependencies(QString const& transform_fingerprint)
:	m_transformFingerprint(transform_fingerprint)
{
}

Dependencies::Dependencies(QDomElement const& deps_el)
:	m_transformFingerprint(
		deps_el.namedItem(
			QStringLiteral("transform-fingerprint")
		).toElement().text().trimmed().toLatin1()
	)
{
}

Dependencies::~Dependencies()
{
}

bool
Dependencies::matches(Dependencies const& other) const
{
	return m_transformFingerprint == other.m_transformFingerprint;
}

QDomElement
Dependencies::toXml(QDomDocument& doc, QString const& name) const
{
	QDomElement fingerprint_el(
		doc.createElement(QStringLiteral("transform-fingerprint"))
	);
	fingerprint_el.appendChild(doc.createTextNode(m_transformFingerprint));

	QDomElement deps_el(doc.createElement(name));
	deps_el.appendChild(std::move(fingerprint_el));
	
	return deps_el;
}

} // namespace select_content
