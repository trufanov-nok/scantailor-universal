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

#include "NonAcceleratedOperations.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/GaussBlur.h"
#include "imageproc/AffineTransform.h"
#include "imageproc/SavGolFilter.h"
#include "imageproc/Morphology.h"
#include "dewarping/RasterDewarper.h"
#include <stdexcept>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QSize>
#include <limits>

using namespace imageproc;

Grid<float>
NonAcceleratedOperations::gaussBlur(
	Grid<float> const& src, float h_sigma, float v_sigma) const
{
	Grid<float> dst(src.width(), src.height());

	gaussBlurGeneric(
		QSize(src.width(), src.height()), h_sigma, v_sigma,
		src.data(), src.stride(), [](float val) { return val; },
		dst.data(), dst.stride(), [](float& dst, float src) { dst = src; }
	);

	return dst;
}

Grid<float>
NonAcceleratedOperations::anisotropicGaussBlur(
	Grid<float> const& src, float dir_x, float dir_y,
	float dir_sigma, float ortho_dir_sigma) const
{
	Grid<float> dst(src.width(), src.height());

	anisotropicGaussBlurGeneric(
		QSize(src.width(), src.height()), dir_x, dir_y, dir_sigma, ortho_dir_sigma,
		src.data(), src.stride(), [](float val) { return val; },
		dst.data(), dst.stride(), [](float& dst, float src) { dst = src; }
	);

	return dst;
}

std::pair<Grid<float>, Grid<uint8_t>>
NonAcceleratedOperations::textFilterBank(
	Grid<float> const& src, std::vector<Vec2f> const& directions,
	std::vector<Vec2f> const& sigmas, float shoulder_length) const
{
	Grid<float> accum(src.width(), src.height());
	accum.initInterior(-std::numeric_limits<float>::max());

	Grid<uint8_t> direction_map(src.width(), src.height());
	direction_map.initInterior(0);

	QRect const rect(0, 0, src.width(), src.height());

	for (Vec2f const& s : sigmas) {
		for (size_t dir_idx = 0; dir_idx < directions.size(); ++dir_idx) {
			Vec2f const& dir = directions[dir_idx];

			//status.throwIfCancelled();

			Grid<float> blurred(src.width(), src.height(), /*padding=*/0);
			anisotropicGaussBlurGeneric(
				QSize(src.width(), src.height()), dir[0], dir[1], s[0], s[1],
				src.data(), src.stride(), [](float val) { return val; },
				blurred.data(), blurred.stride(), [](float& dst, float src) { dst = src; }
			);

			QPointF shoulder_f(dir[1], -dir[0]);
			shoulder_f *= s[1] * shoulder_length;
			QPoint const shoulder_i(shoulder_f.toPoint());

			rasterOpGenericXY(
				[rect, shoulder_i, &blurred, dir_idx](
						float& accum, uint8_t& direction_idx,
						float const origin_px, int x, int y) {

					QPoint const origin(x, y);
					QPoint const pt1(origin + shoulder_i);
					QPoint const pt2(origin - shoulder_i);

					float pt1_px = origin_px;
					float pt2_px = origin_px;

					if (rect.contains(pt1)) {
						pt1_px = blurred(pt1.x(), pt1.y());
					}
					if (rect.contains(pt2)) {
						pt2_px = blurred(pt2.x(), pt2.y());
					}

					float const response = 0.5f * (pt1_px + pt2_px) - origin_px;
					if (response > accum) {
						accum = response;
						direction_idx = dir_idx;
					}
				},
				accum, direction_map, blurred
			);
		}
	}

	return std::make_pair(std::move(accum), std::move(direction_map));
}

QImage
NonAcceleratedOperations::dewarp(
	QImage const& src, QSize const& dst_size,
	dewarping::CylindricalSurfaceDewarper const& distortion_model,
	QRectF const& model_domain, QColor const& background_color,
	float min_density, float max_density,
	QSizeF const& min_mapping_area) const
{
	return dewarping::RasterDewarper::dewarp(
		src, dst_size, distortion_model, model_domain, background_color,
		min_density, max_density, min_mapping_area
	);
}

QImage
NonAcceleratedOperations::affineTransform(
	QImage const& src, QTransform const& xform,
	QRect const& dst_rect, imageproc::OutsidePixels const& outside_pixels,
	QSizeF const& min_mapping_area) const
{
	return imageproc::affineTransform(src, xform, dst_rect, outside_pixels, min_mapping_area);
}

GrayImage
NonAcceleratedOperations::renderPolynomialSurface(
	PolynomialSurface const& surface, int width, int height)
{
	return surface.render(QSize(width, height));
}

GrayImage
NonAcceleratedOperations::savGolFilter(
	imageproc::GrayImage const& src, QSize const& window_size,
	int hor_degree, int vert_degree)
{
	return imageproc::savGolFilter(src, window_size, hor_degree, vert_degree);
}

void
NonAcceleratedOperations::hitMissReplaceInPlace(
	imageproc::BinaryImage& img, imageproc::BWColor const img_surroundings,
	std::vector<Grid<char>> const& patterns)
{
	for (Grid<char> const& pattern : patterns) {
		if (pattern.stride() != pattern.width()) {
			throw std::invalid_argument(
				"NonAcceleratedOperations::hitMissReplaceInPlace: "
				"patterns with extended stride are not supported"
			);
		}

		imageproc::hitMissReplaceInPlace(
			img, img_surroundings, pattern.data(), pattern.width(), pattern.height()
		);
	}
}
