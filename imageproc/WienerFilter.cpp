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

#include "WienerFilter.h"
#include "GrayImage.h"
#include "IntegralImage.h"
#include <QSize>
#include <QtGlobal>
#include <algorithm>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace imageproc
{

GrayImage wienerFilter(
    GrayImage const& image, QSize const& window_size, double const noise_sigma)
{
    GrayImage dst(image);
    wienerFilterInPlace(dst, window_size, noise_sigma);
    return dst;
}

void wienerFilterInPlace(
    GrayImage& image, QSize const& window_size, double const noise_sigma)
{
    if (window_size.isEmpty()) {
        throw std::invalid_argument("wienerFilter: empty window_size");
    }
    if (noise_sigma < 0) {
        throw std::invalid_argument("wienerFilter: negative noise_sigma");
    }
    if (image.isNull()) {
        return;
    }

    int const w = image.width();
    int const h = image.height();
    double const noise_variance = noise_sigma * noise_sigma;

    IntegralImage<uint32_t> integral_image(w, h);
    IntegralImage<uint64_t> integral_sqimage(w, h);

    uint8_t* image_line = image.data();
    int const image_stride = image.stride();

    for (int y = 0; y < h; ++y, image_line += image_stride) {
        integral_image.beginRow();
        integral_sqimage.beginRow();
        for (int x = 0; x < w; ++x) {
            uint32_t const pixel = image_line[x];
            integral_image.push(pixel);
            integral_sqimage.push(pixel * pixel);
        }
    }

    int const window_lower_half = window_size.height() >> 1;
    int const window_upper_half = window_size.height() - window_lower_half;
    int const window_left_half = window_size.width() >> 1;
    int const window_right_half = window_size.width() - window_left_half;

    image_line = image.data();
    for (int y = 0; y < h; ++y, image_line += image_stride) {
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

            if (variance > 1e-6) {
                double const src_pixel = static_cast<double>(image_line[x]);
                double const dst_pixel = mean + (src_pixel - mean) *
                    std::max<double>(0.0, variance - noise_variance) / variance;
                image_line[x] = static_cast<uint8_t>(std::lround(dst_pixel));
            }
        }
    }
}

} // namespace imageproc
