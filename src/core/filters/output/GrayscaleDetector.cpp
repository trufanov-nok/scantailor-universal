/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2026  ScanTailor Universal contributors

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


#include "GrayscaleDetector.h"
#include <QImage>
#include <QRectF>
#include <algorithm>
#include <cmath>
#include <vector>

namespace output
{

namespace
{

// Pixels darker or lighter than these limits are ink or paper, and they do not show if a picture has color.
int const MIN_LUMA = 30;
int const MAX_LUMA = 200;

// The correction removes a paper tint up to this chroma; a stronger uniform color, for example of a cover, stays in the result.
double const MAX_TINT = 10.0;

// The sample grid has at most this many points along the long side, because a finer grid does not change the result.
int const MAX_SAMPLES_PER_SIDE = 1000;

std::size_t const MIN_PIXELS = 100;

} // anonymous namespace

double
GrayscaleDetector::measureChroma(QImage const& image, QRectF const& area)
{
    QRect const rect(area.toAlignedRect().intersected(image.rect()));
    if (rect.isEmpty()) {
        return -1.0;
    }

    switch (image.format()) {
    case QImage::Format_Mono:
    case QImage::Format_MonoLSB:
    case QImage::Format_Grayscale8:
        return 0.0;
    case QImage::Format_Indexed8:
        if (image.isGrayscale()) {
            return 0.0;
        }
        break;
    default:
        break;
    }

    QImage const rgb(
        image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32
        ? image : image.convertToFormat(QImage::Format_RGB32)
    );

    int const step = std::max(1, std::max(rect.width(), rect.height()) / MAX_SAMPLES_PER_SIDE);

    std::vector<float> cb_values;
    std::vector<float> cr_values;
    double cb_sum = 0.0;
    double cr_sum = 0.0;
    for (int y = rect.top(); y <= rect.bottom(); y += step) {
        QRgb const* line = reinterpret_cast<QRgb const*>(rgb.constScanLine(y));
        for (int x = rect.left(); x <= rect.right(); x += step) {
            double const r = qRed(line[x]);
            double const g = qGreen(line[x]);
            double const b = qBlue(line[x]);
            double const luma = 0.299 * r + 0.587 * g + 0.114 * b;
            if (luma <= MIN_LUMA || luma >= MAX_LUMA) {
                continue;
            }
            double const cb = -0.168736 * r - 0.331264 * g + 0.5 * b;
            double const cr = 0.5 * r - 0.418688 * g - 0.081312 * b;
            cb_values.push_back(static_cast<float>(cb));
            cr_values.push_back(static_cast<float>(cr));
            cb_sum += cb;
            cr_sum += cr;
        }
    }

    std::size_t const count = cb_values.size();
    if (count < MIN_PIXELS) {
        return -1.0;
    }

    double cb_tint = cb_sum / count;
    double cr_tint = cr_sum / count;
    double const tint = std::sqrt(cb_tint * cb_tint + cr_tint * cr_tint);
    if (tint > MAX_TINT) {
        cb_tint *= MAX_TINT / tint;
        cr_tint *= MAX_TINT / tint;
    }

    std::vector<float> distances(count);
    for (std::size_t i = 0; i < count; ++i) {
        double const dcb = cb_values[i] - cb_tint;
        double const dcr = cr_values[i] - cr_tint;
        distances[i] = static_cast<float>(std::sqrt(dcb * dcb + dcr * dcr));
    }

    std::size_t const index = (count - 1) * 95 / 100;
    std::nth_element(distances.begin(), distances.begin() + index, distances.end());
    return distances[index];
}

} // namespace output
