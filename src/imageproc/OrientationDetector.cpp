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

#include "OrientationDetector.h"
#include "BinaryImage.h"
#include "BinaryThreshold.h"
#include "ConnComp.h"
#include "ConnCompEraser.h"
#include "Connectivity.h"
#include "GrayImage.h"
#include "Scale.h"
#include <QImage>
#include <QImageIOHandler>
#include <QImageReader>
#include <QSize>
#include <QString>
#include <utility>
#include <map>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <vector>

namespace imageproc
{

namespace
{

struct Box {
    int x0;
    int y0;
    int x1;
    int y1;

    int width() const
    {
        return x1 - x0;
    }

    int height() const
    {
        return y1 - y0;
    }

    int maxSide() const
    {
        return std::max(width(), height());
    }
};

// Separate counts per cue let one cue that votes against the others stop a decision.
struct Cue {
    int up;
    int down;

    Cue() : up(0), down(0) {}

    int total() const
    {
        return up + down;
    }

    double z() const
    {
        return double(up - down) / std::sqrt(double(up + down) + 1.0);
    }
};

struct FrameStats {
    int pairs;
    Cue ascender;
    Cue stacked;
    Cue punctuation;

    FrameStats() : pairs(0) {}

    Cue sum() const
    {
        Cue c;
        c.up = ascender.up + stacked.up + punctuation.up;
        c.down = ascender.down + stacked.down + punctuation.down;
        return c;
    }
};

// Buckets of boxes in a coarse grid, so that a neighbour search looks only at near boxes.
class BoxGrid
{
public:
    BoxGrid(std::vector<Box> const& boxes, int width, int height, int cell)
        : m_boxes(boxes), m_cell(std::max(1, cell))
    {
        m_cols = width / m_cell + 1;
        m_rows = height / m_cell + 1;
        m_cells.resize(size_t(m_cols) * size_t(m_rows));
        for (size_t i = 0; i < boxes.size(); ++i) {
            Box const& b = boxes[i];
            int const cx = std::min(m_cols - 1, std::max(0, b.x0 / m_cell));
            int const cy = std::min(m_rows - 1, std::max(0, b.y0 / m_cell));
            m_cells[size_t(cy) * size_t(m_cols) + size_t(cx)].push_back(int(i));
        }
    }

    // The grid keeps a box in the cell of its top left corner, thus the search area grows by one box size.
    template<typename F>
    void forEachNear(int x0, int y0, int x1, int y1, int grow, F f) const
    {
        int const cx0 = std::max(0, (x0 - grow) / m_cell);
        int const cy0 = std::max(0, (y0 - grow) / m_cell);
        int const cx1 = std::min(m_cols - 1, std::max(0, x1 / m_cell));
        int const cy1 = std::min(m_rows - 1, std::max(0, y1 / m_cell));
        for (int cy = cy0; cy <= cy1; ++cy) {
            for (int cx = cx0; cx <= cx1; ++cx) {
                std::vector<int> const& cell = m_cells[size_t(cy) * size_t(m_cols) + size_t(cx)];
                for (size_t k = 0; k < cell.size(); ++k) {
                    f(m_boxes[size_t(cell[k])]);
                }
            }
        }
    }
private:
    std::vector<Box> const& m_boxes;
    int m_cell;
    int m_cols;
    int m_rows;
    std::vector<std::vector<int> > m_cells;
};

// A turn of the boxes is faster than a turn of the image, and the vertical text axis needs only the boxes.
Box rotateBoxCw(Box const& b, int height)
{
    Box r;
    r.x0 = height - b.y1;
    r.y0 = b.x0;
    r.x1 = height - b.y0;
    r.y1 = b.x1;
    return r;
}

// Collects the evidence on the assumption that the text lines in this frame are horizontal.
FrameStats analyzeFrame(
    std::vector<Box> const& large, std::vector<Box> const& marks,
    int width, int height, double size)
{
    // Heavy print joins letters into wide boxes, and these boxes still carry the dots and accents.
    std::vector<Box> chars;
    std::vector<Box> bodies;
    for (size_t i = 0; i < large.size(); ++i) {
        Box const& b = large[i];
        if (b.height() < 0.5 * size && b.width() < 0.5 * size) {
            continue;
        }
        if (b.height() <= 2.5 * size && b.width() <= 2.5 * size) {
            chars.push_back(b);
        }
        if (b.height() <= 2.5 * size) {
            bodies.push_back(b);
        }
    }

    FrameStats stats;
    int const cell = std::max(4, int(size * 2.0));
    BoxGrid const grid(chars, width, height, cell);
    BoxGrid const body_grid(bodies, width, height, cell);
    int const grow = int(size * 3.0);
    double const tol = std::max(1.0, 0.1 * size);
    double const big = std::max(2.0, 0.22 * size);
    double const max_gap = 0.5 * size;
    double const min_gap = -0.15 * size;

    for (size_t i = 0; i < chars.size(); ++i) {
        Box const& a = chars[i];
        Box const* best = 0;
        int best_gap = 0;
        grid.forEachNear(
            a.x1 + int(min_gap) - 1, a.y0, a.x1 + int(max_gap) + 1, a.y1, grow,
        [&](Box const & b) {
            if (b.x0 <= a.x0 || b.x0 + b.x1 <= a.x0 + a.x1) {
                return;
            }
            int const gap = b.x0 - a.x1;
            if (gap < min_gap || gap > max_gap) {
                return;
            }
            int const overlap = std::min(a.y1, b.y1) - std::max(a.y0, b.y0);
            if (overlap * 2 < std::min(a.height(), b.height())) {
                return;
            }
            if (!best || gap < best_gap) {
                best = &b;
                best_gap = gap;
            }
        }
        );
        if (!best) {
            continue;
        }
        ++stats.pairs;
        int const d_bottom = std::abs(a.y1 - best->y1);
        int const d_top = std::abs(a.y0 - best->y0);
        // Latin text has more ascenders than descenders, so the bottoms of neighbour letters line up more often.
        if (d_bottom <= tol && d_top >= big) {
            ++stats.ascender.up;
        } else if (d_top <= tol && d_bottom >= big) {
            ++stats.ascender.down;
        }
    }

    int const body_grow = int(size * 15.0);
    double const stack_gap = 0.5 * size;
    double const side_gap = 0.4 * size;
    for (size_t i = 0; i < marks.size(); ++i) {
        Box const& m = marks[i];
        int const mcx2 = m.x0 + m.x1;
        int const mcy2 = m.y0 + m.y1;
        double best_above = stack_gap + 1.0;
        double best_below = stack_gap + 1.0;
        Box const* side = 0;
        bool inside = false;
        double best_side = side_gap + 1.0;
        // The dot of an i can sit a little off the narrow stem, thus the stem gets a small margin.
        int const stem_margin = int(0.15 * size);
        body_grid.forEachNear(
            m.x0 - int(side_gap) - 1, m.y0 - int(stack_gap) - 1,
            m.x1 + int(side_gap) + 1, m.y1 + int(stack_gap) + 1, body_grow,
        [&](Box const & c) {
            if (mcx2 >= 2 * (c.x0 - stem_margin) && mcx2 <= 2 * (c.x1 + stem_margin)) {
                if (m.y1 <= c.y0 + 1) {
                    best_above = std::min(best_above, double(c.y0 - m.y1));
                } else if (m.y0 >= c.y1 - 1) {
                    best_below = std::min(best_below, double(m.y0 - c.y1));
                } else {
                    inside = true;
                }
                return;
            }
            if (mcy2 < 2 * c.y0 - c.height() || mcy2 > 2 * c.y1 + c.height()) {
                return;
            }
            double const gap = m.x0 >= c.x1 ? m.x0 - c.x1 : c.x0 - m.x1;
            if (gap >= -0.1 * size && gap < best_side) {
                best_side = gap;
                side = &c;
            }
        }
        );
        // Dots and accents sit above the letters, and only ogoneks sit below them.
        if (best_above <= stack_gap || best_below <= stack_gap) {
            if (best_above < best_below) {
                ++stats.stacked.up;
            } else if (best_below < best_above) {
                ++stats.stacked.down;
            }
            continue;
        }
        // A mark inside the box of joined letters is a dot or an accent, not punctuation.
        if (!side || inside) {
            continue;
        }
        // Full stops and commas sit at the baseline of the letter next to them.
        double const rel = (0.5 * mcy2 - side->y0) / std::max(1, side->height());
        if (rel > 0.75) {
            ++stats.punctuation.up;
        } else if (rel < 0.25) {
            ++stats.punctuation.down;
        }
    }
    return stats;
}

QString cueText(Cue const& c)
{
    return QString::number(c.up) + "/" + QString::number(c.down);
}

int toDpi(int dpm)
{
    return int(std::lround(dpm * 0.0254));
}

} // namespace

double
OrientationDetector::confidenceThreshold()
{
    return 0.5;
}

OrientationResult
OrientationDetector::detect(QImage const& image)
{
    if (image.isNull()) {
        return OrientationResult(0, 0.0, false, QStringLiteral("no image"));
    }

    // A fixed scale near 300 DPI keeps the letters large enough to see ascenders and descenders.
    int const dpi = std::max(toDpi(image.dotsPerMeterX()), toDpi(image.dotsPerMeterY()));
    int const max_side = std::max(image.width(), image.height());
    int factor = 1;
    if (dpi >= 150 && dpi <= 2400) {
        factor = std::max(1, (dpi + 150) / 300);
    } else {
        factor = std::max(1, (max_side + 2999) / 3000);
    }
    while (max_side / factor > 4000) {
        ++factor;
    }

    GrayImage gray(image);
    if (factor > 1) {
        gray = scaleToGray(gray, QSize(std::max(1, image.width() / factor), std::max(1, image.height() / factor)));
    }
    int const width = gray.width();
    int const height = gray.height();

    BinaryImage bin(gray, BinaryThreshold::otsuThreshold(gray));
    gray = GrayImage();

    std::vector<Box> all;
    {
        ConnCompEraser eraser(bin.release(), CONN8);
        int const max_box = std::max(8, max_side / factor / 12);
        for (;;) {
            ConnComp const cc(eraser.nextConnComp());
            if (cc.isNull()) {
                break;
            }
            QRect const r(cc.rect());
            if (r.width() > max_box || r.height() > max_box || cc.pixCount() < 2) {
                continue;
            }
            Box b;
            b.x0 = r.left();
            b.y0 = r.top();
            b.x1 = r.right() + 1;
            b.y1 = r.bottom() + 1;
            all.push_back(b);
        }
    }

    // The median size of the letter boxes sets every distance threshold below.
    std::vector<int> sizes;
    sizes.reserve(all.size());
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i].maxSide() >= 6) {
            sizes.push_back(all[i].maxSide());
        }
    }
    if (sizes.size() < 50) {
        return OrientationResult(0, 0.0, false, QStringLiteral("no text"));
    }
    std::nth_element(sizes.begin(), sizes.begin() + sizes.size() / 2, sizes.end());
    double const size = sizes[sizes.size() / 2];

    std::vector<Box> large;
    std::vector<Box> marks;
    for (size_t i = 0; i < all.size(); ++i) {
        int const s = all[i].maxSide();
        if (s >= 0.5 * size && s <= 15.0 * size) {
            large.push_back(all[i]);
        } else if (s >= 0.12 * size && s <= 0.45 * size && s >= 2) {
            marks.push_back(all[i]);
        }
    }

    std::vector<Box> large_cw;
    std::vector<Box> marks_cw;
    large_cw.reserve(large.size());
    marks_cw.reserve(marks.size());
    for (size_t i = 0; i < large.size(); ++i) {
        large_cw.push_back(rotateBoxCw(large[i], height));
    }
    for (size_t i = 0; i < marks.size(); ++i) {
        marks_cw.push_back(rotateBoxCw(marks[i], height));
    }

    FrameStats const horizontal(analyzeFrame(large, marks, width, height, size));
    FrameStats const vertical(analyzeFrame(large_cw, marks_cw, height, width, size));

    bool const is_horizontal = horizontal.pairs >= vertical.pairs;
    FrameStats const& frame = is_horizontal ? horizontal : vertical;
    int const pairs_max = std::max(horizontal.pairs, vertical.pairs);
    int const pairs_min = std::min(horizontal.pairs, vertical.pairs);
    Cue const all_cues(frame.sum());
    double const z = all_cues.z();

    QString details = QStringLiteral("size=%1 large=%2 marks=%3 pairs=%4/%5 asc=%6 stack=%7 punct=%8 z=%9")
                      .arg(size).arg(large.size()).arg(marks.size())
                      .arg(horizontal.pairs).arg(vertical.pairs)
                      .arg(cueText(frame.ascender), cueText(frame.stacked), cueText(frame.punctuation))
                      .arg(z, 0, 'f', 2);

    // Letters in a text line have close neighbours along the line, but not across it.
    double const axis_ratio = double(pairs_max) / double(std::max(1, pairs_min));
    double const axis_conf = std::max(0.0, std::min(1.0, (axis_ratio - 1.5) / 1.5));
    double const dir_conf = std::max(0.0, std::min(1.0, (std::fabs(z) - 3.0) / 6.0));
    double const confidence = std::min(axis_conf, dir_conf);

    int angle = 0;
    if (is_horizontal) {
        angle = z >= 0 ? 0 : 180;
    } else {
        angle = z >= 0 ? 90 : 270;
    }

    OrientationResult result;
    if (pairs_max < 100) {
        result = OrientationResult(0, 0.0, false, QStringLiteral("no text"));
    } else if (axis_conf <= 0.0) {
        result = OrientationResult(0, 0.0, false, QStringLiteral("unclear text direction"));
    } else {
        // A cue that votes clearly against the others means that the page is not plain Latin text.
        Cue const cues[3] = { frame.ascender, frame.stacked, frame.punctuation };
        bool conflict = false;
        for (int i = 0; i < 3; ++i) {
            if (cues[i].z() * z < 0 && std::fabs(cues[i].z()) >= 2.0) {
                conflict = true;
            }
        }
        if (conflict) {
            result = OrientationResult(0, 0.0, false, QStringLiteral("cues disagree"));
        } else if (confidence < confidenceThreshold()) {
            result = OrientationResult(0, confidence, false, QStringLiteral("low confidence"));
        } else {
            result = OrientationResult(angle, confidence, true, QStringLiteral("text"));
        }
    }
    details += QStringLiteral(" guess=%1 conf=%2").arg(angle).arg(confidence, 0, 'f', 2);
    result.setDetails(details);
    return result;
}

OrientationResult
OrientationDetector::detect(QImage const& image, int const exif_hint)
{
    OrientationResult result(combine(detect(image), exif_hint));
    // Only a scan without a decision needs the shape, so the other scans skip this work.
    result.setShape(image.size(), result.isDecided() ? -1 : shapeAxis(image));
    if (!result.isDecided()) {
        result.setDetails(result.details() + QStringLiteral(" shape=%1").arg(result.shapeAxis()));
    }
    return result;
}

OrientationResult
OrientationDetector::combine(OrientationResult const& content, int const exif_hint)
{
    OrientationResult result;
    if (exif_hint < 0) {
        result = content;
    } else if (content.isDecided()) {
        QString const reason = content.angle() == exif_hint
                               ? QStringLiteral("text, exif agrees")
                               : QStringLiteral("text, exif disagrees");
        result = OrientationResult(content.angle(), content.confidence(), true, reason);
    } else if (exif_hint != 0) {
        // Without text the tag is the only evidence, thus the low confidence asks for a look at the page.
        result = OrientationResult(exif_hint, 0.3, true, QStringLiteral("exif only, ") + content.reason());
    } else {
        result = content;
    }
    result.setDetails(content.details());
    return result;
}

void
OrientationDetector::applyBookRotation(std::vector<OrientationResult>& results)
{
    int votes[4] = { 0, 0, 0, 0 };
    int decisions = 0;
    std::map<std::pair<int, int>, int> sizes;
    for (size_t i = 0; i < results.size(); ++i) {
        OrientationResult const& r = results[i];
        if (r.isDecided() && r.confidence() >= confidenceThreshold()) {
            ++votes[r.angle() / 90];
            ++decisions;
            ++sizes[std::make_pair(r.imageSize().width(), r.imageSize().height())];
        }
    }
    int book = 0;
    for (int i = 1; i < 4; ++i) {
        if (votes[i] > votes[book]) {
            book = i;
        }
    }
    // Only a clear majority of several scans shows the rotation of the book.
    if (votes[book] < 5 || votes[book] * 10 < decisions * 6) {
        return;
    }
    QSize book_size;
    int best = 0;
    for (auto const& kv : sizes) {
        if (kv.second > best) {
            best = kv.second;
            book_size = QSize(kv.first.first, kv.first.second);
        }
    }
    for (size_t i = 0; i < results.size(); ++i) {
        results[i] = applyBookRotation(results[i], book * 90, book_size);
    }
}

namespace
{

// A change of the scanner settings in one book changes the page scan size a little, while a cover scan differs a lot.
bool similarSize(QSize const& a, QSize const& b)
{
    if (a.isEmpty() || b.isEmpty()) {
        return a == b;
    }
    return std::abs(a.width() - b.width()) * 20 <= b.width() && std::abs(a.height() - b.height()) * 20 <= b.height();
}

} // namespace

OrientationResult
OrientationDetector::applyBookRotation(
    OrientationResult const& r, int const book_angle, QSize const& book_size)
{
    if (book_angle < 0) {
        return r;
    }
    OrientationResult changed;
    if (r.isDecided()) {
        if ((r.angle() - book_angle + 360) % 180 == 0) {
            return r;
        }
        changed = OrientationResult(
            book_angle, 0.5 * r.confidence(), true,
            QStringLiteral("text runs across the book, book rotation used")
        );
    } else if (similarSize(r.imageSize(), book_size)) {
        // A plate or a page without text has the size of the other page scans, so it lies in the book like them.
        changed = OrientationResult(book_angle, 0.0, true, QStringLiteral("no decision, book rotation used"));
    } else if (r.shapeAxis() >= 0 && r.shapeAxis() == book_angle % 180) {
        changed = OrientationResult(
            book_angle, 0.0, true, QStringLiteral("no text, the shape agrees with the book rotation")
        );
    } else {
        return r;
    }
    changed.setDetails(r.details());
    changed.setShape(r.imageSize(), r.shapeAxis());
    return changed;
}

int
OrientationDetector::shapeAxis(QImage const& image)
{
    if (image.isNull()) {
        return -1;
    }
    QImage const small(
        image.scaled(500, 500, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        .convertToFormat(QImage::Format_Grayscale8)
    );
    int const w = small.width();
    int const h = small.height();
    if (w < 20 || h < 20) {
        return -1;
    }
    auto px = [&small](int x, int y) {
        return int(small.constScanLine(y)[x]);
    };

    // The scanner background is the median of the border pixels.
    std::vector<int> border;
    for (int x = 0; x < w; ++x) {
        for (int k = 0; k < 4; ++k) {
            border.push_back(px(x, k));
            border.push_back(px(x, h - 1 - k));
        }
    }
    for (int y = 0; y < h; ++y) {
        for (int k = 0; k < 4; ++k) {
            border.push_back(px(k, y));
            border.push_back(px(w - 1 - k, y));
        }
    }
    std::nth_element(border.begin(), border.begin() + border.size() / 2, border.end());
    int const bg = border[border.size() / 2];

    std::vector<int> row_count(h, 0);
    std::vector<int> col_count(w, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (std::abs(px(x, y) - bg) > 40) {
                ++row_count[y];
                ++col_count[x];
            }
        }
    }
    int y0 = -1, y1 = -1, x0 = -1, x1 = -1;
    for (int y = 0; y < h; ++y) {
        if (row_count[y] * 4 > w) {
            if (y0 < 0) {
                y0 = y;
            }
            y1 = y;
        }
    }
    for (int x = 0; x < w; ++x) {
        if (col_count[x] * 4 > h) {
            if (x0 < 0) {
                x0 = x;
            }
            x1 = x;
        }
    }
    if (y0 < 0 || x0 < 0 || y1 - y0 < 10 || x1 - x0 < 10) {
        return -1;
    }
    int const bh = y1 - y0 + 1;
    int const bw = x1 - x0 + 1;
    if (bh * bw * 100 < 15 * w * h) {
        return -1;
    }
    bool const tall = bh >= bw;
    int const len = tall ? bh : bw;
    int const across = tall ? bw : bh;
    auto at = [&](int along, int a) {
        return tall ? px(x0 + a, y0 + along) : px(x0 + along, y0 + a);
    };

    // A fold is a sharp line at one place in the profiles of four strips along the long side of the object.
    int const from = len * 38 / 100;
    int const to = len * 62 / 100;
    // Up to three strong lines in each strip, because a tilted fold or a picture edge can move the strongest one.
    std::vector<std::vector<int> > candidates(4);
    for (int seg = 0; seg < 4; ++seg) {
        int const a0 = across * (10 + 20 * seg) / 100;
        int const a1 = std::max(a0 + 1, across * (30 + 20 * seg) / 100);
        std::vector<double> profile(len, 0.0);
        for (int i = 0; i < len; ++i) {
            double sum = 0.0;
            for (int a = a0; a < a1; ++a) {
                sum += at(i, a);
            }
            profile[i] = sum / (a1 - a0);
        }
        std::vector<double> abs_diff(len, 0.0);
        std::vector<double> window;
        for (int i = 0; i < len; ++i) {
            window.assign(profile.begin() + std::max(0, i - 12), profile.begin() + std::min(len, i + 13));
            std::nth_element(window.begin(), window.begin() + window.size() / 2, window.end());
            abs_diff[i] = std::fabs(profile[i] - window[window.size() / 2]);
        }
        std::vector<double> sorted(abs_diff);
        std::nth_element(sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end());
        double const noise = sorted[sorted.size() / 2] + 1e-3;
        std::vector<std::pair<double, int> > maxima;
        for (int i = std::max(1, from); i < std::min(len - 1, to); ++i) {
            if (abs_diff[i] >= abs_diff[i - 1] && abs_diff[i] > abs_diff[i + 1] && abs_diff[i] >= 6.0 * noise) {
                maxima.push_back(std::make_pair(abs_diff[i], i));
            }
        }
        std::sort(maxima.rbegin(), maxima.rend());
        for (size_t k = 0; k < maxima.size() && k < 3; ++k) {
            candidates[seg].push_back(maxima[k].second);
        }
    }

    double const tolerance = std::max(3.0, 0.04 * len);
    double const ratio = double(len) / across;
    int best_support = 0;
    int best_position = -1;
    for (int seg = 0; seg < 4; ++seg) {
        for (int c : candidates[seg]) {
            int support = 0;
            for (int other = 0; other < 4; ++other) {
                for (int p : candidates[other]) {
                    if (std::abs(p - c) <= tolerance) {
                        ++support;
                        break;
                    }
                }
            }
            // The two pages of a spread are equal, so the fold is near the middle, while a picture edge on a cover is often elsewhere.
            if (c >= len * 45 / 100 && c <= len * 55 / 100 && support > best_support) {
                best_support = support;
                best_position = c;
            }
        }
    }
    bool const fold = (best_support == 4 && ratio < 1.5) || (best_support == 3 && ratio < 1.4);

    // A spread needs its fold upright, and a single page needs its long side upright.
    if (fold) {
        return tall ? 90 : 0;
    }
    return tall ? 0 : 90;
}

int
OrientationDetector::exifRotation(QString const& file_path)
{
    QImageReader reader(file_path);
    if (!reader.canRead()) {
        return -1;
    }
    // A mirror tag does not occur in scans, and a rotation cannot correct a mirror, so this tag gives no hint.
    switch (int(reader.transformation())) {
    case QImageIOHandler::TransformationRotate90:
        return 90;
    case QImageIOHandler::TransformationRotate180:
        return 180;
    case QImageIOHandler::TransformationRotate270:
        return 270;
    default:
        return -1;
    }
}

} // namespace imageproc
