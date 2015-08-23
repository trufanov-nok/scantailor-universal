/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "ProjectCreationContext.h"
#include "ProjectFilesDialog.h"
#include "ImageFileInfo.h"
#include <QString>
#include <Qt>
#include <algorithm>
#include <assert.h>

ProjectCreationContext::ProjectCreationContext(QWidget* parent)
:	m_layoutDirection(Qt::LeftToRight),
	m_pParent(parent)
{
	showProjectFilesDialog();
}

ProjectCreationContext::~ProjectCreationContext()
{
	// Deleting a null pointer is OK.
	delete m_ptrProjectFilesDialog;
}

void
ProjectCreationContext::projectFilesSubmitted()
{
	m_files = m_ptrProjectFilesDialog->inProjectFiles();
	m_outDir = m_ptrProjectFilesDialog->outputDirectory();
	m_layoutDirection = Qt::LeftToRight;
	if (m_ptrProjectFilesDialog->isRtlLayout()) {
		m_layoutDirection = Qt::RightToLeft;
	}

	emit done(this);
}

void
ProjectCreationContext::projectFilesDialogDestroyed()
{
	deleteLater();
}

void
ProjectCreationContext::showProjectFilesDialog()
{
	assert(!m_ptrProjectFilesDialog);
	m_ptrProjectFilesDialog = new ProjectFilesDialog(m_pParent);
	m_ptrProjectFilesDialog->setAttribute(Qt::WA_DeleteOnClose);
	m_ptrProjectFilesDialog->setAttribute(Qt::WA_QuitOnClose, false);
	if (m_pParent) {
		m_ptrProjectFilesDialog->setWindowModality(Qt::WindowModal);
	}
	connect(
		m_ptrProjectFilesDialog, SIGNAL(accepted()),
		this, SLOT(projectFilesSubmitted())
	);
	connect(
		m_ptrProjectFilesDialog, SIGNAL(destroyed(QObject*)),
		this, SLOT(projectFilesDialogDestroyed())
	);
	m_ptrProjectFilesDialog->show();
}
