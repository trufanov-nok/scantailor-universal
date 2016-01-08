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

#include "OutputGenerator.h"
#include "TaskStatus.h"
#include "Utils.h"
#include "DebugImages.h"
#include "EstimateBackground.h"
#include "Despeckle.h"
#include "RenderParams.h"
#include "Zone.h"
#include "ZoneSet.h"
#include "PictureLayerProperty.h"
#include "FillColorProperty.h"
#include "Grid.h"
#include "dewarping/DistortionModel.h"
#include "imageproc/AffineImageTransform.h"
#include "imageproc/AffineTransform.h"
#include "imageproc/GrayImage.h"
#include "imageproc/BinaryImage.h"
#include "imageproc/BinaryThreshold.h"
#include "imageproc/Binarize.h"
#include "imageproc/BWColor.h"
#include "imageproc/Scale.h"
#include "imageproc/Morphology.h"
#include "imageproc/SeedFill.h"
#include "imageproc/Constants.h"
#include "imageproc/Grayscale.h"
#include "imageproc/RasterOp.h"
#include "imageproc/RasterOpGeneric.h"
#include "imageproc/PolynomialSurface.h"
#include "imageproc/SavGolFilter.h"
#include "imageproc/DrawOver.h"
#include "imageproc/AdjustBrightness.h"
#include "imageproc/PolygonRasterizer.h"
#include "config.h"
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <QImage>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QPointF>
#include <QPolygonF>
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QBrush>
#include <QtGlobal>
#include <QDebug>
#include <Qt>
#include <vector>
#include <memory>
#include <new>
#include <algorithm>
#include <assert.h>
#include <string.h>
#include <stdint.h>

using namespace imageproc;
using namespace dewarping;

namespace output
{

namespace
{

/**
 * In picture areas we make sure we don't use pure black and pure white colors.
 * These are reserved for text areas.  This behaviour makes it possible to
 * detect those picture areas later and treat them differently, for example
 * encoding them as a background layer in DjVu format.
 */
template<typename PixelType>
PixelType reserveBlackAndWhite(PixelType color);

template<>
uint32_t reserveBlackAndWhite(uint32_t color)
{
	// We handle both RGB32 and ARGB32 here.
	switch (color & 0x00FFFFFF) {
		case 0x00000000:
			return 0xFF010101;
		case 0x00FFFFFF:
			return 0xFFFEFEFE;
		default:
			return color;
	}
}

template<>
uint8_t reserveBlackAndWhite(uint8_t color)
{
	switch (color) {
		case 0x00:
			return 0x01;
		case 0xFF:
			return 0xFE;
		default:
			return color;
	}
}

template<typename PixelType>
void reserveBlackAndWhite(QSize size, int stride, PixelType* data)
{
	int const width = size.width();
	int const height = size.height();

	PixelType* line = data;
	for (int y = 0; y < height; ++y, line += stride) {
		for (int x = 0; x < width; ++x) {
			line[x] = reserveBlackAndWhite<PixelType>(line[x]);
		}
	}
}

void reserveBlackAndWhite(QImage& img)
{
	assert(img.depth() == 8 || img.depth() == 24 || img.depth() == 32);
	switch (img.format()) {
		case QImage::Format_Indexed8:
			reserveBlackAndWhite(img.size(), img.bytesPerLine(), img.bits());
			break;
		case QImage::Format_RGB32:
		case QImage::Format_ARGB32:
			reserveBlackAndWhite(img.size(), img.bytesPerLine()/4, (uint32_t*)img.bits());
			break;
		default:; // Should not happen.
	}
}

/**
 * Fills areas of \p mixed with pixels from \p bw_content in
 * areas where \p bw_mask is black.  Supported \p mixed image formats
 * are Indexed8 grayscale, RGB32 and ARGB32.
 * The \p MixedPixel type is uint8_t for Indexed8 grayscale and uint32_t
 * for RGB32 and ARGB32.
 */
template<typename MixedPixel>
void combineMixed(
	QImage& mixed, BinaryImage const& bw_content,
	BinaryImage const& bw_mask)
{
	MixedPixel* mixed_line = reinterpret_cast<MixedPixel*>(mixed.bits());
	int const mixed_stride = mixed.bytesPerLine() / sizeof(MixedPixel);
	uint32_t const* bw_content_line = bw_content.data();
	int const bw_content_stride = bw_content.wordsPerLine();
	uint32_t const* bw_mask_line = bw_mask.data();
	int const bw_mask_stride = bw_mask.wordsPerLine();
	int const width = mixed.width();
	int const height = mixed.height();
	uint32_t const msb = uint32_t(1) << 31;
	
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (bw_mask_line[x >> 5] & (msb >> (x & 31))) {
				// B/W content.

				uint32_t tmp = bw_content_line[x >> 5];
				tmp >>= (31 - (x & 31));
				tmp &= uint32_t(1);
				// Now it's 0 for white and 1 for black.
				
				--tmp; // 0 becomes 0xffffffff and 1 becomes 0.
				
				tmp |= 0xff000000; // Force opacity.
				
				mixed_line[x] = static_cast<MixedPixel>(tmp);
			} else {
				// Non-B/W content.
				mixed_line[x] = reserveBlackAndWhite<MixedPixel>(mixed_line[x]);
			}
		}
		mixed_line += mixed_stride;
		bw_content_line += bw_content_stride;
		bw_mask_line += bw_mask_stride;
	}
}

} // anonymous namespace


OutputGenerator::OutputGenerator(
	std::shared_ptr<AbstractImageTransform const> const& image_transform,
	QRectF const& content_rect, QRectF const& outer_rect,
	ColorParams const& color_params,
	DespeckleLevel const despeckle_level)
:	m_ptrImageTransform(image_transform)
,	m_colorParams(color_params)
,	m_outRect(outer_rect.toRect())
,	m_contentRect(content_rect.toRect())
,	m_despeckleLevel(despeckle_level)
{
	// An empty outer_rect may be a result of all pages having no content box.
	if (m_outRect.width() <= 0) {
		m_outRect.setWidth(1);
	}
	if (m_outRect.height() <= 0) {
		m_outRect.setHeight(1);
	}

	// Make sure m_contentRect is fully inside m_outerRect.
	// Note that QRect::intersected() would be inappropriate here because of the way
	// it handles null rectangles.
	m_contentRect.setLeft(std::max(m_contentRect.left(), m_outRect.left()));
	m_contentRect.setTop(std::max(m_contentRect.top(), m_outRect.top()));
	m_contentRect.setRight(std::min(m_contentRect.right(), m_outRect.right()));
	m_contentRect.setBottom(std::min(m_contentRect.bottom(), m_outRect.bottom()));

	// Make m_contentRect be relative to m_outerRect.
	m_contentRect.translate(-m_outRect.topLeft());
}

std::function<QPointF(QPointF const&)>
OutputGenerator::origToOutputMapper() const
{
	QPointF const out_origin(m_outRect.topLeft());
	auto forward_mapper = m_ptrImageTransform->forwardMapper();

	return [forward_mapper, out_origin](QPointF const& pt) {
		return forward_mapper(pt) - out_origin;
	};
}

std::function<QPointF(QPointF const&)>
OutputGenerator::outputToOrigMapper() const
{
	QPointF const out_origin(m_outRect.topLeft());
	auto backward_mapper = m_ptrImageTransform->backwardMapper();

	return [backward_mapper, out_origin](QPointF const& pt) {
		return backward_mapper(pt + out_origin);
	};
}

QImage
OutputGenerator::process(
	TaskStatus const& status,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	QImage const& orig_image,
	CachingFactory<imageproc::GrayImage> const& gray_orig_image_factory,
	ZoneSet const& picture_zones, ZoneSet const& fill_zones,
	imageproc::BinaryImage* out_auto_picture_mask,
	imageproc::BinaryImage* out_speckles_image,
	DebugImages* const dbg) const
{
	assert(!orig_image.isNull());

	if (m_contentRect.isEmpty()) {
		// This page doesn't have a content box. Output a blank white image.

		QImage res(m_outRect.size(), QImage::Format_Mono);
		res.fill(1);

		if (out_auto_picture_mask) {
			if (out_auto_picture_mask->size() != m_outRect.size()) {
				BinaryImage(m_outRect.size()).swap(*out_auto_picture_mask);
			}
			out_auto_picture_mask->fill(BLACK); // Not a picture.
		}

		if (out_speckles_image) {
			if (out_speckles_image->size() != m_outRect.size()) {
				BinaryImage(m_outRect.size()).swap(*out_speckles_image);
			}
			out_speckles_image->fill(WHITE); // No speckles.
		}

		return res;
	}

	RenderParams const render_params(m_colorParams);

	uint8_t const dominant_gray = reserveBlackAndWhite<uint8_t>(
		calcDominantBackgroundGrayLevel(gray_orig_image_factory())
	);
	QColor const bg_color(dominant_gray, dominant_gray, dominant_gray);

	QImage transformed_image(
		m_ptrImageTransform->materialize(orig_image, m_outRect, bg_color, accel_ops)
	);
	if (transformed_image.hasAlphaChannel()) {
		// We don't handle ARGB32_Premultiplied below.
		transformed_image = transformed_image.convertToFormat(QImage::Format_ARGB32);
	}

	QPolygonF transformed_crop_area = m_ptrImageTransform->transformedCropArea();
	transformed_crop_area.translate(-m_outRect.topLeft());

	// The whole image minus the part cut off by the split line.
	QRect const big_margins_rect(
		transformed_crop_area.boundingRect().toRect() | m_contentRect
	);

	// For various reasons, we need some whitespace around the content
	// area.  This is the number of pixels of such whitespace.
	int const content_margin = std::min<int>(100, 20 * 2000 / (m_contentRect.width() + 1));

	// The content area (in output image coordinates) extended
	// with content_margin.  Note that we prevent that extension
	// from reaching the neighboring page.
	QRect const small_margins_rect(
		m_contentRect.adjusted(
			-content_margin, -content_margin,
			content_margin, content_margin
		).intersected(big_margins_rect)
	);

	// This is the area we are going to pass to estimateBackground().
	// estimateBackground() needs some margins around content, and
	// generally smaller margins are better, except when there is
	// some garbage that connects the content to the edge of the
	// image area.
	QRect const normalize_illumination_rect(
#if 1
		small_margins_rect
#else
		big_margins_rect
#endif
	);

	QImage maybe_normalized;

	if (!render_params.normalizeIllumination()) {
		maybe_normalized = transformed_image;
	} else {
		// For background estimation we need a downscaled image of about 200x300 pixels.
		// We can't just downscale transformed_image, as this background estimation we
		// need to set outside pixels to black.
		qreal const downscale_factor = 300.0 / std::max<int>(
			300, std::max(m_outRect.width(), m_outRect.height())
		);
		auto downscaled_transform = m_ptrImageTransform->clone();
		QTransform const downscale_only_transform(
			downscaled_transform->scale(downscale_factor, downscale_factor)
		);

		QRect const downscaled_out_rect(downscale_only_transform.mapRect(m_outRect));
		GrayImage const transformed_for_bg_estimation(
			downscaled_transform->materialize(
				gray_orig_image_factory(), downscaled_out_rect, Qt::black, accel_ops
			)
		);
		QPolygonF const downscaled_region_of_intereset(
			downscale_only_transform.map(
				transformed_crop_area.intersected(QRectF(normalize_illumination_rect))
			)
		);

		maybe_normalized = normalizeIlluminationGray(
			status, accel_ops, GrayImage(transformed_image),
			transformed_for_bg_estimation,
			downscaled_region_of_intereset, dbg
		);
	}

	status.throwIfCancelled();

	QImage maybe_smoothed;

	// We only do smoothing if we are going to do binarization later.
	if (!render_params.needBinarization()) {
		maybe_smoothed = maybe_normalized;
	} else {
		maybe_smoothed = smoothToGrayscale(maybe_normalized, accel_ops);
		if (dbg) {
			dbg->add(maybe_smoothed, "smoothed");
		}
	}

	status.throwIfCancelled();

	if (render_params.binaryOutput() || m_outRect.isEmpty()) {
		BinaryImage dst(m_outRect.size().expandedTo(QSize(1, 1)), WHITE);

		if (!m_contentRect.isEmpty()) {
			BinaryImage binarization_mask(dst.size(), BLACK);
			binarization_mask.fillExcept(m_contentRect, WHITE);

			dst = binarize(maybe_smoothed, binarization_mask);
			binarization_mask.release(); // Save memory.
			if (dbg) {
				dbg->add(dst, "binarized_and_cropped");
			}

			status.throwIfCancelled();

			morphologicalSmoothInPlace(dst, accel_ops);
			if (dbg) {
				dbg->add(dst, "edges_smoothed");
			}

			status.throwIfCancelled();

			// We want to keep despeckling the very last operation
			// affecting the binary part of the output. That's because
			// for "Deskpeckling" tab we will be reconstructing the input
			// to this despeckling operation from the final output file.
			// That's done to be able to "replay" the despeckling with
			// different parameters. Unfortunately we do have that
			// applyFillZonesInPlace() call below us, so the reconstruction
			// is not accurate. Fortunately, that reconstruction is for
			// visualization purposes only and that's the best we can do
			// without caching the full-size input-to-despeckling images.
			maybeDespeckleInPlace(
				dst, m_despeckleLevel, out_speckles_image, status, dbg
			);
		}

		applyFillZonesInPlace(dst, fill_zones);
		return dst.toQImage();
	}

	BinaryImage bw_mask;
	if (render_params.mixedOutput()) {
		// This block should go before the block with
		// adjustBrightnessGrayscale(), which may convert
		// maybe_normalized from grayscale to color mode.

		bw_mask = estimateBinarizationMask(status, GrayImage(maybe_normalized), dbg);
		if (dbg) {
			dbg->add(bw_mask, "bw_mask");
		}

		if (out_auto_picture_mask) {
			if (out_auto_picture_mask->size() != m_outRect.size()) {
				BinaryImage(m_outRect.size()).swap(*out_auto_picture_mask);
			}
			out_auto_picture_mask->fill(BLACK);

			if (!m_contentRect.isEmpty()) {
				rasterOp<RopSrc>(
					*out_auto_picture_mask, m_contentRect,
					bw_mask, m_contentRect.topLeft()
				);
			}
		}

		status.throwIfCancelled();

		std::function<QPointF(QPointF const&)> forward_mapper(m_ptrImageTransform->forwardMapper());
		QPointF const origin(m_outRect.topLeft());
		auto orig_to_output = [forward_mapper, origin](QPointF const& pt) {
			return forward_mapper(pt) - origin;
		};

		modifyBinarizationMask(bw_mask, picture_zones, orig_to_output);
		if (dbg) {
			dbg->add(bw_mask, "bw_mask with zones");
		}
	}

	if (render_params.normalizeIllumination() && !orig_image.allGray()) {
		assert(maybe_normalized.format() == QImage::Format_Indexed8);

		QImage tmp(transformed_image);
		adjustBrightnessGrayscale(tmp, maybe_normalized);

		status.throwIfCancelled();

		maybe_normalized = tmp;
		if (dbg) {
			dbg->add(maybe_normalized, "norm_illum_color");
		}
	}

	if (!render_params.mixedOutput()) {
		// It's "Color / Grayscale" mode, as we handle B/W above.
		reserveBlackAndWhite(maybe_normalized);
	} else {
		BinaryImage binarization_mask(bw_mask);
		binarization_mask.fillExcept(m_contentRect, WHITE);
		PolygonRasterizer::fillExcept(
			binarization_mask, WHITE, transformed_crop_area, Qt::WindingFill
		);

		BinaryImage bw_content(binarize(maybe_smoothed, binarization_mask));
		maybe_smoothed = QImage();   //
		binarization_mask.release(); // Save memory.
		if (dbg) {
			dbg->add(bw_content, "binarized_and_cropped");
		}

		status.throwIfCancelled();

		morphologicalSmoothInPlace(bw_content, accel_ops);
		if (dbg) {
			dbg->add(bw_content, "edges_smoothed");
		}

		status.throwIfCancelled();

		// We don't want speckles in non-B/W areas, as they would
		// then get visualized on the Despeckling tab.
		rasterOp<RopAnd<RopSrc, RopDst> >(bw_content, bw_mask);

		status.throwIfCancelled();

		// It's important to keep despeckling the very last operation
		// affecting the binary part of the output. That's because
		// we will be reconstructing the input to this despeckling
		// operation from the final output file.
		maybeDespeckleInPlace(
			bw_content, m_despeckleLevel, out_speckles_image, status, dbg
		);

		status.throwIfCancelled();

		if (maybe_normalized.format() == QImage::Format_Indexed8) {
			combineMixed<uint8_t>(
				maybe_normalized, bw_content, bw_mask
			);
		} else {
			assert(maybe_normalized.format() == QImage::Format_RGB32
				|| maybe_normalized.format() == QImage::Format_ARGB32);

			combineMixed<uint32_t>(
				maybe_normalized, bw_content, bw_mask
			);
		}
	}

	status.throwIfCancelled();

	QImage dst(maybe_normalized);

	if (render_params.whiteMargins()) {
		dst = QImage(m_outRect.size(), maybe_normalized.format());

		if (dst.format() == QImage::Format_Indexed8) {
			dst.setColorTable(createGrayscalePalette());
			// White.  0xff is reserved if in "Color / Grayscale" mode.
			uint8_t const color = render_params.mixedOutput() ? 0xff : 0xfe;
			dst.fill(color);
		} else {
			// White.  0x[ff]ffffff is reserved if in "Color / Grayscale" mode.
			uint32_t const color = render_params.mixedOutput() ? 0xffffffff : 0xfffefefe;
			dst.fill(color);
		}

		if (dst.isNull()) {
			// Both the constructor and setColorTable() above can leave the image null.
			throw std::bad_alloc();
		}

		if (!m_contentRect.isEmpty()) {
			drawOver(dst, m_contentRect, maybe_normalized, m_contentRect);
		}
	}

	maybe_normalized = QImage(); // Save memory and avoid a copy-on-write below.
	applyFillZonesInPlace(dst, fill_zones);

	return dst;
}

QSize
OutputGenerator::outputImageSize() const
{
	return m_outRect.size();
}

QRect
OutputGenerator::outputImageRect() const
{
	return m_outRect;
}

QRect
OutputGenerator::outputContentRect() const
{
	return m_contentRect;
}

/**
 * @brief Equalizes illumination in a grayscale image.
 *
 * @param status Used for task cancellation.
 * @pstsm accel_ops OpenCL-acceleratable operations.
 * @param input_for_normalization The image to normalize illumination in.
 * @param input_for_estimation There are two key differences between
 *        image_for_normalization and image_for_estimation:
 *        @li image_for_estimation has to be downscaled to about 200x300 px.
 *        @li When applying a transformation to produce image_for_estimation,
 *            pixels outside of the image have to be set to black.
 * @param estimation_region_of_intereset An option polygon in input_for_estimation
 *        image coordinates to limit the region of illumination estimation.
 * @param dbg Debug image sink.
 * @return The normalized version of input_for_normalization.
 */
GrayImage
OutputGenerator::normalizeIlluminationGray(
	TaskStatus const& status,
	std::shared_ptr<AcceleratableOperations> const& accel_ops,
	GrayImage const& input_for_normalisation,
	GrayImage const& input_for_estimation,
	boost::optional<QPolygonF> const& estimation_region_of_intereset,
	DebugImages* const dbg)
{
	PolynomialSurface const bg_ps(
		estimateBackground(
			input_for_estimation, estimation_region_of_intereset, accel_ops, status, dbg
		)
	);
	
	status.throwIfCancelled();
	
	GrayImage bg_img = accel_ops->renderPolynomialSurface(
		bg_ps, input_for_normalisation.width(), input_for_normalisation.height()
	);
	if (dbg) {
		dbg->add(bg_img, "background");
	}
	
	status.throwIfCancelled();
	
	// Divide input_for_normalisation by bg_img. Save result in bg_img.
	rasterOpGeneric(
		[](uint8_t orig, uint8_t& bg) {
			int const i_orig = static_cast<int>(orig);
			int const i_bg = static_cast<int>(bg);
			if (i_bg > i_orig) {
				bg = static_cast<uint8_t>((i_orig * 255 + (i_bg >> 1)) / i_bg);
			} else {
				bg = 0xff;
			}
		},
		input_for_normalisation, bg_img
	);
	if (dbg) {
		dbg->add(bg_img, "normalized_illumination");
	}
	
	return bg_img;
}

imageproc::BinaryImage
OutputGenerator::estimateBinarizationMask(
	TaskStatus const& status, GrayImage const& gray_source,
	DebugImages* const dbg) const
{
	QSize const downscaled_size(gray_source.size().scaled(1600, 1600, Qt::KeepAspectRatio));
	GrayImage downscaled(scaleToGray(gray_source, downscaled_size));
	
	status.throwIfCancelled();
	
	// Light areas indicate pictures.
	GrayImage picture_areas(detectPictures(status, downscaled, dbg));
	downscaled = GrayImage(); // Save memory.
	
	status.throwIfCancelled();
	
	BinaryThreshold const threshold(
		BinaryThreshold::mokjiThreshold(picture_areas, 5, 26)
	);
	
	// Scale back to original size.
	picture_areas = scaleToGray(picture_areas, gray_source.size());
	
	return BinaryImage(picture_areas, threshold);
}

void
OutputGenerator::modifyBinarizationMask(
	imageproc::BinaryImage& bw_mask, ZoneSet const& zones,
	std::function<QPointF(QPointF const&)> const& orig_to_output) const
{
	typedef PictureLayerProperty PLP;

	// Pass 1: ERASER1
	for (Zone const& zone : zones) {
		if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ERASER1) {
			QPolygonF const poly(zone.spline().transformed(orig_to_output).toPolygon());
			PolygonRasterizer::fill(bw_mask, BLACK, poly, Qt::WindingFill);
		}
	}

	// Pass 2: PAINTER2
	for (Zone const& zone : zones) {
		if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::PAINTER2) {
			QPolygonF const poly(zone.spline().transformed(orig_to_output).toPolygon());
			PolygonRasterizer::fill(bw_mask, WHITE, poly, Qt::WindingFill);
		}
	}

	// Pass 1: ERASER3
	for (Zone const& zone : zones) {
		if (zone.properties().locateOrDefault<PLP>()->layer() == PLP::ERASER3) {
			QPolygonF const poly(zone.spline().transformed(orig_to_output).toPolygon());
			PolygonRasterizer::fill(bw_mask, BLACK, poly, Qt::WindingFill);
		}
	}
}

QImage
OutputGenerator::convertToRGBorRGBA(QImage const& src)
{
	QImage::Format const fmt = src.hasAlphaChannel()
		? QImage::Format_ARGB32 : QImage::Format_RGB32;

	return src.convertToFormat(fmt);
}

GrayImage
OutputGenerator::detectPictures(
	TaskStatus const& status, GrayImage const& downscaled_input,
	DebugImages* const dbg)
{
	// downscaled_input is expected to be ~300 DPI.

	// We stretch the range of gray levels to cover the whole
	// range of [0, 255].  We do it because we want text
	// and background to be equally far from the center
	// of the whole range.  Otherwise text printed with a big
	// font will be considered a picture.
	GrayImage stretched(stretchGrayRange(downscaled_input, 0.01, 0.01));
	if (dbg) {
		dbg->add(stretched, "stretched");
	}
	
	status.throwIfCancelled();
	
	GrayImage eroded(erodeGray(stretched, QSize(3, 3), 0x00));
	if (dbg) {
		dbg->add(eroded, "eroded");
	}
	
	status.throwIfCancelled();
	
	GrayImage dilated(dilateGray(stretched, QSize(3, 3), 0xff));
	if (dbg) {
		dbg->add(dilated, "dilated");
	}
	
	stretched = GrayImage(); // Save memory.
	
	status.throwIfCancelled();
	
	// Invert "dilated", multiply by "eroded", invert the result.
	rasterOpGeneric(
		[](uint8_t eroded, uint8_t& dilated) {
			dilated = static_cast<uint8_t>(255u - (255u - dilated) * eroded / 255u);
		},
		eroded, dilated
	);
	GrayImage gray_gradient(dilated);
	dilated = GrayImage();
	eroded = GrayImage();
	if (dbg) {
		dbg->add(gray_gradient, "gray_gradient");
	}
	
	status.throwIfCancelled();
	
	GrayImage marker(erodeGray(gray_gradient, QSize(35, 35), 0x00));
	if (dbg) {
		dbg->add(marker, "marker");
	}
	
	status.throwIfCancelled();
	
	seedFillGrayInPlace(marker, gray_gradient, CONN8);
	GrayImage reconstructed(marker);
	marker = GrayImage();
	if (dbg) {
		dbg->add(reconstructed, "reconstructed");
	}
	
	status.throwIfCancelled();
	
	// Invert "reconstructed".
	rasterOpGeneric([](uint8_t& px) { px = uint8_t(0xff) - px; }, reconstructed);
	if (dbg) {
		dbg->add(reconstructed, "reconstructed_inverted");
	}
	
	status.throwIfCancelled();
	
	GrayImage holes_filled(createFramedImage(reconstructed.size()));
	seedFillGrayInPlace(holes_filled, reconstructed, CONN8);
	reconstructed = GrayImage();
	if (dbg) {
		dbg->add(holes_filled, "holes_filled");
	}
	
	return holes_filled;
}

GrayImage
OutputGenerator::smoothToGrayscale(
	QImage const& src, std::shared_ptr<AcceleratableOperations> const& accel_ops)
{
	int const min_dim = std::min(src.width(), src.height());
	int window;
	int degree;
	if (min_dim <= 1250) {
		window = 5;
		degree = 3;
	} else if (min_dim <= 2500) {
		window = 7;
		degree = 4;
	} else if (min_dim <= 5000) {
		window = 11;
		degree = 4;
	} else {
		window = 11;
		degree = 2;
	}
	return accel_ops->savGolFilter(GrayImage(src), QSize(window, window), degree, degree);
}

BinaryThreshold
OutputGenerator::adjustThreshold(BinaryThreshold threshold) const
{
	int const adjusted = threshold +
		m_colorParams.blackWhiteOptions().thresholdAdjustment();

	// Hard-bounding threshold values is necessary for example
	// if all the content went into the picture mask.
	return BinaryThreshold(qBound(30, adjusted, 225));
}

BinaryImage
OutputGenerator::binarize(QImage const& image, BinaryImage const& mask) const
{
	GrayscaleHistogram hist(image, mask);
	BinaryThreshold const bw_thresh(BinaryThreshold::otsuThreshold(hist));
	BinaryImage binarized(image, adjustThreshold(bw_thresh));
	
	// Fill masked out areas with white.
	rasterOp<RopAnd<RopSrc, RopDst> >(binarized, mask);
	
	return binarized;
}

/**
 * \brief Remove small connected components that are considered to be garbage.
 *
 * Both the size and the distance to other components are taken into account.
 *
 * \param[in,out] image The image to despeckle.
 * \param level Despeckling aggressiveness.
 * \param[out] out_speckles_img See the identically named parameter
 *             of process() for more info.
 * \param status Task status.
 * \param dbg An optional sink for debugging images.
 *
 * \note This function only works effectively when the DPI is symmetric,
 * that is, its horizontal and vertical components are equal.
 */
void
OutputGenerator::maybeDespeckleInPlace(
	imageproc::BinaryImage& image, DespeckleLevel const level,
	BinaryImage* out_speckles_img, TaskStatus const& status, DebugImages* dbg) const
{
	if (out_speckles_img) {
		*out_speckles_img = image;
	}

	if (level != DESPECKLE_OFF) {
		Despeckle::Level lvl = Despeckle::NORMAL;
		switch (level) {
			case DESPECKLE_CAUTIOUS:
				lvl = Despeckle::CAUTIOUS;
				break;
			case DESPECKLE_NORMAL:
				lvl = Despeckle::NORMAL;
				break;
			case DESPECKLE_AGGRESSIVE:
				lvl = Despeckle::AGGRESSIVE;
				break;
			default:;
		}

		Despeckle::despeckleInPlace(image, lvl, status, dbg);

		if (dbg) {
			dbg->add(image, "despeckled");
		}
	}

	if (out_speckles_img) {
		rasterOp<RopSubtract<RopDst, RopSrc>>(*out_speckles_img, image);
	}
}

void
OutputGenerator::morphologicalSmoothInPlace(
	BinaryImage& bin_img, std::shared_ptr<AcceleratableOperations> const& accel_ops)
{
	std::vector<Grid<char>> patterns;

	// When removing black noise, remove small ones first.
	
	{
		char const pattern[] =
			"XXX"
			" - "
			"   ";
		generatePatternsForAllDirections(patterns, pattern, 3, 3);
	}
	
	{
		char const pattern[] =
			"X ?"
			"X  "
			"X- "
			"X- "
			"X  "
			"X ?";
		generatePatternsForAllDirections(patterns, pattern, 3, 6);
	}
	
	{
		char const pattern[] =
			"X ?"
			"X ?"
			"X  "
			"X- "
			"X- "
			"X- "
			"X  "
			"X ?"
			"X ?";
		generatePatternsForAllDirections(patterns, pattern, 3, 9);
	}
	
	{
		char const pattern[] =
			"XX?"
			"XX?"
			"XX "
			"X+ "
			"X+ "
			"X+ "
			"XX "
			"XX?"
			"XX?";
		generatePatternsForAllDirections(patterns, pattern, 3, 9);
	}
	
	{
		char const pattern[] =
			"XX?"
			"XX "
			"X+ "
			"X+ "
			"XX "
			"XX?";
		generatePatternsForAllDirections(patterns, pattern, 3, 6);
	}
	
	{
		char const pattern[] =
			"   "
			"X+X"
			"XXX";
		generatePatternsForAllDirections(patterns, pattern, 3, 3);
	}

	accel_ops->hitMissReplaceInPlace(bin_img, WHITE, patterns);
}

void
OutputGenerator::generatePatternsForAllDirections(
	std::vector<Grid<char>>& sink, char const* const pattern,
	int const pattern_width, int const pattern_height)
{
	// Rotations are clockwise.
	Grid<char> pattern0(pattern_width, pattern_height);
	Grid<char> pattern90(pattern_height, pattern_width);
	Grid<char> pattern180(pattern_width, pattern_height);
	Grid<char> pattern270(pattern_height, pattern_width);

	// pattern -> pattern0
	memcpy(pattern0.data(), pattern, pattern_width*pattern_height);

	// pattern0 -> pattern90
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x) {
			int const new_x = pattern_height - 1 - y;
			int const new_y = x;
			pattern90(new_x, new_y) = pattern0(x, y);
		}
	}
	
	// pattern0 -> pattern180
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x) {
			int const new_x = pattern_width - 1 - x;
			int const new_y = pattern_height - 1 - y;
			pattern180(new_x, new_y) = pattern0(x, y);
		}
	}
	
	// pattern0 -> pattern270
	for (int y = 0; y < pattern_height; ++y) {
		for (int x = 0; x < pattern_width; ++x) {
			int const new_x = y;
			int const new_y = pattern_width - 1 - x;
			pattern270(new_x, new_y) = pattern0(x, y);
		}
	}

	sink.push_back(std::move(pattern0));
	sink.push_back(std::move(pattern90));
	sink.push_back(std::move(pattern180));
	sink.push_back(std::move(pattern270));
}

unsigned char
OutputGenerator::calcDominantBackgroundGrayLevel(QImage const& img)
{
	// TODO: make a color version.
	// In ColorPickupInteraction.cpp we have code for median color finding.
	// We can use that.

	QImage const gray(toGrayscale(img));
	
	BinaryImage mask(binarizeOtsu(gray));
	mask.invert();
	
	GrayscaleHistogram const hist(gray, mask);
	
	int integral_hist[256];
	integral_hist[0] = hist[0];
	for (int i = 1; i < 256; ++i) {
		integral_hist[i] = hist[i] + integral_hist[i - 1];
	}
	
	int const num_colors = 256;
	int const window_size = 10;
	
	int best_pos = 0;
	int best_sum = integral_hist[window_size - 1];
	for (int i = 1; i <= num_colors - window_size; ++i) {
		int const sum = integral_hist[i + window_size - 1] - integral_hist[i - 1];
		if (sum > best_sum) {
			best_sum = sum;
			best_pos = i;
		}
	}
	
	int half_sum = 0;
	for (int i = best_pos; i < best_pos + window_size; ++i) {
		half_sum += hist[i];
		if (half_sum >= best_sum / 2) {
			return i;
		}
	}
	
	assert(!"Unreachable");
	return 0;
}

void
OutputGenerator::applyFillZonesInPlace(
	QImage& img, ZoneSet const& zones,
	std::function<QPointF(QPointF const&)> const& orig_to_output) const
{
	if (zones.empty()) {
		return;
	}

	QImage canvas(img.convertToFormat(QImage::Format_ARGB32_Premultiplied));
	
	{
		QPainter painter(&canvas);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setPen(Qt::NoPen);

		for (Zone const& zone : zones) {
			QColor const color(zone.properties().locateOrDefault<FillColorProperty>()->color());
			QPolygonF const poly(zone.spline().transformed(orig_to_output).toPolygon());
			painter.setBrush(color);
			painter.drawPolygon(poly, Qt::WindingFill);
		}
	}

	if (img.format() == QImage::Format_Indexed8 && img.isGrayscale()) {
		img = toGrayscale(canvas);
	} else {
		img = canvas.convertToFormat(img.format());
	}
}

/**
 * A simplified version of the above, not requiring coordinate mapping function.
 */
void
OutputGenerator::applyFillZonesInPlace(QImage& img, ZoneSet const& zones) const
{
	applyFillZonesInPlace(img, zones, origToOutputMapper());
}

void
OutputGenerator::applyFillZonesInPlace(
	imageproc::BinaryImage& img, ZoneSet const& zones,
	std::function<QPointF(QPointF const&)> const& orig_to_output) const
{
	if (zones.empty()) {
		return;
	}

	for (Zone const& zone : zones) {
		QColor const color(zone.properties().locateOrDefault<FillColorProperty>()->color());
		BWColor const bw_color = qGray(color.rgb()) < 128 ? BLACK : WHITE;
		QPolygonF const poly(zone.spline().transformed(orig_to_output).toPolygon());
		PolygonRasterizer::fill(img, bw_color, poly, Qt::WindingFill);
	}
}

/**
 * A simplified version of the above, not requiring coordinate mapping function.
 */
void
OutputGenerator::applyFillZonesInPlace(
	imageproc::BinaryImage& img, ZoneSet const& zones) const
{
	applyFillZonesInPlace(img, zones, origToOutputMapper());
}

} // namespace output
