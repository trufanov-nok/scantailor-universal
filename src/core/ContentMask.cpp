/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>

    Backported from ScanTailor Advanced (src/core/ContentMask.cpp).

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

#include "ContentMask.h"
#include "MultipleTargetsSupport.h"
#include "Despeckle.h"
#include "Dpi.h"
#include "ImageTransformation.h"
#include "TaskStatus.h"
#include "imageproc/BWColor.h"
#include "imageproc/Binarize.h"
#include "imageproc/GrayImage.h"
#include "imageproc/PolygonRasterizer.h"
#include "imageproc/Transform.h"
#include <QColor>
#include <QRegularExpression>
#include <QImage>
#include <algorithm>
#include <limits>
#include <stdint.h>

using namespace imageproc;

ContentMask::ContentMask(
    GrayImage const& gray_image, ImageTransformation const& xform, TaskStatus const& status)
{
    ImageTransformation xform150dpi(xform);
    xform150dpi.preScaleToDpi(Dpi(150, 150));
    if (xform150dpi.resultingRect().toRect().isEmpty()) {
        return;
    }
    m_originalToContentXform = xform150dpi.transform();
    m_contentToOriginalXform = m_originalToContentXform.inverted();
    // virtual (page) coordinates are the original image mapped by xform.transform()
    m_contentToVirtualXform = m_contentToOriginalXform * xform.transform();
    m_virtualToContentXform = m_contentToVirtualXform.inverted();

    QImage const gray150(
        transformToGray(
            gray_image, xform150dpi.transform(), xform150dpi.resultingRect().toRect(),
            OutsidePixels::assumeColor(Qt::white)
        )
    );
    m_image = binarizeWolf(gray150, QSize(51, 51), 50);
    PolygonRasterizer::fillExcept(
        m_image, WHITE, xform150dpi.resultingPreCropArea(), Qt::WindingFill
    );
    Despeckle::despeckleInPlace(m_image, Dpi(150, 150), Despeckle::NORMAL, status);
}

QRect
ContentMask::findContentInArea(QRect const& area) const
{
    if (isNull()) {
        return QRect();
    }

    QRect const finding_area(area.intersected(m_image.rect()));
    if (finding_area.isEmpty()) {
        return QRect();
    }

    uint32_t const* line = m_image.data();
    int const stride = m_image.wordsPerLine();
    uint32_t const msb = uint32_t(1) << 31;

    int top = std::numeric_limits<int>::max();
    int left = std::numeric_limits<int>::max();
    int bottom = std::numeric_limits<int>::min();
    int right = std::numeric_limits<int>::min();

    line += finding_area.top() * stride;
    for (int y = finding_area.top(); y <= finding_area.bottom(); ++y) {
        for (int x = finding_area.left(); x <= finding_area.right(); ++x) {
            if (line[x >> 5] & (msb >> (x & 31))) {
                top = std::min(top, y);
                left = std::min(left, x);
                bottom = std::max(bottom, y);
                right = std::max(right, x);
            }
        }
        line += stride;
    }

    if (top > bottom) {
        return QRect();
    }

    QRect found(left, top, right - left + 1, bottom - top + 1);
    found.adjust(-1, -1, 1, 1);
    return found;
}

bool
ContentMask::snapContentRect(
    QRectF& content_rect, QPointF const& pos, SnapMode const mode, QRectF const& bounds,
    double const min_size) const
{
    if (isNull() || content_rect.isEmpty()) {
        return false;
    }

    QPointF const content_pos(m_virtualToContentXform.map(pos));
    QRect const finding_area((content_pos - QPointF(15, 15)).toPoint(), QSize(30, 30));
    QRect const found_area(findContentInArea(finding_area));
    if (found_area.isEmpty()) {
        return false;
    }
    QRectF const found(m_contentToVirtualXform.mapRect(QRectF(found_area)).intersected(bounds));
    if (found.isEmpty()) {
        return false;
    }

    enum { LEFT = 1, RIGHT = 2, TOP = 4, BOTTOM = 8 };

    if (!content_rect.contains(pos)) {
        content_rect |= found;
        return true;
    }

    QRectF r(content_rect);
    double const top_dist = pos.y() - r.top();
    double const left_dist = pos.x() - r.left();
    double const bottom_dist = r.bottom() - pos.y();
    double const right_dist = r.right() - pos.x();
    int const h_edge = (left_dist <= right_dist) ? LEFT : RIGHT;
    int const v_edge = (top_dist <= bottom_dist) ? TOP : BOTTOM;

    int edge_mask;
    switch (mode) {
    case SNAP_HORIZONTAL:
        edge_mask = h_edge;
        break;
    case SNAP_VERTICAL:
        edge_mask = v_edge;
        break;
    case SNAP_CORNER:
        edge_mask = h_edge | v_edge;
        break;
    case SNAP_LEFT:
        edge_mask = LEFT;
        break;
    case SNAP_RIGHT:
        edge_mask = RIGHT;
        break;
    case SNAP_TOP:
        edge_mask = TOP;
        break;
    case SNAP_BOTTOM:
        edge_mask = BOTTOM;
        break;
    default:
        edge_mask = (std::min(left_dist, right_dist) <= std::min(top_dist, bottom_dist)) ? h_edge : v_edge;
    }

    if (edge_mask & TOP) {
        r.setTop(std::min(found.top(), r.bottom() - min_size));
    } else if (edge_mask & BOTTOM) {
        r.setBottom(std::max(found.bottom(), r.top() + min_size));
    }
    if (edge_mask & LEFT) {
        r.setLeft(std::min(found.left(), r.right() - min_size));
    } else if (edge_mask & RIGHT) {
        r.setRight(std::max(found.right(), r.left() + min_size));
    }
    content_rect = r;
    return true;
}

QVector<ContentSnapRequest>
parseContentSnap(QString const& value, QStringList* errors)
{
    // <file>[:L|R]@<x>,<y>[:nearest|h|v|hv]
    static QRegularExpression const rx(
        "^(.+?)(?::([LRlr]))?@(-?[0-9.]+),(-?[0-9.]+)(?::(nearest|h|v|hv|left|right|top|bottom))?$"
    );
    QVector<ContentSnapRequest> out;
    for (QString const& raw : value.split(';', QStringSkipEmptyParts)) {
        QString const item(raw.trimmed());
        QRegularExpressionMatch const m(rx.match(item));
        if (!m.hasMatch()) {
            if (errors) {
                errors->append(item);
            }
            continue;
        }
        ContentSnapRequest req;
        req.fileName = m.captured(1);
        req.side = m.captured(2).isEmpty() ? QChar() : m.captured(2).toUpper().at(0);
        req.pos = QPointF(m.captured(3).toDouble(), m.captured(4).toDouble());
        QString const mode(m.captured(5));
        req.mode = mode == "h" ? ContentMask::SNAP_HORIZONTAL
                   : mode == "v" ? ContentMask::SNAP_VERTICAL
                   : mode == "hv" ? ContentMask::SNAP_CORNER
                   : mode == "left" ? ContentMask::SNAP_LEFT
                   : mode == "right" ? ContentMask::SNAP_RIGHT
                   : mode == "top" ? ContentMask::SNAP_TOP
                   : mode == "bottom" ? ContentMask::SNAP_BOTTOM
                   : ContentMask::SNAP_NEAREST;
        out.append(req);
    }
    return out;
}
