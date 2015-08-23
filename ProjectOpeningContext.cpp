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

#include "ProjectOpeningContext.h"
#include "ProjectPages.h"
#include <QString>
#include <QMessageBox>
#include <Qt>
#include <algorithm>
#include <assert.h>

ProjectOpeningContext::ProjectOpeningContext(
	QWidget* parent, QString const& project_file, QDomDocument const& doc)
:	m_projectFile(project_file),
	m_reader(doc),
	m_pParent(parent)
{
}

ProjectOpeningContext::~ProjectOpeningContext()
{
}

void
ProjectOpeningContext::proceed()
{
	if (!m_reader.success()) {
		deleteLater();
		QMessageBox::warning(
			m_pParent, tr("Error"),
			tr("Unable to interpret the project file.")
		);
		return;
	}
	
	deleteLater();
	emit done(this);
}
