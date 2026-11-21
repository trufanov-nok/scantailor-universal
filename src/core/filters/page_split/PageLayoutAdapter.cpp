/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
    Backported from ScanTailor Advanced (src/core/filters/page_split/PageLayoutAdapter.cpp).

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

#include "PageLayoutAdapter.h"
#include "Params.h"
#include "Dependencies.h"
#include "LegacySupport.h"
#include <algorithm>

namespace page_split
{

static QLineF topToBottom(QLineF const& line)
{
    return line.y1() <= line.y2() ? line : QLineF(line.p2(), line.p1());
}

static bool cutterLessThan(QLineF const& line1, QLineF const& line2)
{
    return line1.x1() < line2.x1();
}

QLineF
PageLayoutAdapter::adaptCutter(QLineF const& cutter, QRectF const& new_rect)
{
    if (!new_rect.isValid() || cutter.isNull()) {
        return topToBottom(cutter);
    }

    QLineF const upper_border(new_rect.topLeft(), new_rect.topRight());
    QPointF upper_point;
    if (QLineIntersect(upper_border, cutter, &upper_point) == QLineF::NoIntersection) {
        return topToBottom(cutter);
    }
    upper_point.setX(qBound(new_rect.left(), upper_point.x(), new_rect.right()));

    QLineF const lower_border(new_rect.bottomLeft(), new_rect.bottomRight());
    QPointF lower_point;
    if (QLineIntersect(lower_border, cutter, &lower_point) == QLineF::NoIntersection) {
        return topToBottom(cutter);
    }
    lower_point.setX(qBound(new_rect.left(), lower_point.x(), new_rect.right()));

    return QLineF(upper_point, lower_point);
}

QVector<QLineF>
PageLayoutAdapter::adaptCutters(QVector<QLineF> const& cutters, QRectF const& new_rect)
{
    QVector<QLineF> adapted;
    for (QLineF const& cutter : cutters) {
        adapted.append(adaptCutter(cutter, new_rect));
    }

    std::sort(adapted.begin(), adapted.end(), cutterLessThan);

    // Cutters that cross inside the outline give an invalid layout, so move the cross point to the nearest edge.
    qreal const upper_bound = new_rect.top();
    qreal const lower_bound = new_rect.bottom();
    for (int i = 1; i < adapted.size(); ++i) {
        QLineF left_cutter(adapted[i - 1]);
        QLineF right_cutter(adapted[i]);
        QPointF point;
        if (QLineIntersect(left_cutter, right_cutter, &point) == QLineF::NoIntersection) {
            continue;
        }
        if (point.y() >= lower_bound || point.y() <= upper_bound) {
            continue;
        }

        if (lower_bound - point.y() <= (lower_bound - upper_bound) / 2) {
            left_cutter.setP2(QPointF(point.x(), lower_bound));
            right_cutter.setP2(QPointF(point.x(), lower_bound));
        } else {
            left_cutter.setP1(QPointF(point.x(), upper_bound));
            right_cutter.setP1(QPointF(point.x(), upper_bound));
        }
        adapted[i - 1] = left_cutter;
        adapted[i] = right_cutter;
    }

    return adapted;
}

void
PageLayoutAdapter::correctPageLayoutType(PageLayout* layout)
{
    QRectF const outline(layout->uncutOutline().boundingRect().toRect());

    if (layout->type() == PageLayout::SINGLE_PAGE_CUT) {
        QLineF const cutter1(layout->cutterLine(0).toLine());
        QLineF const cutter2(layout->cutterLine(1).toLine());

        bool const cutter1_on_edge = cutter1.x1() == cutter1.x2()
                                     && (cutter1.x1() == outline.left() || cutter1.x1() == outline.right());
        bool const cutter2_on_edge = cutter2.x1() == cutter2.x2()
                                     && (cutter2.x1() == outline.left() || cutter2.x1() == outline.right());
        if (cutter1_on_edge && cutter2_on_edge) {
            layout->setType(PageLayout::SINGLE_PAGE_UNCUT);
            return;
        }

        QPointF point;
        int const intersection = QLineIntersect(cutter1, cutter2, &point);
        bool const cross_inside = intersection != QLineF::NoIntersection
                                  && point.y() > outline.top() && point.y() < outline.bottom();
        bool const same_line = intersection == QLineF::NoIntersection && cutter1.p1() == cutter2.p1();
        if (cross_inside || same_line) {
            layout->setType(PageLayout::SINGLE_PAGE_UNCUT);
        }
    } else if (layout->type() == PageLayout::TWO_PAGES) {
        QLineF const cutter(layout->cutterLine(0).toLine());
        if (cutter.x1() == cutter.x2()
                && (cutter.x1() == outline.left() || cutter.x1() == outline.right())) {
            layout->setType(PageLayout::SINGLE_PAGE_UNCUT);
        }
    }
}

PageLayout
PageLayoutAdapter::adaptPageLayout(PageLayout const& layout, QRectF const& outline)
{
    if (layout.uncutOutline().boundingRect() == outline) {
        return layout;
    }

    PageLayout new_layout(outline);
    if (layout.type() == PageLayout::SINGLE_PAGE_CUT) {
        QVector<QLineF> cutters;
        cutters.append(layout.cutterLine(0));
        cutters.append(layout.cutterLine(1));
        QVector<QLineF> const adapted(adaptCutters(cutters, outline));
        new_layout = PageLayout(outline, adapted[0], adapted[1]);
        correctPageLayoutType(&new_layout);
    } else if (layout.type() == PageLayout::TWO_PAGES) {
        new_layout = PageLayout(outline, adaptCutter(layout.cutterLine(0), outline));
        correctPageLayoutType(&new_layout);
    }

    return new_layout;
}

bool
PageLayoutAdapter::adaptParams(Params& params, Dependencies const& deps, QRectF const& outline)
{
    // The estimator must find a new auto split line, because the gutter can be in a different position on the new image.
    if (deps.layoutType() == AUTO_LAYOUT_TYPE || params.splitLineMode() != MODE_MANUAL) {
        return false;
    }

    Dependencies const& old_deps = params.dependencies();
    if (old_deps.isNull() || old_deps.imageSize() == deps.imageSize()) {
        return false;
    }

    // Other changes, for example a new rotation, make the old cutters useless, so the estimator must find a new layout.
    Dependencies const same_size_deps(old_deps.imageSize(), deps.orientation(), deps.layoutType());
    if (!same_size_deps.compatibleWith(params)) {
        return false;
    }

    PageLayout const new_layout(adaptPageLayout(params.pageLayout(), outline));
    Dependencies new_deps(deps);
    new_deps.setLayoutType(new_layout.toLayoutType());
    Params adapted(new_layout, new_deps, params.splitLineMode(), params.origDpi());
    adapted.setForceReprocess(params.getForceReprocess());
    params = adapted;
    return true;
}

} // namespace page_split
