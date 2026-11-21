/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>

    Backported from ScanTailor Advanced (src/core/ContentMask.h).

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

#ifndef CONTENTMASK_H_
#define CONTENTMASK_H_

#include "imageproc/BinaryImage.h"
#include <QRect>
#include <QRectF>
#include <QPointF>
#include <QString>
#include <QTransform>
#include <QVector>
#include <QStringList>

namespace imageproc
{
class GrayImage;
}
class ImageTransformation;
class TaskStatus;

// The GUI double-click and the CLI option --content-snap use this 150 DPI binary page to find ink near a point.
class ContentMask
{
public:
    ContentMask() {}

    ContentMask(imageproc::GrayImage const& gray_image,
                ImageTransformation const& xform, TaskStatus const& status);

    // The result grows by 1 px on each side, as in ScanTailor Advanced.
    QRect findContentInArea(QRect const& area) const;

    // The GUI modifiers select the first four modes, and only the CLI can name one edge.
    enum SnapMode {
        SNAP_NEAREST = 0, SNAP_HORIZONTAL = 1, SNAP_VERTICAL = 2, SNAP_CORNER = 3,
        SNAP_LEFT, SNAP_RIGHT, SNAP_TOP, SNAP_BOTTOM
    };

    // All arguments use page coordinates, so that the GUI and the CLI share one implementation.
    bool snapContentRect(QRectF& content_rect, QPointF const& pos, SnapMode mode,
                         QRectF const& bounds, double min_size = 10.0) const;

    imageproc::BinaryImage const& image() const
    {
        return m_image;
    }

    QTransform const& originalToContentXform() const
    {
        return m_originalToContentXform;
    }

    QTransform const& contentToOriginalXform() const
    {
        return m_contentToOriginalXform;
    }

    bool isNull() const
    {
        return m_image.isNull();
    }
private:
    imageproc::BinaryImage m_image;
    QTransform m_originalToContentXform;
    QTransform m_contentToOriginalXform;
    QTransform m_virtualToContentXform;
    QTransform m_contentToVirtualXform;
};

// The CLI help text gives the syntax of one --content-snap entry.
struct ContentSnapRequest {
    QString fileName;
    QChar side;            // 'L', 'R' or 0 (single page / any)
    QPointF pos;
    ContentMask::SnapMode mode;
};

// The parser skips an entry that it cannot read and adds that entry to errors.
QVector<ContentSnapRequest> parseContentSnap(QString const& value, QStringList* errors = 0);

#endif
