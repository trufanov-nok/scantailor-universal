/*
    Scan Tailor Universal - Interactive post-processing tool for scanned
    pages. A fork of Scan Tailor by Joseph Artsimovich.
    Copyright (C) 2020 Alexander Trufanov <trufanovan@gmail.com>

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

#include "ZoneEllipseVertexDragInteraction.h"
#include "ZoneInteractionContext.h"
#include "EditableZoneSet.h"
#include "ImageViewBase.h"
#include "settings/globalstaticsettings.h"
#include "LocalClipboard.h"
#include <QTransform>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QLinearGradient>
#include <Qt>
#include <QLineF>

ZoneEllipseVertexDragInteraction::ZoneEllipseVertexDragInteraction(
    ZoneInteractionContext& context, InteractionState& interaction,
    EditableEllipse::Ptr const& ellipse, int vertexId)
    :   m_rContext(context),
        m_ptrEllipse(ellipse),
        m_vertexId(vertexId)
{
    QPointF const screen_mouse_pos(
        m_rContext.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5)
    );
    QTransform const to_screen(m_rContext.imageView().imageToWidget());
    if (vertexId == 5) {
        m_dragOffset = to_screen.map(ellipse->center()) - screen_mouse_pos;
    } else {
        m_dragOffset = to_screen.map(ellipse->const_data()[vertexId]) - screen_mouse_pos;
    }

    m_interaction.setInteractionStatusTip(tr("Hold %1 to make a circle.").arg(GlobalStaticSettings::getShortcutText(ZoneEllipse)));

    interaction.capture(m_interaction);
}

void
ZoneEllipseVertexDragInteraction::onPaint(QPainter& painter, InteractionState const& /*interaction*/)
{
    painter.setWorldMatrixEnabled(false);
    painter.setRenderHint(QPainter::Antialiasing);

    QTransform const to_screen(m_rContext.imageView().imageToWidget());

    for (EditableZoneSet::Zone const& zone : m_rContext.zones()) {
        if (!zone.isEllipse() || zone.ellipse() != m_ptrEllipse) {
            // Draw the whole spline in solid color.
            m_visualizer.drawZone(painter, to_screen, zone);
            continue;
        }
    }

    painter.save();
    painter.setWorldMatrixEnabled(true);
    painter.setTransform(m_ptrEllipse->transform(to_screen));
    painter.drawEllipse(m_ptrEllipse->center(), m_ptrEllipse->rx(), m_ptrEllipse->ry());
    painter.restore();

    m_visualizer.drawVertex(
        painter, to_screen.map(m_ptrEllipse->center()),  m_visualizer.highlightBrightColor()
    );
    for (const QPointF &p : m_ptrEllipse->const_data()) {
        m_visualizer.drawVertex(
                    painter, to_screen.map(p), m_visualizer.highlightBrightColor()
                    );
    }

}

void
ZoneEllipseVertexDragInteraction::onMouseReleaseEvent(
    QMouseEvent* event, InteractionState& /*interaction*/)
{
    if (event->button() == Qt::LeftButton) {
        LocalClipboard::getInstance()->setLastZoneEllipse(SerializableEllipse(*m_ptrEllipse));
        m_rContext.zones().commit();
        makePeerPreceeder(*m_rContext.createDefaultInteraction());
        delete this;
    }
}

void
ZoneEllipseVertexDragInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& /*interaction*/)
{
    QTransform const from_screen(m_rContext.imageView().widgetToImage());

    if (m_vertexId == 5) { // center
        m_ptrEllipse->setCenter(from_screen.map(event->pos() + QPointF(0.5, 0.5) + m_dragOffset));
    } else {
//        QTransform const to_screen(m_rContext.imageView().imageToWidget());
        m_ptrEllipse->changePoint(m_vertexId, from_screen.map(event->pos() + QPointF(0.5, 0.5) + m_dragOffset));
    }

    if (GlobalStaticSettings::checkModifiersMatch(ZoneEllipse, event->modifiers())) {
        if (m_vertexId == 1 || m_vertexId == 3) {// move by ry vertex
            m_ptrEllipse->setRx(m_ptrEllipse->ry());
        } else {
            m_ptrEllipse->setRy(m_ptrEllipse->rx());
        }
    }

    m_rContext.imageView().update();
}

