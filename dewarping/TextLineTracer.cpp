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

#include "TextLineTracer.h"
#include "TextLineRefiner.h"
#include "TextLineSegmenter.h"
#include "DetectVerticalBounds.h"
#include "TaskStatus.h"
#include "DebugImages.h"
#include "NumericTraits.h"
#include "VecNT.h"
#include "Grid.h"
#include "DistortionModelBuilder.h"
#include "Curve.h"
#include "math/ToLineProjector.h"
#include "math/SidesOfLine.h"
#include "math/LineBoundedByRect.h"
#include "imageproc/AffineTransformedImage.h"
#include "imageproc/AffineTransform.h"
#include "imageproc/Binarize.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/GrayImage.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/Sobel.h"
#include "imageproc/Constants.h"
#include <QPainter>
#include <QPen>
#include <QColor>
#include <QDebug>

using namespace imageproc;

namespace dewarping
{

void
TextLineTracer::trace(
	AffineTransformedImage const& input, DistortionModelBuilder& output,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	TaskStatus const& status, DebugImages* dbg)
{
	TextLineSegmenter::Result segmentation(
		TextLineSegmenter::process(input, accel_ops, status, dbg)
	);

	status.throwIfCancelled();

	AffineTransformedImage const downscaled = input.withAdjustedTransform(
		[](AffineImageTransform& xform) {
			xform.scaleTo(QSizeF(1600, 1600), Qt::KeepAspectRatio);
			xform.translateSoThatPointBecomes(
				xform.transformedCropArea().boundingRect().topLeft(), QPointF(0, 0)
			);
		}
	);

	status.throwIfCancelled();

	// Transform traced curves into downscaled coordinates.
	for (auto& polyline : segmentation.tracedCurves) {
		for (QPointF& pt : polyline) {
			pt = downscaled.xform().transform().map(pt);
		}
	}

	// Detect vertical bounds. The reason we do it in downscaled coordinates
	// is that we can hardcode the distance threshold in this case, as we
	// roughly know the size of the image.
	auto const vert_bounds = detectVerticalBounds(segmentation.tracedCurves, 10.0);
	if (vert_bounds.first.isNull() || vert_bounds.second.isNull()) {
		return;
	}

	filterShortCurves(segmentation.tracedCurves, vert_bounds.first, vert_bounds.second);
	filterOutOfBoundsCurves(segmentation.tracedCurves, vert_bounds.first, vert_bounds.second);

	QPolygonF const downscaled_crop_area(downscaled.xform().transformedCropArea());
	QRect const downscaled_rect(downscaled_crop_area.boundingRect().toRect());
	assert(downscaled_rect.topLeft() == QPoint(0, 0));

	GrayImage downscaled_image(
		accel_ops->affineTransform(
			GrayImage(downscaled.origImage()), downscaled.xform().transform(),
			downscaled_rect, OutsidePixels::assumeWeakNearest()
		)
	);

	status.throwIfCancelled();

	downscaled_image = GrayImage(binarizeGatos(downscaled_image, QSize(21, 21), 3.0).toQImage());

	status.throwIfCancelled();

	Grid<Vec2f> gradient(calcGradient(downscaled_image, dbg));

	status.throwIfCancelled();

	Grid<float> dir_deriv(calcDirectionalDerivative(gradient, segmentation.flowDirectionMap));

	status.throwIfCancelled();

	Grid<float> dir_deriv_pos(downscaled_image.width(), downscaled_image.height(), 0);
	Grid<float> dir_deriv_neg(downscaled_image.width(), downscaled_image.height(), 0);

	TextLineRefiner refiner(segmentation.tracedCurves, Vec2f(0, 1));

	static float const blur_sigmas[][2] = {
		{ 4.0f, 4.0f }, // h_sigma, v_sigma
		{ 1.5f, 1.5f }
	};

	int round = -1;
	for (auto const sigmas : blur_sigmas) {
		++round;

		rasterOpGeneric(
			[](float& pos, float dir_deriv) {
				pos = std::max(0.0f, -dir_deriv);
			},
			dir_deriv_pos, dir_deriv
		);

		status.throwIfCancelled();

		rasterOpGeneric(
			[](float& neg, float dir_deriv) {
				neg = std::max(0.0f, dir_deriv);
			},
			dir_deriv_neg, dir_deriv
		);

		status.throwIfCancelled();

		dir_deriv_pos = accel_ops->gaussBlur(dir_deriv_pos, sigmas[0], sigmas[1]);

		status.throwIfCancelled();

		dir_deriv_neg = accel_ops->gaussBlur(dir_deriv_neg, sigmas[0], sigmas[1]);

		status.throwIfCancelled();

		if (dbg) {
			dbg->add(
				visualizeGradient(downscaled_image, dir_deriv_neg),
				QString("top_attraction_force%1").arg(round + 1)
			);
			dbg->add(
				visualizeGradient(downscaled_image, dir_deriv_pos),
				QString("bottom_attraction_force%1").arg(round + 1)
			);
		}

		// The idea here is to make the impulse response of a gaussian to have the height of 1
		// rather than the area of 1.
		float const normalizer = 2.0 * constants::PI * std::sqrt(sigmas[0] * sigmas[1]);

		auto const top_attraction_force = [&dir_deriv_neg, normalizer](QPointF const& pos) {
			return normalizer * attractionForceAt(dir_deriv_neg, pos, 0.0f);
		};
		auto const bottom_attraction_force = [&dir_deriv_pos, normalizer](QPointF const& pos) {
			return normalizer * attractionForceAt(dir_deriv_pos, pos, 0.0f);
		};

		refiner.refine(
			top_attraction_force, bottom_attraction_force,
			/*iterations=*/100, TextLineRefiner::ON_CONVERGENCE_GO_FINER
		);

		status.throwIfCancelled();

		if (dbg) {
			auto bounds = output.verticalBounds();
			bounds.first = downscaled.xform().transform().map(bounds.first);
			bounds.second = downscaled.xform().transform().map(bounds.second);
			QImage const bg(visualizeVerticalBounds(downscaled_image, bounds));
			dbg->add(refiner.visualize(bg), QString("refined%1").arg(round + 1));
		}
	}

	QTransform const upscale_xform(downscaled.xform().transform().inverted());

	output.setVerticalBounds(
		upscale_xform.map(vert_bounds.first),
		upscale_xform.map(vert_bounds.second)
	);

	for (auto& polyline : refiner.refinedPolylines()) {
		for (QPointF& pt : polyline) {
			pt = upscale_xform.map(pt);
		}
		output.addHorizontalCurve(polyline);
	}
}

float
TextLineTracer::attractionForceAt(
	Grid<float> const& field, Vec2f pos, float outside_force)
{
	// We want attractionForceAt(..., Vec2f(0.5, 0.5), ...) to be interpreted
	// as "force in the middle of pixel (0, 0)" rather than "force half-way
	// between pixels at (0, 0) and (1, 1)".
	pos -= 0.5f;

	float const x_base = floor(pos[0]);
	float const y_base = floor(pos[1]);
	int const x_base_i = (int)x_base;
	int const y_base_i = (int)y_base;

	if (x_base_i < 0 || y_base_i < 0 || x_base_i + 1 >= field.width() || y_base_i + 1 >= field.height()) {
		return outside_force;
	}

	float const x = pos[0] - x_base;
	float const y = pos[1] - y_base;
	float const x1 = 1.0f - x;
	float const y1 = 1.0f - y;

	int const stride = field.stride();
	float const* base = field.data() + y_base_i * stride + x_base_i;

	return base[0]*x1*y1 + base[1]*x*y1 + base[stride]*x1*y + base[stride + 1]*x*y;
}

Grid<Vec2f>
TextLineTracer::calcGradient(
	imageproc::GrayImage const& image, DebugImages* dbg)
{
	int const width = image.width();
	int const height = image.height();
	Grid<Vec2f> grad(width, height, 0);

	// 8 to compensate the fact that Sobel operator approximates a derivative
	// multiplied by 8.
	float const downscale = 1.0f / (255 * 8);

	horizontalSobel<float>(
		width, height, image.data(), image.stride(),
		[=](uint8_t gray_level) { return gray_level * downscale; },
		grad.data(), grad.stride(),
		[](Vec2f& tmp, float val) { tmp[0] = val; },
		[](Vec2f const& tmp) { return tmp[0]; },
		grad.data(), grad.stride(),
		[](Vec2f& out, float val) { out[0] = -val; }
	);
	verticalSobel<float>(
		width, height, image.data(), image.stride(),
		[=](uint8_t gray_level) { return gray_level * downscale; },
		grad.data(), grad.stride(),
		[](Vec2f& tmp, float val) { tmp[1] = val; },
		[](Vec2f const& tmp) { return tmp[1]; },
		grad.data(), grad.stride(),
		[](Vec2f& out, float val) { out[1] = -val; }
	);

	return std::move(grad);
}

Grid<float>
TextLineTracer::calcDirectionalDerivative(
	Grid<Vec2f> const& gradient, Grid<Vec2f> const& downscaled_direction_map)
{
	int const width = gradient.width();
	int const height = gradient.height();

	double const x_downscale_factor = double(downscaled_direction_map.width() - 1) / double(width - 1);
	double const y_downscale_factor = double(downscaled_direction_map.height() - 1) / double(height - 1);

	Grid<float> dir_deriv(width, height);
	rasterOpGenericXY(
		[&downscaled_direction_map, x_downscale_factor, y_downscale_factor]
				(Vec2f const& gradient, float& deriv, int x, int y) {

			int const downscaled_x = std::lround(x * x_downscale_factor);
			int const downscaled_y = std::lround(y * y_downscale_factor);
			Vec2f dir = downscaled_direction_map(downscaled_x, downscaled_y);

			// dir points roughly to the right. We want it to point roughly down.
			std::swap(dir[0], dir[1]);
			dir[0] = -dir[0];

			deriv = dir.dot(gradient);
		},
		gradient, dir_deriv
	);

	return std::move(dir_deriv);
}

void
TextLineTracer::filterShortCurves(
	std::list<std::vector<QPointF>>& polylines,
	QLineF const& left_bound, QLineF const& right_bound)
{
	ToLineProjector const proj1(left_bound);
	ToLineProjector const proj2(right_bound);

	std::list<std::vector<QPointF> >::iterator it(polylines.begin());
	std::list<std::vector<QPointF> >::iterator const end(polylines.end());	
	while (it != end) {
		assert(!it->empty());
		QPointF const front(it->front());
		QPointF const back(it->back());
		double const front_proj_len = proj1.projectionDist(front);
		double const back_proj_len = proj2.projectionDist(back);
		double const chord_len = QLineF(front, back).length();

		if (front_proj_len + back_proj_len > 0.3 * chord_len) {
			polylines.erase(it++);
		} else {
			++it;
		}
	}
}

void
TextLineTracer::filterOutOfBoundsCurves(
	std::list<std::vector<QPointF>>& polylines,
	QLineF const& left_bound, QLineF const& right_bound)
{
	QPointF const left_midpoint(left_bound.pointAt(0.5));
	QPointF const right_midpoint(right_bound.pointAt(0.5));

	std::list<std::vector<QPointF> >::iterator it(polylines.begin());
	std::list<std::vector<QPointF> >::iterator const end(polylines.end());	
	while (it != end) {
		QPointF const chord_midpoint(0.5 * (it->front() + it->back()));
		if ((sidesOfLine(left_bound, it->front(), it->back()) >= 0 &&
				sidesOfLine(left_bound, chord_midpoint, right_midpoint) <= 0) ||
				(sidesOfLine(right_bound, it->front(), it->back()) >= 0 &&
				sidesOfLine(right_bound, chord_midpoint, left_midpoint) <= 0)) {
			polylines.erase(it++);
		} else {
			++it;
		}
	}
}

QImage
TextLineTracer::visualizeVerticalBounds(
	QImage const& background, std::pair<QLineF, QLineF> bounds)
{
	lineBoundedByRect(bounds.first, QRectF(background.rect()));
	lineBoundedByRect(bounds.second, QRectF(background.rect()));

	QImage canvas(background.convertToFormat(QImage::Format_RGB32));

	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::blue);
	pen.setWidthF(2.0);
	painter.setPen(pen);
	painter.setOpacity(0.7);

	painter.drawLine(bounds.first);
	painter.drawLine(bounds.second);

	return canvas;
}

QImage
TextLineTracer::visualizeGradient(QImage const& background, Grid<float> const& grad)
{
	int const width = grad.width();
	int const height = grad.height();
	int const grad_stride = grad.stride();

	// First let's find the maximum and minimum values.
	float min_value = NumericTraits<float>::max();
	float max_value = NumericTraits<float>::min();

	float const* grad_line = grad.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float const value = grad_line[x];
			if (value < min_value) {
				min_value = value;
			} else if (value > max_value) {
				max_value = value;
			}
		}
		grad_line += grad_stride;
	}

	float scale = std::max(max_value, -min_value);
	if (scale > std::numeric_limits<float>::epsilon()) {
		scale = 255.0f / scale;
	} 

	QImage overlay(width, height, QImage::Format_ARGB32_Premultiplied);
	uint32_t* overlay_line = (uint32_t*)overlay.bits();
	int const overlay_stride = overlay.bytesPerLine() / 4;

	grad_line = grad.data();
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			float const value = grad_line[x] * scale;
			int const magnitude = qBound(0, (int)(fabs(value) + 0.5), 255);
			if (value < 0) {
				overlay_line[x] = qRgba(0, 0, magnitude, magnitude);
			} else {
				overlay_line[x] = qRgba(magnitude, 0, 0, magnitude);
			}
		}
		grad_line += grad_stride;
		overlay_line += overlay_stride;
	}

	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.drawImage(0, 0, overlay);

	return canvas;
}

QImage
TextLineTracer::visualizePolylines(
	QImage const& background, std::list<std::vector<QPointF> > const& polylines,
	std::pair<QLineF, QLineF> const* vert_bounds)
{
	QImage canvas(background.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	QPainter painter(&canvas);
	painter.setRenderHint(QPainter::Antialiasing);
	QPen pen(Qt::blue);
	pen.setWidthF(3.0);
	painter.setPen(pen);

	for (std::vector<QPointF> const& polyline : polylines) {
		if (!polyline.empty()) {
			painter.drawPolyline(&polyline[0], polyline.size());
		}
	}

	if (vert_bounds) {
		painter.drawLine(vert_bounds->first);
		painter.drawLine(vert_bounds->second);
	}

	return canvas;
}

} // namespace dewarping
