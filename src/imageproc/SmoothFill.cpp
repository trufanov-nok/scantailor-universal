/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

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

#include "SmoothFill.h"
#include <QColor>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QRect>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace imageproc
{

namespace
{

// With a large width, the ring has more samples, but it also takes pixels that are far from the zone edge.
int const RING_WIDTH = 3;

// The window must be wider than a text stroke, so that the median ignores the stroke.
int const MEDIAN_RADIUS = 8;

// A ring pixel that differs more than this from its local median is a part of an object, not of the background.
int const OUTLIER_LUMINANCE = 32;

int const RELAX_ITERATIONS = 8;

// The fill is smooth, so the solver uses a coarser grid for a large zone, because the solver uses about 60 bytes per pixel.
int const MAX_SOLVER_PIXELS = 1 << 20;

// Strong texture is not paper grain, and noise at that level makes the fill look dirty.
float const MAX_GRAIN_SIGMA = 24.0f;

struct Level
{
    int width = 0;
    int height = 0;
    std::vector<float> color;
    std::vector<float> weight;

    void resize(int w, int h)
    {
        width = w;
        height = h;
        color.assign(size_t(w) * h * 3, 0.0f);
        weight.assign(size_t(w) * h, 0.0f);
    }
};

// A hash of the position gives noise that does not change from run to run.
float gaussianNoise(int x, int y)
{
    uint32_t h = uint32_t(x) * 0x8da6b343u ^ uint32_t(y) * 0xd8163841u;
    float sum = 0.0f;
    for (int i = 0; i < 4; ++i) {
        h ^= h >> 15;
        h *= 0x2c1b3c6du;
        h ^= h >> 12;
        h *= 0x297a2d39u;
        h ^= h >> 15;
        sum += float(h & 0xffff) / 65535.0f - 0.5f;
    }
    // The sum of four uniform values has the variance 1/3.
    return sum * 1.7320508f;
}

void dilate(std::vector<uint8_t>& mask, int width, int height, int radius)
{
    std::vector<uint8_t> tmp(mask.size(), 0);
    std::vector<int> prefix(std::max(width, height) + 1);

    for (int y = 0; y < height; ++y) {
        uint8_t const* line = &mask[size_t(y) * width];
        for (int x = 0; x < width; ++x) {
            prefix[x + 1] = prefix[x] + (line[x] ? 1 : 0);
        }
        for (int x = 0; x < width; ++x) {
            int const left = std::max(0, x - radius);
            int const right = std::min(width, x + radius + 1);
            tmp[size_t(y) * width + x] = prefix[right] - prefix[left] > 0;
        }
    }

    for (int x = 0; x < width; ++x) {
        for (int y = 0; y < height; ++y) {
            prefix[y + 1] = prefix[y] + (tmp[size_t(y) * width + x] ? 1 : 0);
        }
        for (int y = 0; y < height; ++y) {
            int const top = std::max(0, y - radius);
            int const bottom = std::min(height, y + radius + 1);
            mask[size_t(y) * width + x] = prefix[bottom] - prefix[top] > 0;
        }
    }
}

Level pullLevel(Level const& src)
{
    Level dst;
    dst.resize((src.width + 1) / 2, (src.height + 1) / 2);
    for (int y = 0; y < dst.height; ++y) {
        for (int x = 0; x < dst.width; ++x) {
            float sum_w = 0.0f;
            float sum_c[3] = { 0.0f, 0.0f, 0.0f };
            for (int dy = 0; dy < 2; ++dy) {
                int const sy = y * 2 + dy;
                if (sy >= src.height) {
                    continue;
                }
                for (int dx = 0; dx < 2; ++dx) {
                    int const sx = x * 2 + dx;
                    if (sx >= src.width) {
                        continue;
                    }
                    size_t const si = size_t(sy) * src.width + sx;
                    float const w = src.weight[si];
                    sum_w += w;
                    for (int c = 0; c < 3; ++c) {
                        sum_c[c] += w * src.color[si * 3 + c];
                    }
                }
            }
            size_t const di = size_t(y) * dst.width + x;
            dst.weight[di] = std::min(1.0f, sum_w);
            if (sum_w > 0.0f) {
                for (int c = 0; c < 3; ++c) {
                    dst.color[di * 3 + c] = sum_c[c] / sum_w;
                }
            }
        }
    }
    return dst;
}

void sampleBilinear(Level const& lvl, float fx, float fy, float* out)
{
    fx = std::min(std::max(fx, 0.0f), float(lvl.width - 1));
    fy = std::min(std::max(fy, 0.0f), float(lvl.height - 1));
    int const x0 = int(fx);
    int const y0 = int(fy);
    int const x1 = std::min(x0 + 1, lvl.width - 1);
    int const y1 = std::min(y0 + 1, lvl.height - 1);
    float const ax = fx - x0;
    float const ay = fy - y0;
    for (int c = 0; c < 3; ++c) {
        float const top = lvl.color[(size_t(y0) * lvl.width + x0) * 3 + c] * (1 - ax)
                          + lvl.color[(size_t(y0) * lvl.width + x1) * 3 + c] * ax;
        float const bottom = lvl.color[(size_t(y1) * lvl.width + x0) * 3 + c] * (1 - ax)
                             + lvl.color[(size_t(y1) * lvl.width + x1) * 3 + c] * ax;
        out[c] = top * (1 - ay) + bottom * ay;
    }
}

// Gauss-Seidel steps on the free pixels move the push result to the smooth membrane surface.
void relax(Level& lvl)
{
    for (int it = 0; it < RELAX_ITERATIONS; ++it) {
        for (int y = 0; y < lvl.height; ++y) {
            for (int x = 0; x < lvl.width; ++x) {
                size_t const i = size_t(y) * lvl.width + x;
                if (lvl.weight[i] > 0.0f) {
                    continue;
                }
                float sum[3] = { 0.0f, 0.0f, 0.0f };
                int count = 0;
                int const nx[4] = { x - 1, x + 1, x, x };
                int const ny[4] = { y, y, y - 1, y + 1 };
                for (int n = 0; n < 4; ++n) {
                    if (nx[n] < 0 || ny[n] < 0 || nx[n] >= lvl.width || ny[n] >= lvl.height) {
                        continue;
                    }
                    size_t const ni = size_t(ny[n]) * lvl.width + nx[n];
                    for (int c = 0; c < 3; ++c) {
                        sum[c] += lvl.color[ni * 3 + c];
                    }
                    ++count;
                }
                if (count > 0) {
                    for (int c = 0; c < 3; ++c) {
                        lvl.color[i * 3 + c] = sum[c] / count;
                    }
                }
            }
        }
    }
}

void solve(std::vector<Level>& levels)
{
    while (levels.back().width > 1 || levels.back().height > 1) {
        Level next(pullLevel(levels.back()));
        levels.push_back(std::move(next));
    }

    for (int k = int(levels.size()) - 2; k >= 0; --k) {
        Level& lvl = levels[k];
        Level const& up = levels[k + 1];
        float parent[3];
        for (int y = 0; y < lvl.height; ++y) {
            for (int x = 0; x < lvl.width; ++x) {
                size_t const i = size_t(y) * lvl.width + x;
                float const w = lvl.weight[i];
                if (w >= 1.0f) {
                    continue;
                }
                sampleBilinear(up, x * 0.5f - 0.25f, y * 0.5f - 0.25f, parent);
                for (int c = 0; c < 3; ++c) {
                    lvl.color[i * 3 + c] = w * lvl.color[i * 3 + c] + (1.0f - w) * parent[c];
                }
            }
        }
        relax(lvl);
    }
}

// Directional estimates tell how to continue the straight edges, which the membrane surface makes blurred.
struct DirectionalSums
{
    std::vector<float> color;
    std::vector<float> weight;
};

// Smaller values make a direction with similar ends win more strongly against the other directions.
float const END_DIFF_SIGMA = 10.0f;

int const DIRECTIONAL_BLUR_RADIUS = 6;

// The membrane surface gets the weight of a direction with ends that differ by this much.
float const MEMBRANE_END_DIFF = 120.0f;

void addDirection(
    int dx, int dy, int width, int height, std::vector<uint8_t> const& accepted,
    std::vector<uint8_t> const& inside, std::vector<float> const& end_color, DirectionalSums& sums)
{
    std::vector<size_t> line;
    std::vector<int> prev;
    for (int sy = 0; sy < height; ++sy) {
        for (int sx = 0; sx < width; ++sx) {
            int const px = sx - dx;
            int const py = sy - dy;
            if (px >= 0 && py >= 0 && px < width && py < height) {
                continue;
            }
            line.clear();
            for (int x = sx, y = sy; x >= 0 && y >= 0 && x < width && y < height; x += dx, y += dy) {
                line.push_back(size_t(y) * width + x);
            }
            int const n = int(line.size());
            prev.assign(n, -1);
            int last = -1;
            for (int k = 0; k < n; ++k) {
                if (accepted[line[k]]) {
                    last = k;
                }
                prev[k] = last;
            }
            int next = -1;
            for (int k = n - 1; k >= 0; --k) {
                if (accepted[line[k]]) {
                    next = k;
                    continue;
                }
                if (!inside[line[k]] || prev[k] < 0 || next < 0) {
                    continue;
                }
                size_t const a = line[prev[k]];
                size_t const b = line[next];
                float const t = float(k - prev[k]) / float(next - prev[k]);
                float diff2 = 0.0f;
                for (int c = 0; c < 3; ++c) {
                    float const d = end_color[a * 3 + c] - end_color[b * 3 + c];
                    diff2 += d * d;
                }
                float const w = 1.0f / (END_DIFF_SIGMA * END_DIFF_SIGMA + diff2);
                size_t const i = line[k];
                for (int c = 0; c < 3; ++c) {
                    float const v = end_color[a * 3 + c] * (1.0f - t) + end_color[b * 3 + c] * t;
                    sums.color[i * 3 + c] += w * v;
                }
                sums.weight[i] += w;
            }
        }
    }
}

// A box blur of the weighted sums removes the stripes that come from small differences between the ends of next lines.
void blurSums(DirectionalSums& sums, int width, int height, int radius)
{
    int const channels = 4;
    std::vector<float> data(size_t(width) * height * channels);
    for (size_t i = 0; i < size_t(width) * height; ++i) {
        for (int c = 0; c < 3; ++c) {
            data[i * channels + c] = sums.color[i * 3 + c];
        }
        data[i * channels + 3] = sums.weight[i];
    }
    std::vector<float> tmp(data.size());
    std::vector<double> prefix(size_t(std::max(width, height) + 1) * channels);
    for (int pass = 0; pass < 2; ++pass) {
        int const len = pass == 0 ? width : height;
        int const lines = pass == 0 ? height : width;
        for (int l = 0; l < lines; ++l) {
            for (int k = 0; k < len; ++k) {
                size_t const i = pass == 0 ? size_t(l) * width + k : size_t(k) * width + l;
                for (int c = 0; c < channels; ++c) {
                    prefix[size_t(k + 1) * channels + c] = prefix[size_t(k) * channels + c] + data[i * channels + c];
                }
            }
            for (int k = 0; k < len; ++k) {
                int const lo = std::max(0, k - radius);
                int const hi = std::min(len, k + radius + 1);
                size_t const i = pass == 0 ? size_t(l) * width + k : size_t(k) * width + l;
                for (int c = 0; c < channels; ++c) {
                    tmp[i * channels + c] = float(prefix[size_t(hi) * channels + c] - prefix[size_t(lo) * channels + c]);
                }
            }
        }
        data.swap(tmp);
    }
    for (size_t i = 0; i < size_t(width) * height; ++i) {
        for (int c = 0; c < 3; ++c) {
            sums.color[i * 3 + c] = data[i * channels + c];
        }
        sums.weight[i] = data[i * channels + 3];
    }
}

uint8_t clampToByte(float v)
{
    return uint8_t(std::min(255.0f, std::max(0.0f, v + 0.5f)));
}

} // namespace

QImage smoothFillPatch(QImage const& image, QPainterPath const& path, QPoint* origin, QPainterPath const& exclude)
{
    QRect const rect(
        path.boundingRect().toAlignedRect()
        .adjusted(-RING_WIDTH, -RING_WIDTH, RING_WIDTH, RING_WIDTH)
        .intersected(image.rect())
    );
    if (rect.isEmpty()) {
        return QImage();
    }
    *origin = rect.topLeft();

    int const width = rect.width();
    int const height = rect.height();
    size_t const num_pixels = size_t(width) * height;

    QImage coverage(width, height, QImage::Format_Alpha8);
    coverage.fill(0);
    {
        QPainter painter(&coverage);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::black);
        painter.translate(-rect.topLeft());
        painter.drawPath(path);
    }

    bool const opaque32 = image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32
                          || image.format() == QImage::Format_ARGB32_Premultiplied;
    // A view into a 32-bit image does not copy the pixels, which keeps the memory usage of a large zone lower.
    QImage const src(
        opaque32
        ? QImage(image.constScanLine(rect.y()) + rect.x() * 4, width, height, image.bytesPerLine(), image.format())
        : image.copy(rect).convertToFormat(QImage::Format_ARGB32)
    );
    bool const bilevel = image.format() == QImage::Format_Mono || image.format() == QImage::Format_MonoLSB;

    std::vector<uint8_t> inside(num_pixels);
    for (int y = 0; y < height; ++y) {
        uint8_t const* line = coverage.constScanLine(y);
        for (int x = 0; x < width; ++x) {
            inside[size_t(y) * width + x] = line[x] != 0;
        }
    }

    std::vector<uint8_t> ring(inside);
    dilate(ring, width, height, RING_WIDTH);
    for (size_t i = 0; i < num_pixels; ++i) {
        ring[i] = ring[i] && !inside[i];
    }
    if (!exclude.isEmpty()) {
        QImage excluded(width, height, QImage::Format_Alpha8);
        excluded.fill(0);
        {
            QPainter painter(&excluded);
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::black);
            painter.translate(-rect.topLeft());
            painter.drawPath(exclude);
        }
        for (int y = 0; y < height; ++y) {
            uint8_t const* line = excluded.constScanLine(y);
            for (int x = 0; x < width; ++x) {
                if (line[x] != 0) {
                    ring[size_t(y) * width + x] = 0;
                }
            }
        }
    }

    std::vector<uint8_t> luminance(num_pixels, 0);
    size_t ring_count = 0;
    for (int y = 0; y < height; ++y) {
        QRgb const* line = reinterpret_cast<QRgb const*>(src.constScanLine(y));
        for (int x = 0; x < width; ++x) {
            size_t const i = size_t(y) * width + x;
            if (ring[i]) {
                luminance[i] = qGray(line[x]);
                ++ring_count;
            }
        }
    }

    // A local median keeps a background gradient, but it rejects strokes that touch the zone edge.
    std::vector<uint8_t> accepted(num_pixels, 0);
    size_t accepted_count = 0;
    std::vector<int> window;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            size_t const i = size_t(y) * width + x;
            if (!ring[i]) {
                continue;
            }
            window.clear();
            for (int wy = std::max(0, y - MEDIAN_RADIUS); wy <= std::min(height - 1, y + MEDIAN_RADIUS); ++wy) {
                for (int wx = std::max(0, x - MEDIAN_RADIUS); wx <= std::min(width - 1, x + MEDIAN_RADIUS); ++wx) {
                    size_t const wi = size_t(wy) * width + wx;
                    if (ring[wi]) {
                        window.push_back(luminance[wi]);
                    }
                }
            }
            std::nth_element(window.begin(), window.begin() + window.size() / 2, window.end());
            if (std::abs(int(luminance[i]) - window[window.size() / 2]) <= OUTLIER_LUMINANCE) {
                accepted[i] = 1;
                ++accepted_count;
            }
        }
    }
    // When the check rejects most of the ring, the zone edge is inside a picture, and all of the ring is the best guess.
    if (accepted_count * 4 < ring_count) {
        accepted = ring;
        accepted_count = ring_count;
    }

    float grain_sigma = 0.0f;
    if (!bilevel) {
        std::vector<int> diffs;
        for (int y = 0; y + 1 < height; ++y) {
            for (int x = 0; x + 1 < width; ++x) {
                size_t const i = size_t(y) * width + x;
                if (!accepted[i]) {
                    continue;
                }
                if (accepted[i + 1]) {
                    diffs.push_back(std::abs(int(luminance[i]) - int(luminance[i + 1])));
                }
                if (accepted[i + width]) {
                    diffs.push_back(std::abs(int(luminance[i]) - int(luminance[i + width])));
                }
            }
        }
        if (diffs.size() >= 16) {
            std::nth_element(diffs.begin(), diffs.begin() + diffs.size() / 2, diffs.end());
            // The MAD of a difference of two pixels gives the noise of one pixel after division by sqrt(2).
            grain_sigma = std::min(MAX_GRAIN_SIGMA, 1.4826f * diffs[diffs.size() / 2] / 1.4142136f);
        }
    }

    int scale = 1;
    while (num_pixels / (size_t(scale) * scale) > size_t(MAX_SOLVER_PIXELS)) {
        ++scale;
    }

    std::vector<Level> levels(1);
    Level& base = levels[0];
    base.resize((width + scale - 1) / scale, (height + scale - 1) / scale);
    for (int y = 0; y < height; ++y) {
        QRgb const* line = reinterpret_cast<QRgb const*>(src.constScanLine(y));
        for (int x = 0; x < width; ++x) {
            if (!accepted[size_t(y) * width + x]) {
                continue;
            }
            size_t const bi = size_t(y / scale) * base.width + x / scale;
            base.weight[bi] += 1.0f;
            base.color[bi * 3 + 0] += qRed(line[x]);
            base.color[bi * 3 + 1] += qGreen(line[x]);
            base.color[bi * 3 + 2] += qBlue(line[x]);
        }
    }
    for (size_t i = 0; i < base.weight.size(); ++i) {
        float const w = base.weight[i];
        if (w > 0.0f) {
            for (int c = 0; c < 3; ++c) {
                base.color[i * 3 + c] /= w;
            }
            base.weight[i] = std::min(1.0f, w);
        }
    }

    if (accepted_count == 0) {
        // Without a ring there is nothing to continue, so the fill is white as for a new zone before.
        std::fill(base.color.begin(), base.color.end(), 255.0f);
    } else {
        solve(levels);
    }
    Level const& result = levels[0];

    DirectionalSums directional;
    if (scale == 1 && accepted_count > 0) {
        // An average of the nearby accepted pixels keeps the grain out of the end colors and repairs the rejected ring pixels.
        std::vector<float> end_color(num_pixels * 3, 0.0f);
        std::vector<uint8_t> is_end(num_pixels, 0);
        int const radii[3] = { 2, 5, 10 };
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                size_t const i = size_t(y) * width + x;
                if (!ring[i]) {
                    continue;
                }
                for (int radius : radii) {
                    if (!accepted[i] && radius == radii[0]) {
                        continue;
                    }
                    float sum[3] = { 0.0f, 0.0f, 0.0f };
                    int count = 0;
                    for (int wy = std::max(0, y - radius); wy <= std::min(height - 1, y + radius); ++wy) {
                        QRgb const* line = reinterpret_cast<QRgb const*>(src.constScanLine(wy));
                        for (int wx = std::max(0, x - radius); wx <= std::min(width - 1, x + radius); ++wx) {
                            if (accepted[size_t(wy) * width + wx]) {
                                sum[0] += qRed(line[wx]);
                                sum[1] += qGreen(line[wx]);
                                sum[2] += qBlue(line[wx]);
                                ++count;
                            }
                        }
                    }
                    if (count > 0) {
                        for (int c = 0; c < 3; ++c) {
                            end_color[i * 3 + c] = sum[c] / count;
                        }
                        is_end[i] = 1;
                        break;
                    }
                }
            }
        }
        directional.color.assign(num_pixels * 3, 0.0f);
        directional.weight.assign(num_pixels, 0.0f);
        addDirection(1, 0, width, height, is_end, inside, end_color, directional);
        addDirection(0, 1, width, height, is_end, inside, end_color, directional);
        addDirection(1, 1, width, height, is_end, inside, end_color, directional);
        addDirection(1, -1, width, height, is_end, inside, end_color, directional);
        blurSums(directional, width, height, DIRECTIONAL_BLUR_RADIUS);
    }

    QImage patch(width, height, QImage::Format_ARGB32_Premultiplied);
    patch.fill(Qt::transparent);
    float color[3];
    for (int y = 0; y < height; ++y) {
        uint8_t const* cov_line = coverage.constScanLine(y);
        QRgb* out_line = reinterpret_cast<QRgb*>(patch.scanLine(y));
        for (int x = 0; x < width; ++x) {
            int const alpha = cov_line[x];
            if (alpha == 0) {
                continue;
            }
            if (scale == 1) {
                size_t const i = size_t(y) * width + x;
                for (int c = 0; c < 3; ++c) {
                    color[c] = result.color[i * 3 + c];
                }
                if (!directional.weight.empty() && directional.weight[i] > 0.0f) {
                    // The blur adds the weights of a whole window, so the membrane weight grows by the window area.
                    float const window = float((2 * DIRECTIONAL_BLUR_RADIUS + 1) * (2 * DIRECTIONAL_BLUR_RADIUS + 1));
                    float const membrane_w = window / (
                        END_DIFF_SIGMA * END_DIFF_SIGMA + MEMBRANE_END_DIFF * MEMBRANE_END_DIFF
                    );
                    float const total_w = directional.weight[i] + membrane_w;
                    for (int c = 0; c < 3; ++c) {
                        color[c] = (directional.color[i * 3 + c] + membrane_w * color[c]) / total_w;
                    }
                }
            } else {
                sampleBilinear(result, (x + 0.5f) / scale - 0.5f, (y + 0.5f) / scale - 0.5f, color);
            }
            uint8_t r, g, b;
            if (bilevel) {
                uint8_t const v = qGray(clampToByte(color[0]), clampToByte(color[1]), clampToByte(color[2])) < 128 ? 0 : 255;
                r = g = b = v;
            } else {
                float const noise = grain_sigma > 0.0f ? grain_sigma * gaussianNoise(rect.x() + x, rect.y() + y) : 0.0f;
                r = clampToByte(color[0] + noise);
                g = clampToByte(color[1] + noise);
                b = clampToByte(color[2] + noise);
            }
            out_line[x] = qPremultiply(qRgba(r, g, b, alpha));
        }
    }
    return patch;
}

} // namespace imageproc
