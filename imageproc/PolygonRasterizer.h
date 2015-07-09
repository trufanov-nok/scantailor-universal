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

#ifndef IMAGEPROC_POLYGONRASTERIZER_H_
#define IMAGEPROC_POLYGONRASTERIZER_H_

#include "imageproc_config.h"
#include "BWColor.h"
#include <Qt>

class QPolygonF;
class QRectF;

namespace imageproc
{

class BinaryImage;
class GrayImage;

class IMAGEPROC_EXPORT PolygonRasterizer
{
public:
	static void fill(
		BinaryImage& image, BWColor color,
		QPolygonF const& poly, Qt::FillRule fill_rule);
	
	static void fillExcept(
		BinaryImage& image, BWColor color,
		QPolygonF const& poly, Qt::FillRule fill_rule);
	
	static void fill(
		GrayImage& image, unsigned char color,
		QPolygonF const& poly, Qt::FillRule fill_rule);
	
	static void fillExcept(
		GrayImage& image, unsigned char color,
		QPolygonF const& poly, Qt::FillRule fill_rule);
private:
	class Edge;
	class EdgeComponent;
	class EdgeOrderY;
	class EdgeOrderX;
	class Rasterizer;
};

} // namespace imageproc

#endif
