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

#ifndef OBJECT_SWAPPER_IMPL_GRID_H_
#define OBJECT_SWAPPER_IMPL_GRID_H_

#include "foundation_config.h"
#include "ObjectSwapperImpl.h"
#include "CopyableByMemcpy.h"
#include "AutoRemovingFile.h"
#include "Grid.h"
#include <QString>
#include <QFile>
#include <QTemporaryFile>
#include <QDebug>
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <stddef.h>

template<typename Node>
class ObjectSwapperImpl<Grid<Node> >
{
	BOOST_STATIC_ASSERT(CopyableByMemcpy<Node>::value == true);
public:
	ObjectSwapperImpl(QString const& swap_dir);

	boost::shared_ptr<Grid<Node> > swapIn();

	void swapOut(boost::shared_ptr<Grid<Node> > const& obj);
private:
	QString m_swapDir;
	AutoRemovingFile m_file;
	int m_width;
	int m_height;
	int m_padding;
};

template<typename Node>
ObjectSwapperImpl<Grid<Node> >::ObjectSwapperImpl(QString const& swap_dir)
:	m_swapDir(swap_dir)
,	m_width(0)
,	m_height(0)
,	m_padding(0)
{
}


template<typename Node>
boost::shared_ptr<Grid<Node> >
ObjectSwapperImpl<Grid<Node> >::swapIn()
{
	boost::shared_ptr<Grid<Node> > grid(new Grid<Node>(m_width, m_height, m_padding));

	QFile file(m_file.get());
	if (!file.open(file.ReadOnly)) {
		qDebug() << "Unable to load a grid from file: " << m_file.get();
		return grid;
	}

	size_t const bytes = (m_width + m_padding*2) * (m_height + m_padding*2) * sizeof(Node);
	if (file.size() != bytes) {
		qDebug() << "Unexpected size of file: " << m_file.get();
		return grid;
	}

	file.read((char*)grid->paddedData(), bytes);
	return grid;
}

template<typename Node>
void
ObjectSwapperImpl<Grid<Node> >::swapOut(boost::shared_ptr<Grid<Node> > const& obj)
{
	assert(obj.get());
	m_width = obj->width();
	m_height = obj->height();
	m_padding = obj->padding();

	if (!m_file.get().isEmpty()) {
		// We don't swap out the same stuff twice.
		return;
	}

	QTemporaryFile file(m_swapDir+"/XXXXXX.grid");
	if (!file.open()) {
		qDebug() << "Unable to create a temporary file in " << m_swapDir;
		return;
	}

	AutoRemovingFile remover(file.fileName());
	file.setAutoRemove(false);

	size_t const bytes = (m_width + m_padding*2) * (m_height + m_padding*2) * sizeof(Node);
	file.write((char const*)obj->paddedData(), bytes);

	m_file = remover;
}

#endif
