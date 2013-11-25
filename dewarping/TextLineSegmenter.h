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

#ifndef DEWARPING_TEXT_LINE_SEGMENTER2_H_
#define DEWARPING_TEXT_LINE_SEGMENTER2_H_

#include "NonCopyable.h"
#include "Grid.h"
#include <QPointF>
#include <list>
#include <vector>

class DebugImages;
class AffineTransformedImage;
class TaskStatus;
class QImage;
class QPolygonF;
class QLineF;

namespace imageproc
{
	class GrayImage;
	class BinaryImage;
	class ConnectivityMap;
}

namespace dewarping
{

class DistortionModelBuilder;

class TextLineSegmenter
{
	DECLARE_NON_COPYABLE(TextLineSegmenter);
public:
	/**
	 * Returns a list polylines in image.origImage() coordinates.
	 */
	static std::list<std::vector<QPointF>> process(
		AffineTransformedImage const& image,
		DistortionModelBuilder& model_builder,
		TaskStatus const& status, DebugImages* dbg = nullptr);
private:
	enum LeftRightSide { LEFT_SIDE, RIGHT_SIDE };

	static std::list<std::vector<QPointF>> processDownscaled(
		imageproc::GrayImage const& image, QPolygonF const& crop_area,
		TaskStatus const& status, DebugImages* dbg);

	static double findSkewAngleRad(
		imageproc::GrayImage const& image,
		TaskStatus const& status, DebugImages* dbg);

	static imageproc::ConnectivityMap initialSegmentation(
		imageproc::GrayImage const& image, QPolygonF const& crop_area,
		Grid<float>& blurred, imageproc::BinaryImage const& peaks,
		TaskStatus const& status, DebugImages* dbg);

	static std::list<std::vector<QPointF>> refineSegmentation(
		imageproc::GrayImage const& image, QPolygonF const& crop_area,
		imageproc::ConnectivityMap& cmap, TaskStatus const& status, DebugImages* dbg);

	static QLineF findVerticalBound(
		std::vector<QLineF> const& chords, LeftRightSide side);

	static imageproc::BinaryImage calcPageMask(
		imageproc::GrayImage const& no_content,
		TaskStatus const& status, DebugImages* dbg);

	static void maskTextLines(
		std::list<std::vector<QPointF>>& lines,
		imageproc::GrayImage const& image,
		imageproc::BinaryImage const& mask,
		TaskStatus const& status, DebugImages* dbg);
};

} // namespace dewarping

#endif
