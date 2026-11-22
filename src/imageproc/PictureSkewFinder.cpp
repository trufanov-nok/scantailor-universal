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

#include "PictureSkewFinder.h"
#include "GrayImage.h"
#include "BinaryImage.h"
#include "Constants.h"
#include <QSize>
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <math.h>

namespace imageproc
{

double const PictureSkew::MIN_PICTURE_FRACTION = 0.15;

double const PictureSkew::GOOD_CONFIDENCE = 1.5;

int const PictureSkewFinder::TARGET_DPI = 150;

double const PictureSkewFinder::DEFAULT_MAX_ANGLE = 10.0;

namespace
{

struct Point {
    float x;
    float y;
    float weight;
    Point(float x_, float y_, float weight_) : x(x_), y(y_), weight(weight_) {}
};

bool isInPage(BinaryImage const& mask, int x, int y)
{
    uint32_t const* line = mask.data() + mask.wordsPerLine() * y;
    return (line[x >> 5] >> (31 - (x & 31))) & 1;
}

int percentile(std::vector<int> const& hist, double fraction)
{
    long long total = 0;
    for (int count : hist) {
        total += count;
    }
    long long const limit = (long long)(fraction * total);
    long long sum = 0;
    for (int level = 0; level < 256; ++level) {
        sum += hist[level];
        if (sum > limit) {
            return level;
        }
    }
    return 255;
}

double sumOfSquaredSteps(std::vector<double> const& profile)
{
    double sum = 0.0;
    for (size_t i = 1; i < profile.size(); ++i) {
        double const step = profile[i] - profile[i - 1];
        sum += step * step;
    }
    return sum;
}

// When the angle is correct, each straight edge of the picture makes a sharp step in the projections.
double profileScore(std::vector<Point> const& points, double angle_deg,
                    std::vector<double>& columns, std::vector<double>& rows)
{
    double const rad = angle_deg * constants::DEG2RAD;
    double const cos_a = cos(rad);
    double const sin_a = sin(rad);
    std::fill(columns.begin(), columns.end(), 0.0);
    std::fill(rows.begin(), rows.end(), 0.0);
    double const offset = columns.size() / 2;
    for (Point const& p : points) {
        double const x = p.x * cos_a - p.y * sin_a;
        double const y = p.x * sin_a + p.y * cos_a;
        columns[int(x + offset)] += p.weight;
        rows[int(y + offset)] += p.weight;
    }
    return sumOfSquaredSteps(columns) + sumOfSquaredSteps(rows);
}

} // anonymous namespace

PictureSkew
PictureSkewFinder::findSkew(GrayImage const& image, BinaryImage const& page_mask, double max_angle)
{
    int const width = image.width();
    int const height = image.height();
    if (width < 2 || height < 2 || page_mask.size() != image.size()) {
        return PictureSkew();
    }

    std::vector<int> hist(256, 0);
    for (int y = 0; y < height; ++y) {
        uint8_t const* line = image.data() + image.stride() * y;
        for (int x = 0; x < width; ++x) {
            if (isInPage(page_mask, x, y)) {
                ++hist[line[x]];
            }
        }
    }
    double const paper = percentile(hist, 0.95);
    double const paper90 = percentile(hist, 0.90);
    if (paper <= 0) {
        return PictureSkew();
    }

    // A block of about 3 mm is larger than a text character, so a block of text seldom has many middle tones.
    int const block = std::max(4, (TARGET_DPI * 3 + 12) / 25);
    int page_blocks = 0;
    int picture_blocks = 0;
    for (int by = 0; by + block <= height; by += block) {
        for (int bx = 0; bx + block <= width; bx += block) {
            if (!isInPage(page_mask, bx + block / 2, by + block / 2)) {
                continue;
            }
            ++page_blocks;
            int sum = 0;
            int mid_tones = 0;
            for (int y = by; y < by + block; ++y) {
                uint8_t const* line = image.data() + image.stride() * y;
                for (int x = bx; x < bx + block; ++x) {
                    int const level = line[x];
                    sum += level;
                    if (level > paper90 * 0.25 && level < paper90 * 0.80) {
                        ++mid_tones;
                    }
                }
            }
            int const area = block * block;
            // Text is mostly paper or ink, but a photo or a halftone is full of middle tones.
            if (mid_tones * 2 > area && sum < paper90 * 0.85 * area) {
                ++picture_blocks;
            }
        }
    }
    double const picture_fraction = page_blocks ? double(picture_blocks) / page_blocks : 0.0;
    // A page with too few picture blocks cannot give a reliable result, so the angle search is not necessary.
    if (picture_fraction < PictureSkew::MIN_PICTURE_FRACTION) {
        return PictureSkew(0.0, picture_fraction, 0.0);
    }

    // The page mask is convex, so its extent in each row and column gives the distance to the page edge.
    std::vector<int> row_first(height, width), row_last(height, -1);
    std::vector<int> col_first(width, height), col_last(width, -1);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (isInPage(page_mask, x, y)) {
                row_first[y] = std::min(row_first[y], x);
                row_last[y] = std::max(row_last[y], x);
                col_first[x] = std::min(col_first[x], y);
                col_last[x] = std::max(col_last[x], y);
            }
        }
    }

    // A picture cut by the page edge has a false straight edge there, so the weight falls to zero at the page edge.
    float const taper = std::max(1.0f, 0.04f * std::min(width, height));
    std::vector<Point> points;
    float const cx = 0.5f * width;
    float const cy = 0.5f * height;
    for (int y = 0; y < height; ++y) {
        uint8_t const* line = image.data() + image.stride() * y;
        for (int x = 0; x < width; ++x) {
            if (line[x] < paper * 0.8 && isInPage(page_mask, x, y)) {
                int const dx = std::min(x - row_first[y], row_last[y] - x);
                int const dy = std::min(y - col_first[x], col_last[x] - y);
                float const edge = float(std::min(dx, dy));
                points.push_back(Point(x + 0.5f - cx, y + 0.5f - cy, std::min(1.0f, edge / taper)));
            }
        }
    }
    if (points.empty()) {
        return PictureSkew(0.0, picture_fraction, 0.0);
    }

    int const bins = int(ceil(sqrt(double(width) * width + double(height) * height))) + 4;
    std::vector<double> columns(bins);
    std::vector<double> rows(bins);

    double const coarse_step = 0.25;
    int const steps = int(max_angle / coarse_step);
    std::vector<double> scores;
    double best_score = -1.0;
    double best_angle = 0.0;
    for (int i = -steps; i <= steps; ++i) {
        double const angle = i * coarse_step;
        double const score = profileScore(points, angle, columns, rows);
        scores.push_back(score);
        if (score > best_score) {
            best_score = score;
            best_angle = angle;
        }
    }

    double const fine_step = 0.05;
    double const coarse_best = best_angle;
    for (int i = -4; i <= 4; ++i) {
        double const angle = coarse_best + i * fine_step;
        if (i == 0 || fabs(angle) > max_angle) {
            continue;
        }
        double const score = profileScore(points, angle, columns, rows);
        if (score > best_score) {
            best_score = score;
            best_angle = angle;
        }
    }

    std::nth_element(scores.begin(), scores.begin() + scores.size() / 2, scores.end());
    double const median = scores[scores.size() / 2];
    double const confidence = median > 0.0 ? best_score / median : 0.0;

    return PictureSkew(best_angle, picture_fraction, confidence);
}

} // namespace imageproc
