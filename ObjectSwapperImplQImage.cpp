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

#include "ObjectSwapperImplQImage.h"
#include <QTemporaryFile>
#include <QImageWriter>
#include <QDebug>
#include <assert.h>

ObjectSwapperImpl<QImage>::ObjectSwapperImpl(QString const& swap_dir)
:	m_swapDir(swap_dir)
{
}

boost::shared_ptr<QImage>
ObjectSwapperImpl<QImage>::swapIn()
{
	return boost::shared_ptr<QImage>(new QImage(m_file.get()));
}

void
ObjectSwapperImpl<QImage>::swapOut(boost::shared_ptr<QImage> const& obj)
{
	assert(obj.get());

	if (!m_file.get().isEmpty()) {
		// We don't swap out the same stuff twice.
		return;
	}

	QTemporaryFile file(m_swapDir+"/XXXXXX.png");
	if (!file.open()) {
		qDebug() << "Unable to create a temporary file in " << m_swapDir;
		return;
	}

	AutoRemovingFile remover(file.fileName());
	file.setAutoRemove(false);

	QImageWriter writer(&file, "png");
	writer.setCompression(2); // Trade space for speed.
	if (!writer.write(*obj)) {
		qDebug() << "Unable to swap out an image";
		return;
	}

	m_file = remover;
}
