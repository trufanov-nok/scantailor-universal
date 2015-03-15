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

#include "Binarize.h"
#include "BinaryImage.h"
#include "BinaryThreshold.h"
#include "Grayscale.h"
#include "GrayImage.h"
#include "IntegralImage.h"
#include "WienerFilter.h"
#include "RasterOpGeneric.h"
#include <QImage>
#include <QRect>
#include <QDebug>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

namespace imageproc
{

BinaryImage binarizeOtsu(QImage const& src)
{
	return BinaryImage(src, BinaryThreshold::otsuThreshold(src));
}

BinaryImage binarizeMokji(
	QImage const& src, unsigned const max_edge_width,
	unsigned const min_edge_magnitude)
{
	BinaryThreshold const threshold(
		BinaryThreshold::mokjiThreshold(
			src, max_edge_width, min_edge_magnitude
		)
	);
	return BinaryImage(src, threshold);
}

BinaryImage binarizeNiblack(GrayImage const& src, QSize const window_size)
{
	if (window_size.isEmpty()) {
		throw std::invalid_argument("binarizeNiblack: invalid window_size");
	}

	if (src.isNull()) {
		return BinaryImage();
	}

	int const w = src.width();
	int const h = src.height();

	IntegralImage<uint32_t> integral_image(w, h);
	IntegralImage<uint64_t> integral_sqimage(w, h);

	uint8_t const* src_line = src.data();
	int const src_stride = src.stride();

	for (int y = 0; y < h; ++y, src_line += src_stride) {
		integral_image.beginRow();
		integral_sqimage.beginRow();
		for (int x = 0; x < w; ++x) {
			uint32_t const pixel = src_line[x];
			integral_image.push(pixel);
			integral_sqimage.push(pixel * pixel);
		}
	}

	int const window_lower_half = window_size.height() >> 1;
	int const window_upper_half = window_size.height() - window_lower_half;
	int const window_left_half = window_size.width() >> 1;
	int const window_right_half = window_size.width() - window_left_half;

	BinaryImage bw_img(w, h);
	uint32_t* bw_line = bw_img.data();
	int const bw_stride = bw_img.wordsPerLine();

	src_line = src.data();
	for (int y = 0; y < h; ++y) {
		int const top = std::max(0, y - window_lower_half);
		int const bottom = std::min(h, y + window_upper_half); // exclusive

		for (int x = 0; x < w; ++x) {
			int const left = std::max(0, x - window_left_half);
			int const right = std::min(w, x + window_right_half); // exclusive
			int const area = (bottom - top) * (right - left);
			assert(area > 0); // because window_size > 0 and w > 0 and h > 0

			QRect const rect(left, top, right - left, bottom - top);
			double const window_sum = integral_image.sum(rect);
			double const window_sqsum = integral_sqimage.sum(rect);

			double const r_area = 1.0 / area;
			double const mean = window_sum * r_area;
			double const sqmean = window_sqsum * r_area;

			double const variance = sqmean - mean * mean;
			double const stddev = sqrt(fabs(variance));

			double const k = -0.2;
			double const threshold = mean + k * stddev;

			static uint32_t const msb = uint32_t(1) << 31;
			uint32_t const mask = msb >> (x & 31);
			if (int(src_line[x]) < threshold) {
				// black
				bw_line[x >> 5] |= mask;
			} else {
				// white
				bw_line[x >> 5] &= ~mask;
			}
		}

		src_line += src_stride;
		bw_line += bw_stride;
	}

	return bw_img;
}

BinaryImage binarizeGatos(
	GrayImage const& src, QSize const window_size, double const noise_sigma)
{
	if (window_size.isEmpty()) {
		throw std::invalid_argument("binarizeGatos: invalid window_size");
	}

	if (src.isNull()) {
		return BinaryImage();
	}

	int const w = src.width();
	int const h = src.height();

	GrayImage wiener(wienerFilter(src, QSize(5, 5), noise_sigma));
	BinaryImage niblack(binarizeNiblack(wiener, window_size));
	IntegralImage<uint32_t> niblack_bg_ii(w, h);
	IntegralImage<uint32_t> wiener_bg_ii(w, h);

	uint32_t const* niblack_line = niblack.data();
	int const niblack_stride = niblack.wordsPerLine();
	uint8_t const* wiener_line = wiener.data();
	int const wiener_stride = wiener.stride();

	for (int y = 0; y < h; ++y) {
		niblack_bg_ii.beginRow();
		wiener_bg_ii.beginRow();
		for (int x = 0; x < w; ++x) {
			// bg: 1, fg: 0
			uint32_t const niblack_inverted_pixel =
				(~niblack_line[x >> 5] >> (31 - (x & 31))) & uint32_t(1);
			uint32_t const wiener_pixel = wiener_line[x];
			niblack_bg_ii.push(niblack_inverted_pixel);

			// bg: wiener_pixel, fg: 0
			wiener_bg_ii.push(wiener_pixel & ~(niblack_inverted_pixel - uint32_t(1)));
		}

		wiener_line += wiener_stride;
		niblack_line += niblack_stride;
	}

	std::vector<QRect> windows;
	for (int scale = 1;; ++scale) {
		windows.emplace_back(0, 0, window_size.width() * scale, window_size.height() * scale);
		if (windows.back().width() > w*2 && windows.back().height() > h * 2) {
			// Such a window is enough to cover the whole image when centered
			// at any of its corners.
			break;
		}
	}

	// sum(background - original) for foreground pixels according to Niblack.
	uint32_t sum_diff = 0;

	// sum(background) pixels for background pixels according to Niblack.
	uint32_t sum_bg = 0;

	QRect const image_rect(src.rect());
	GrayImage background(wiener);
	uint8_t* background_line = background.data();
	int const background_stride = background.stride();
	niblack_line = niblack.data();
	for (int y = 0; y < h; ++y) {
		for (int x = 0; x < w; ++x) {
			for (QRect window : windows) {
				window.moveCenter(QPoint(x, y));
				window &= image_rect;
				uint32_t const niblack_sum_bg = niblack_bg_ii.sum(window);
				if (niblack_sum_bg == 0) {
					// No background pixels in this window. Try a larger one.
					continue;
				}

				static const uint32_t msb = uint32_t(1) << 31;
				if (niblack_line[x >> 5] & (msb >> (x & 31))) {
					// Foreground pixel. Interpolate from background pixels in window.
					uint32_t const wiener_sum_bg = wiener_bg_ii.sum(window);
					uint32_t const bg = (wiener_sum_bg + (niblack_sum_bg >> 1)) / niblack_sum_bg;
					sum_diff += bg - background_line[x];
					background_line[x] = bg;
				} else {
					sum_bg += background_line[x];
				}

				break;
			}
		}

		background_line += background_stride;
		niblack_line += niblack_stride;
	}

	double const delta = double(sum_diff) / (w*h - niblack_bg_ii.sum(image_rect));
	double const b = double(sum_bg) / niblack_bg_ii.sum(image_rect);

	double const q = 0.6;
	double const p1 = 0.5;
	double const p2 = 0.8;

	double const exp_scale = -4.0 / (b * (1.0 - p1));
	double const exp_bias = 2.0 * (1.0 + p1) / (1.0 - p1);
	double const threshold_scale = q * delta * (1.0 - p2);
	double const threshold_bias = q * delta * p2;

	rasterOpGeneric(
		wiener.data(), wiener.stride(), wiener.size(),
		background.data(), background.stride(),
		[exp_scale, exp_bias, threshold_scale, threshold_bias]
		(uint8_t& wiener, uint8_t const bg) {
			double const threshold = threshold_scale /
				(1.0 + exp(double(bg) * exp_scale + exp_bias)) + threshold_bias;
			wiener = double(bg) - double(wiener) > threshold ? 0x00 : 0xff;
		}
	);

	return BinaryImage(wiener);
}

BinaryImage binarizeSauvola(QImage const& src, QSize const window_size)
{
	if (window_size.isEmpty()) {
		throw std::invalid_argument("binarizeSauvola: invalid window_size");
	}
	
	if (src.isNull()) {
		return BinaryImage();
	}
	
	QImage const gray(toGrayscale(src));
	int const w = gray.width();
	int const h = gray.height();
	
	IntegralImage<uint32_t> integral_image(w, h);
	IntegralImage<uint64_t> integral_sqimage(w, h);
	
	uint8_t const* gray_line = gray.bits();
	int const gray_bpl = gray.bytesPerLine();
	
	for (int y = 0; y < h; ++y, gray_line += gray_bpl) {
		integral_image.beginRow();
		integral_sqimage.beginRow();
		for (int x = 0; x < w; ++x) {
			uint32_t const pixel = gray_line[x];
			integral_image.push(pixel);
			integral_sqimage.push(pixel * pixel);
		}
	}
	
	int const window_lower_half = window_size.height() >> 1;
	int const window_upper_half = window_size.height() - window_lower_half;
	int const window_left_half = window_size.width() >> 1;
	int const window_right_half = window_size.width() - window_left_half;
	
	BinaryImage bw_img(w, h);
	uint32_t* bw_line = bw_img.data();
	int const bw_wpl = bw_img.wordsPerLine();
	
	gray_line = gray.bits();
	for (int y = 0; y < h; ++y) {
		int const top = std::max(0, y - window_lower_half);
		int const bottom = std::min(h, y + window_upper_half); // exclusive
		
		for (int x = 0; x < w; ++x) {
			int const left = std::max(0, x - window_left_half);
			int const right = std::min(w, x + window_right_half); // exclusive
			int const area = (bottom - top) * (right - left);
			assert(area > 0); // because window_size > 0 and w > 0 and h > 0
			
			QRect const rect(left, top, right - left, bottom - top);
			double const window_sum = integral_image.sum(rect);
			double const window_sqsum = integral_sqimage.sum(rect);
			
			double const r_area = 1.0 / area;
			double const mean = window_sum * r_area;
			double const sqmean = window_sqsum * r_area;
			
			double const variance = sqmean - mean * mean;
			double const deviation = sqrt(fabs(variance));
			
			double const k = 0.34;
			double const threshold = mean * (1.0 + k * (deviation / 128.0 - 1.0));
			
			uint32_t const msb = uint32_t(1) << 31;
			uint32_t const mask = msb >> (x & 31);
			if (int(gray_line[x]) < threshold) {
				// black
				bw_line[x >> 5] |= mask;
			} else {
				// white
				bw_line[x >> 5] &= ~mask;
			}
		}
		
		gray_line += gray_bpl;
		bw_line += bw_wpl;
	}
	
	return bw_img;
}

BinaryImage binarizeWolf(
	QImage const& src, QSize const window_size,
	unsigned char const lower_bound, unsigned char const upper_bound)
{
	if (window_size.isEmpty()) {
		throw std::invalid_argument("binarizeWolf: invalid window_size");
	}
	
	if (src.isNull()) {
		return BinaryImage();
	}
	
	QImage const gray(toGrayscale(src));
	int const w = gray.width();
	int const h = gray.height();
	
	IntegralImage<uint32_t> integral_image(w, h);
	IntegralImage<uint64_t> integral_sqimage(w, h);
	
	uint8_t const* gray_line = gray.bits();
	int const gray_bpl = gray.bytesPerLine();
	
	uint32_t min_gray_level = 255;
	
	for (int y = 0; y < h; ++y, gray_line += gray_bpl) {
		integral_image.beginRow();
		integral_sqimage.beginRow();
		for (int x = 0; x < w; ++x) {
			uint32_t const pixel = gray_line[x];
			integral_image.push(pixel);
			integral_sqimage.push(pixel * pixel);
			min_gray_level = std::min(min_gray_level, pixel);
		}
	}
	
	int const window_lower_half = window_size.height() >> 1;
	int const window_upper_half = window_size.height() - window_lower_half;
	int const window_left_half = window_size.width() >> 1;
	int const window_right_half = window_size.width() - window_left_half;
	
	std::vector<float> means(w * h, 0);
	std::vector<float> deviations(w * h, 0);
	
	double max_deviation = 0;
	
	for (int y = 0; y < h; ++y) {
		int const top = std::max(0, y - window_lower_half);
		int const bottom = std::min(h, y + window_upper_half); // exclusive
		
		for (int x = 0; x < w; ++x) {
			int const left = std::max(0, x - window_left_half);
			int const right = std::min(w, x + window_right_half); // exclusive
			int const area = (bottom - top) * (right - left);
			assert(area > 0); // because window_size > 0 and w > 0 and h > 0
			
			QRect const rect(left, top, right - left, bottom - top);
			double const window_sum = integral_image.sum(rect);
			double const window_sqsum = integral_sqimage.sum(rect);
			
			double const r_area = 1.0 / area;
			double const mean = window_sum * r_area;
			double const sqmean = window_sqsum * r_area;
			
			double const variance = sqmean - mean * mean;
			double const deviation = sqrt(fabs(variance));
			max_deviation = std::max(max_deviation, deviation);
			means[w * y + x] = mean;
			deviations[w * y + x] = deviation;
		}
	}
	
	// TODO: integral images can be disposed at this point.
	
	BinaryImage bw_img(w, h);
	uint32_t* bw_line = bw_img.data();
	int const bw_wpl = bw_img.wordsPerLine();
	
	gray_line = gray.bits();
	for (int y = 0; y < h; ++y, gray_line += gray_bpl, bw_line += bw_wpl) {
		for (int x = 0; x < w; ++x) {
			float const mean = means[y * w + x];
			float const deviation = deviations[y * w + x];
			double const k = 0.3;
			double const a = 1.0 - deviation / max_deviation;
			double const threshold = mean - k * a * (mean - min_gray_level);
			
			uint32_t const msb = uint32_t(1) << 31;
			uint32_t const mask = msb >> (x & 31);
			if (gray_line[x] < lower_bound ||
					(gray_line[x] <= upper_bound &&
					int(gray_line[x]) < threshold)) {
				// black
				bw_line[x >> 5] |= mask;
			} else {
				// white
				bw_line[x >> 5] &= ~mask;
			}
		}
	}
	
	return bw_img;
}

} // namespace imageproc
