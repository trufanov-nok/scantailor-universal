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

#include "AffineTransformedImage.h"
#include <QSize>
#include <cmath>
#include <assert.h>

AffineTransformedImage::AffineTransformedImage(QImage const& image)
:	m_origImage(image)
,	m_xform(image.size())
{
	assert(!m_origImage.isNull());
}

AffineTransformedImage::AffineTransformedImage(
	QImage const& image, AffineImageTransform const& xform)
:	m_origImage(image)
,	m_xform(xform)
{
	assert(!m_origImage.isNull());
	assert(m_xform.origSize() == m_origImage.size());
}
