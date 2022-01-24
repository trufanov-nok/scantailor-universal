/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "ImageView.h"

#include "ImageTransformation.h"
#include "ImagePresentation.h"
#include "StatusBarProvider.h"
#include "settings/globalstaticsettings.h"
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QString>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QColor>
#include <QCursor>
#include <QDebug>
#include <Qt>
#ifndef Q_MOC_RUN
#include <boost/bind.hpp>
#endif
#include <algorithm>

namespace select_content
{

ImageView::ImageView(
    QImage const& image, QImage const& downscaled_image,
    ImageTransformation const& xform, QRectF const& content_rect, QRectF const& page_rect)
    :   ImageViewBase(
            image, downscaled_image,
            ImagePresentation(xform.transform(), xform.resultingPreCropArea())
        ),
        m_dragHandler(*this),
        m_zoomHandler(*this),
        m_pNoContentMenu(new QMenu(this)),
        m_pHaveContentMenu(new QMenu(this)),
        m_contentRect(content_rect),
        m_pageRect(page_rect),
        m_minBoxSize(10.0, 10.0)
{
    setMouseTracking(true);

    interactionState().setDefaultStatusTip(
        tr("Use the context menu to enable / disable the content box.")
    );

    QString const drag_tip(tr("Drag lines or corners to resize the content box. Hold %1 to move it, %2 to move along axes, %3 to shrink/stretch.")
                           .arg(GlobalStaticSettings::getShortcutText(ContentMove),
                           GlobalStaticSettings::getShortcutText(ContentMoveAxes),
                           GlobalStaticSettings::getShortcutText(ContentStretch)));

    // Setup corner drag handlers.
    static int const masks_by_corner[] = { TOP | LEFT, TOP | RIGHT, BOTTOM | RIGHT, BOTTOM | LEFT };
    for (int i = 0; i < 4; ++i) {
        m_corners[i].setPositionCallback(
            boost::bind(&ImageView::cornerPosition, this, masks_by_corner[i])
        );
        m_corners[i].setMoveRequestCallback(
            boost::bind(&ImageView::cornerMoveRequest, this, masks_by_corner[i], _1, _2)
        );
        m_corners[i].setDragFinishedCallback(
            boost::bind(&ImageView::dragFinished, this)
        );
        m_cornerHandlers[i].setObject(&m_corners[i]);
        m_cornerHandlers[i].setProximityStatusTip(drag_tip);
        Qt::CursorShape cursor = (i & 1) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
        m_cornerHandlers[i].setProximityCursor(cursor);
        m_cornerHandlers[i].setInteractionCursor(cursor);
        makeLastFollower(m_cornerHandlers[i]);
    }

    // Setup edge drag handlers.
    static int const masks_by_edge[] = { TOP, RIGHT, BOTTOM, LEFT };
    for (int i = 0; i < 4; ++i) {
        m_edges[i].setPositionCallback(
            boost::bind(&ImageView::edgePosition, this, masks_by_edge[i])
        );
        m_edges[i].setMoveRequestCallback(
            boost::bind(&ImageView::edgeMoveRequest, this, masks_by_edge[i], _1, _2)
        );
        m_edges[i].setDragFinishedCallback(
            boost::bind(&ImageView::dragFinished, this)
        );
        m_edgeHandlers[i].setObject(&m_edges[i]);
        m_edgeHandlers[i].setProximityStatusTip(drag_tip);
        Qt::CursorShape cursor = (i & 1) ? Qt::SizeHorCursor : Qt::SizeVerCursor;
        m_edgeHandlers[i].setProximityCursor(cursor);
        m_edgeHandlers[i].setInteractionCursor(cursor);
        makeLastFollower(m_edgeHandlers[i]);
    }

    m_contentRectArea.setProximityPriority(2);
    // Setup rectangle drag interaction
    m_contentRectArea.setPositionCallback(boost::bind(&ImageView::contentRectPosition, this));
    m_contentRectArea.setMoveRequestCallback(boost::bind(&ImageView::contentRectMoveRequest, this, _1));
    m_contentRectArea.setDragFinishedCallback(boost::bind(&ImageView::contentRectDragFinished, this));
    m_contentRectAreaHandler.setObject(&m_contentRectArea);
    m_contentRectAreaHandler.setProximityStatusTip(tr("Hold left mouse button to drag the content box."));
    m_contentRectAreaHandler.setInteractionStatusTip(tr("Release left mouse button to finish dragging."));
    if (const HotKeyInfo* hk = GlobalStaticSettings::m_hotKeyManager.get(ContentMove)) {
        if (!hk->sequences().isEmpty()) {
            m_contentRectAreaHandler.setKeyboardModifiers({hk->sequences()[0].m_modifierSequence});
        }
    }
    m_contentRectAreaHandler.setProximityCursor(Qt::DragMoveCursor);
    m_contentRectAreaHandler.setInteractionCursor(Qt::DragMoveCursor);

    if (m_contentRect.isValid()) {
        makeLastFollower(m_contentRectAreaHandler);
    }

    rootInteractionHandler().makeLastFollower(*this);
    rootInteractionHandler().makeLastFollower(m_dragHandler);
    rootInteractionHandler().makeLastFollower(m_zoomHandler);

    QAction* create = m_pNoContentMenu->addAction(tr("Create Content Box"));
    QAction* remove = m_pHaveContentMenu->addAction(tr("Remove Content Box"));

    create->setShortcut(GlobalStaticSettings::createShortcut(ContentInsert));
    remove->setShortcut(GlobalStaticSettings::createShortcut(ContentDelete));
    addAction(create);
    addAction(remove);
    connect(create, SIGNAL(triggered(bool)), this, SLOT(createContentBox()));
    connect(remove, SIGNAL(triggered(bool)), this, SLOT(removeContentBox()));

    if (m_contentRect.isValid()) {
        // override page size with content box size
        StatusBarProvider::setPagePhysSize(m_contentRect.size(), StatusBarProvider::getOriginalDpi());
        setCursorPosAdjustment(m_contentRect.topLeft());
    }
}

ImageView::~ImageView()
{
}

void
ImageView::createContentBox()
{
    if (!m_contentRect.isEmpty()) {
        return;
    }
    if (interactionState().captured()) {
        return;
    }

    QRectF const virtual_rect(virtualDisplayRect());
    QRectF content_rect(0, 0, virtual_rect.width() * 0.7, virtual_rect.height() * 0.7);
    content_rect.moveCenter(virtual_rect.center());
    m_contentRect = content_rect;
    update();
    emit manualContentRectSet(m_contentRect);

    StatusBarProvider::setPagePhysSize(m_contentRect.size(), StatusBarProvider::getOriginalDpi());
    setCursorPosAdjustment(m_contentRect.topLeft());
}

void
ImageView::removeContentBox()
{
    if (m_contentRect.isEmpty()) {
        return;
    }
    if (interactionState().captured()) {
        return;
    }

    m_contentRect = QRectF();
    update();
    emit manualContentRectSet(m_contentRect);

    StatusBarProvider::setPagePhysSize(virtualDisplayRect().size(), StatusBarProvider::getOriginalDpi());
    setCursorPosAdjustment(virtualDisplayRect().topLeft());
}

void
ImageView::onPaint(QPainter& painter, InteractionState const& interaction)
{
    if (m_contentRect.isNull()) {
        return;
    }

    if (! m_pageRect.isNull()) {
        // Draw detected page borders
        QPen pen(QColor(0xee, 0xee, 0x00, 0xcc));
        pen.setWidth(1);
        pen.setCosmetic(true);
        painter.setPen(pen);
        painter.setBrush(QColor(0xee, 0xee, 0x00, 0xcc));

        if (m_pageRect != virtualDisplayRect()) {
            QRectF box;
            QRectF r(virtualDisplayRect());

            box.setLeft(m_pageRect.left());
            box.setTop(r.top());
            box.setRight(m_pageRect.right());
            box.setBottom(m_pageRect.top());
            painter.drawRect(box);

            box.setLeft(r.left());
            box.setTop(r.top());
            box.setRight(m_pageRect.left());
            box.setBottom(r.bottom());
            painter.drawRect(box);

            box.setLeft(m_pageRect.left());
            box.setTop(m_pageRect.bottom());
            box.setRight(m_pageRect.right());
            box.setBottom(r.bottom());
            painter.drawRect(box);

            box.setLeft(m_pageRect.right());
            box.setTop(r.top());
            box.setRight(r.right());
            box.setBottom(r.bottom());
            painter.drawRect(box);
        }
    }

    painter.setRenderHints(QPainter::Antialiasing, true);

    // Draw the content bounding box.
    QPen pen(GlobalStaticSettings::m_content_sel_content_color_pen);
    pen.setWidth(1);
    pen.setCosmetic(true);
    painter.setPen(pen);

    painter.setBrush(GlobalStaticSettings::m_content_sel_content_color);

    // Pen strokes will be outside of m_contentRect - that's how drawRect() works.
    painter.drawRect(m_contentRect);
}

void
ImageView::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction)
{
    if (interaction.captured()) {
        // No context menus during resizing.
        return;
    }

    if (m_contentRect.isEmpty()) {
        m_pNoContentMenu->popup(event->globalPos());
    } else {
        m_pHaveContentMenu->popup(event->globalPos());
    }
}

QPointF
ImageView::cornerPosition(int edge_mask) const
{
    QRectF const r(virtualToWidget().mapRect(m_contentRect));
    QPointF pt;

    if (edge_mask & TOP) {
        pt.setY(r.top());
    } else if (edge_mask & BOTTOM) {
        pt.setY(r.bottom());
    }

    if (edge_mask & LEFT) {
        pt.setX(r.left());
    } else if (edge_mask & RIGHT) {
        pt.setX(r.right());
    }

    return pt;
}

void
ImageView::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_moveStart.isNull() &&
            !GlobalStaticSettings::checkModifiersMatch(ContentMove, event->modifiers()) &&
            !GlobalStaticSettings::checkModifiersMatch(ContentMoveAxes, event->modifiers())) {
        m_moveStart = QPointF();
    }

    ImageViewBase::mouseMoveEvent(event);
}

void
ImageView::mouseReleaseEvent(QMouseEvent* event)
{
    if (!m_moveStart.isNull()) {
        m_moveStart = QPointF();
    }

    ImageViewBase::mouseReleaseEvent(event);
}

void
ImageView::keyPressEvent(QKeyEvent* event)
{
    int dx = 0; int dy = 0;
    const Qt::KeyboardModifiers mask = event->modifiers();
    const Qt::Key key = (Qt::Key) event->key();
    if (GlobalStaticSettings::checkKeysMatch(ContentMoveUp, mask, key)) {
        dy--;
    } else if (GlobalStaticSettings::checkKeysMatch(ContentMoveDown, mask, key)) {
        dy++;
    } else if (GlobalStaticSettings::checkKeysMatch(ContentMoveLeft, mask, key)) {
        dx--;
    } else if (GlobalStaticSettings::checkKeysMatch(ContentMoveRight, mask, key)) {
        dx++;
    } else {
        ImageViewBase::keyPressEvent(event);
        return; // stop processing
    }

    QRectF r(virtualToWidget().mapRect(m_contentRect));
    if (dx || dy) {
        r.translate(dx, dy);
        forceInsideImage(r);
    }

    m_contentRect = widgetToVirtual().mapRect(r);
    update();

    if (m_contentRect.isValid()) {
        StatusBarProvider::setPagePhysSize(m_contentRect.size(), StatusBarProvider::getOriginalDpi());
        setCursorPosAdjustment(m_contentRect.topLeft());
    }

    emit manualContentRectSet(m_contentRect);

    ImageViewBase::keyPressEvent(event);
}

void
ImageView::cornerMoveRequest(int edge_mask, QPointF const& pos, Qt::KeyboardModifiers mask)
{
    QRectF r(virtualToWidget().mapRect(m_contentRect));
    qreal const minw = m_minBoxSize.width();
    qreal const minh = m_minBoxSize.height();

    // Only Ctrl or only Shift is pressed
    const bool move = GlobalStaticSettings::checkModifiersMatch(ContentMove, mask);
    const bool move_along_exes = GlobalStaticSettings::checkModifiersMatch(ContentMoveAxes, mask);
    if (move || move_along_exes) {
        if (!m_moveStart.isNull()) {
            QPointF diff = pos - m_moveStart;
            if (move_along_exes) {
                if (edge_mask & TOP || edge_mask & BOTTOM) {
                    diff.setX(0);
                } else {
                    diff.setY(0);
                }
            }
            r.translate(diff.x(), diff.y());
        }
        m_moveStart = pos;
        forceInsideImage(r);
    } else {
        m_moveStart = QPointF();

        if (!GlobalStaticSettings::checkModifiersMatch(ContentStretch, mask)) {
            if (edge_mask & TOP) {
                r.setTop(std::min(pos.y(), r.bottom() - minh));
            } else if (edge_mask & BOTTOM) {
                r.setBottom(std::max(pos.y(), r.top() + minh));
            }

            if (edge_mask & LEFT) {
                r.setLeft(std::min(pos.x(), r.right() - minw));
            } else if (edge_mask & RIGHT) {
                r.setRight(std::max(pos.x(), r.left() + minw));
            }
        } else {
            // Ctrl and Shift are pressed at the same time
            if (edge_mask & TOP || edge_mask & BOTTOM) {
                qreal dy = (edge_mask & TOP) ? pos.y() - r.top() : r.bottom() - pos.y();
                r.setTop(std::min(r.top() + dy, r.bottom() - minh));
                r.setBottom(std::max(r.bottom() - dy, r.top() + minh));
                edge_mask = TOP | BOTTOM;
            } else {
                qreal dx = (edge_mask & LEFT) ? pos.x() - r.left() : r.right() - pos.x();
                r.setLeft(std::min(r.left() + dx, r.right() - minw));
                r.setRight(std::max(r.right() - dx, r.left() + minw));
                edge_mask = LEFT | RIGHT;
            }
        }

        forceInsideImage(r, edge_mask);
    }

    m_contentRect = widgetToVirtual().mapRect(r);
    update();

    if (m_contentRect.isValid()) {
        StatusBarProvider::setPagePhysSize(m_contentRect.size(), StatusBarProvider::getOriginalDpi());
        setCursorPosAdjustment(m_contentRect.topLeft());
    }
}

QLineF
ImageView::edgePosition(int const edge) const
{
    QRectF const r(virtualToWidget().mapRect(m_contentRect));

    if (edge == TOP) {
        return QLineF(r.topLeft(), r.topRight());
    } else if (edge == BOTTOM) {
        return QLineF(r.bottomLeft(), r.bottomRight());
    } else if (edge == LEFT) {
        return QLineF(r.topLeft(), r.bottomLeft());
    } else {
        return QLineF(r.topRight(), r.bottomRight());
    }
}

void
ImageView::edgeMoveRequest(int const edge, QLineF const& line, Qt::KeyboardModifiers mask)
{
    cornerMoveRequest(edge, line.p1(), mask);
}

void
ImageView::dragFinished()
{
    emit manualContentRectSet(m_contentRect);
}


QRectF ImageView::contentRectPosition() const {
  return virtualToWidget().mapRect(m_contentRect);
}

void ImageView::contentRectMoveRequest(const QPolygonF& polyMoved) {
  QRectF contentRectInWidget(polyMoved.boundingRect());

  const QRectF imageRect(virtualToWidget().mapRect(virtualDisplayRect()));
  if (contentRectInWidget.left() < imageRect.left()) {
    contentRectInWidget.translate(imageRect.left() - contentRectInWidget.left(), 0);
  }
  if (contentRectInWidget.right() > imageRect.right()) {
    contentRectInWidget.translate(imageRect.right() - contentRectInWidget.right(), 0);
  }
  if (contentRectInWidget.top() < imageRect.top()) {
    contentRectInWidget.translate(0, imageRect.top() - contentRectInWidget.top());
  }
  if (contentRectInWidget.bottom() > imageRect.bottom()) {
    contentRectInWidget.translate(0, imageRect.bottom() - contentRectInWidget.bottom());
  }

  m_contentRect = widgetToVirtual().mapRect(contentRectInWidget);

  update();

  if (m_contentRect.isValid()) {
      StatusBarProvider::setPagePhysSize(m_contentRect.size(), StatusBarProvider::getOriginalDpi());
      setCursorPosAdjustment(m_contentRect.topLeft());
  }

  update();
}

void ImageView::contentRectDragFinished()
{
  emit manualContentRectSet(m_contentRect);
}

void
ImageView::forceInsideImage(QRectF& widget_rect, int edge_mask) const
{
    qreal const minw = m_minBoxSize.width();
    qreal const minh = m_minBoxSize.height();
    QRectF const image_rect(virtualToWidget().mapRect(virtualDisplayRect()));

    if ((edge_mask & LEFT) && widget_rect.left() < image_rect.left()) {
        widget_rect.setLeft(image_rect.left());
        widget_rect.setRight(std::max(widget_rect.right(), widget_rect.left() + minw));
    }
    if ((edge_mask & RIGHT) && widget_rect.right() > image_rect.right()) {
        widget_rect.setRight(image_rect.right());
        widget_rect.setLeft(std::min(widget_rect.left(), widget_rect.right() - minw));
    }
    if ((edge_mask & TOP) && widget_rect.top() < image_rect.top()) {
        widget_rect.setTop(image_rect.top());
        widget_rect.setBottom(std::max(widget_rect.bottom(), widget_rect.top() + minh));
    }
    if ((edge_mask & BOTTOM) && widget_rect.bottom() > image_rect.bottom()) {
        widget_rect.setBottom(image_rect.bottom());
        widget_rect.setTop(std::min(widget_rect.top(), widget_rect.bottom() - minh));
    }
}

} // namespace select_content
