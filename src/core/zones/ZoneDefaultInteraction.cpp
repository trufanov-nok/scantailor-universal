/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "ZoneDefaultInteraction.h"
#include "ZoneInteractionContext.h"
#include "EditableZoneSet.h"
#include "ImageViewBase.h"
#include "InteractionState.h"
#include "LocalClipboard.h"
#include "SerializableSpline.h"
#include "SerializableEllipse.h"
#include "settings/globalstaticsettings.h"
#include <QTransform>
#include <QPolygon>
#include <QPointF>
#include <QPen>
#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QLinearGradient>
#include <Qt>
#include <QMouseEvent>
#include <QApplication>
#include <QAction>
#include <vector>
#include <assert.h>

ZoneDefaultInteraction::ZoneDefaultInteraction(ZoneInteractionContext& context)
    :   m_rContext(context),
        m_dragHandler(context.imageView()),
        m_dragWatcher(m_dragHandler),
        m_mouse_over_zone(false),
        m_lastMovingState(ShiftStateUnknown),
        m_dragCopyModifiersHeld(false)
{
    makeLastFollower(m_dragHandler);
    m_dragHandler.makeFirstFollower(m_dragWatcher);

    m_vertexProximity.setProximityStatusTip(tr("Drag the vertex. Hold %1 to make a right angle. Press %2 to delete the vertex.")
                                            .arg(GlobalStaticSettings::getShortcutText(ZoneRectangle),
                                                 GlobalStaticSettings::getShortcutText(ZoneDelete)));
    m_segmentProximity.setProximityStatusTip(tr("Click to create a new vertex here."));
    m_ellipseCenterProximity.setProximityStatusTip(tr("Hold to move the ellipse."));
    m_ellipseCenterProximity.setProximityCursor(QCursor(Qt::DragMoveCursor));
    m_ellipseCoVertexProximity.setProximityStatusTip(tr("Hold to change ellipse size or angle."));
    m_zoneAreaProximity.setProximityStatusTip(tr("Right click to edit zone properties. Hold %1 to move, %2 to copy. Press %3 to delete the zone.")
            .arg(GlobalStaticSettings::getShortcutText(ZoneMove),
                 QHotKeys::modifiersToString(Qt::ControlModifier | Qt::AltModifier),
                 GlobalStaticSettings::getShortcutText(ZoneDelete)));
    updateDefaultStatusTip();

    m_pasteAction = m_defaultMenu.addAction(tr("&Paste"));
    m_pasteAction->setShortcut(GlobalStaticSettings::createShortcut(ZonePaste));

    QObject::connect(m_pasteAction, &QAction::triggered, [ this ]() {
        if (LocalClipboard* clb = LocalClipboard::getInstance()) {
            clb->pasteZone(m_rContext);
        }
    });
}

void
ZoneDefaultInteraction::updateDefaultStatusTip()
{
    QString status_tip;
    switch (m_rContext.zoneCreationMode()) {
    case ZoneInteractionContext::LASSO:
        status_tip = tr("Hold the left mouse button and move the mouse to draw a new zone.");
        break;
    case ZoneInteractionContext::RECTANGULAR:
        status_tip = tr("Click to start creating a new rectangular zone.");
        break;
    default:
        status_tip = tr("Click to start creating a new zone.");
        break;
    }
    status_tip.append(" ").append(tr("Press P, L or R for polygonal, lasso or rectangular mode."));

    if (LocalClipboard::getInstance()->lastZoneIsValid()) {
        status_tip.append(" ").append(tr("%1 + double click to repeat the last zone.")
                                      .arg(GlobalStaticSettings::getShortcutText(ZoneClone)));
    }
    m_rContext.imageView().interactionState().setDefaultStatusTip(status_tip);
}

bool
ZoneDefaultInteraction::isDragCopyModifiers(Qt::KeyboardModifiers modifiers)
{
    return modifiers == (Qt::ControlModifier | Qt::AltModifier);
}

void
ZoneDefaultInteraction::onPaint(QPainter& painter, InteractionState const& interaction)
{
    painter.setWorldMatrixEnabled(false);
    painter.setRenderHint(QPainter::Antialiasing);

    QTransform const to_screen(m_rContext.imageView().imageToWidget());

    const EditableZoneSet & zones = m_rContext.zones();
    for (EditableZoneSet::Zone const& zone : zones) {
        if (!zone.isEllipse()) {
            EditableSpline::Ptr const& spline = zone.spline();
            m_visualizer.prepareForSpline(painter, spline);
            QPolygonF points;

            if (!interaction.captured() && interaction.proximityLeader(m_vertexProximity)
                    && spline == m_ptrNearestVertexSpline) {
                SplineVertex::Ptr vertex(m_ptrNearestVertex->next(SplineVertex::LOOP));
                for (; vertex != m_ptrNearestVertex; vertex = vertex->next(SplineVertex::LOOP)) {
                    points.push_back(to_screen.map(vertex->point()));
                }
                painter.drawPolyline(points);
            } else if (!interaction.captured() && interaction.proximityLeader(m_segmentProximity)
                       && spline == m_ptrNearestSegmentSpline) {
                SplineVertex::Ptr vertex(m_nearestSegment.prev);
                do {
                    vertex = vertex->next(SplineVertex::LOOP);
                    points.push_back(to_screen.map(vertex->point()));
                } while (vertex != m_nearestSegment.prev);
                painter.drawPolyline(points);
            } else {
                m_visualizer.drawSpline(painter, to_screen, spline);
            }
        } else {
            EditableEllipse::Ptr const& ellipse = zone.ellipse();
//            QLinearGradient gradient;
//            gradient.setColorAt(0.0, m_visualizer.solidColor());
//            gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());
//            QPen pen(painter.pen());
//            pen.setBrush(gradient);
//            painter.setPen(pen);
            m_visualizer.drawEllipse(painter, to_screen, ellipse);
            if (m_mouse_over_zone && zone.ellipse() == m_ptrNearestEllipse) {
                m_visualizer.drawVertex(painter, to_screen.map(ellipse->center()), m_visualizer.solidColor());
                for (QPointF const & p: ellipse->const_data()) {
                    m_visualizer.drawVertex(painter, to_screen.map(p), m_visualizer.solidColor());
                }
            }
        }
    }

    if (interaction.proximityLeader(m_ellipseCenterProximity)) {
        m_visualizer.drawVertex(painter, m_ptrNearestEllipse->center(to_screen), m_visualizer.highlightBrightColor());
    } else
    if (interaction.proximityLeader(m_ellipseCoVertexProximity) && m_nearestEllipseVertexId < 5) {
        const QPointF p = to_screen.map(m_ptrNearestEllipse->const_data()[m_nearestEllipseVertexId]);
        m_visualizer.drawVertex(painter, p, m_visualizer.highlightBrightColor());
    } else
    if (interaction.proximityLeader(m_vertexProximity) && m_ptrNearestVertex) {
        // Draw the two adjacent edges in gradient red-to-orange.
        QLinearGradient gradient; // From inactive to active point.
        gradient.setColorAt(0.0, m_visualizer.solidColor());
        gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

        QPen pen(painter.pen());

        QPointF const prev(to_screen.map(m_ptrNearestVertex->prev(SplineVertex::LOOP)->point()));
        QPointF const pt(to_screen.map(m_ptrNearestVertex->point()));
        QPointF const next(to_screen.map(m_ptrNearestVertex->next(SplineVertex::LOOP)->point()));

        gradient.setStart(prev);
        gradient.setFinalStop(pt);
        pen.setBrush(gradient);
        painter.setPen(pen);
        painter.drawLine(prev, pt);

        gradient.setStart(next);
        pen.setBrush(gradient);
        painter.setPen(pen);
        painter.drawLine(next, pt);

        // Visualize the highlighted vertex.
        QPointF const screen_vertex(to_screen.map(m_ptrNearestVertex->point()));
        m_visualizer.drawVertex(painter, screen_vertex, m_visualizer.highlightBrightColor());
    } else if (interaction.proximityLeader(m_segmentProximity)) {
        QLineF const line(to_screen.map(m_nearestSegment.toLine()));

        // Draw the highlighted edge in orange.
        QPen pen(painter.pen());
        pen.setColor(m_visualizer.highlightDarkColor());
        painter.setPen(pen);
        painter.drawLine(line);

        m_visualizer.drawVertex(painter, m_screenPointOnSegment, m_visualizer.highlightBrightColor());
    } else if (!interaction.captured()) {
        m_visualizer.drawVertex(painter, m_screenMousePos, m_visualizer.solidColor());
    }
}

void
ZoneDefaultInteraction::onProximityUpdate(QPointF const& mouse_pos, InteractionState& interaction)
{
    onProximityUpdate(mouse_pos, interaction, ShiftStateUnknown);
}

void
ZoneDefaultInteraction::onProximityUpdate(QPointF const& mouse_pos, InteractionState& interaction, ShiftState shiftState)
{
    m_screenMousePos = mouse_pos;

    QTransform const to_screen(m_rContext.imageView().imageToWidget());
    QTransform const from_screen(m_rContext.imageView().widgetToImage());
    QPointF const image_mouse_pos(from_screen.map(mouse_pos));

    m_ptrNearestVertex.reset();
    m_ptrNearestVertexSpline.reset();
    m_nearestSegment = SplineSegment();
    m_ptrNearestSegmentSpline.reset();

    Proximity best_vertex_proximity;
    Proximity best_segment_proximity;
    Proximity best_ellipse_center_proximity;
    Proximity best_ellipse_covertex_proximity;

    m_mouse_over_zone = false;

    for (EditableZoneSet::Zone const& zone : m_rContext.zones()) {
        if (!zone.isEllipse()) {
            QPainterPath path;
            path.setFillRule(Qt::WindingFill);
            path.addPolygon(zone.spline()->toPolygon());
            if (!m_mouse_over_zone && path.contains(image_mouse_pos)) {
                m_ptrNearestZone = zone;
                m_mouse_over_zone = true;
            }

            EditableSpline::Ptr const& spline = zone.spline();

            // Process vertices.
            for (SplineVertex::Ptr vert(spline->firstVertex());
                 vert; vert = vert->next(SplineVertex::NO_LOOP)) {

                Proximity const proximity(mouse_pos, to_screen.map(vert->point()));
                if (proximity < best_vertex_proximity) {
                    m_ptrNearestVertex = vert;
                    m_ptrNearestVertexSpline = spline;
                    best_vertex_proximity = proximity;
                }
            }

            // Process segments.
            for (EditableSpline::SegmentIterator it(*spline); it.hasNext();) {
                SplineSegment const segment(it.next());
                QLineF const line(to_screen.map(segment.toLine()));
                QPointF point_on_segment;
                Proximity const proximity(Proximity::pointAndLineSegment(mouse_pos, line, &point_on_segment));
                if (proximity < best_segment_proximity) {
                    m_nearestSegment = segment;
                    m_ptrNearestSegmentSpline = spline;
                    best_segment_proximity = proximity;
                    m_screenPointOnSegment = point_on_segment;
                }
            }

            interaction.updateProximity(m_vertexProximity, best_vertex_proximity, 1);
            interaction.updateProximity(m_segmentProximity, best_segment_proximity, 0);
        } else {
            bool exactly_on_border;
            Proximity proximity;
            best_segment_proximity = proximity; // max dist
            const EditableEllipse::Ptr& ellipse = zone.ellipse();
            if (!m_mouse_over_zone && ellipse->contains(image_mouse_pos, &exactly_on_border)) {
                m_ptrNearestZone = zone;
                m_ptrNearestEllipse = zone.ellipse();
                m_mouse_over_zone = true;
            }

            int i = 0;
            for (QPointF const & p : ellipse->const_data()) {
                Proximity const proximity(mouse_pos, to_screen.map(p));
                if (proximity < best_vertex_proximity) {
                    m_nearestEllipseVertexId = i;
                    m_ptrNearestEllipse = zone.ellipse();
                    best_vertex_proximity = proximity;
                }
                i++;
            }
            interaction.updateProximity(m_ellipseCoVertexProximity, best_vertex_proximity, 1);
            proximity = Proximity(mouse_pos, to_screen.map(ellipse->center()));
            if (proximity < best_vertex_proximity) {
                m_nearestEllipseVertexId = 5;
                m_ptrNearestEllipse = zone.ellipse();
            }
            best_vertex_proximity = std::min(proximity, best_vertex_proximity);
            interaction.updateProximity(m_ellipseCenterProximity, proximity, 2);
        }

    }

    if (m_mouse_over_zone) {
        Proximity const zone_area_proximity(std::min(best_vertex_proximity, best_segment_proximity));
        interaction.updateProximity(m_zoneAreaProximity, zone_area_proximity, -1, zone_area_proximity);
        if (shiftState == ShiftStatePressed ||
                (shiftState == ShiftStateUnknown && m_lastMovingState == ShiftStatePressed)) {
            m_zoneAreaProximity.setProximityCursor(QCursor(Qt::DragMoveCursor));
        } else if (m_dragCopyModifiersHeld) {
            m_zoneAreaProximity.setProximityCursor(QCursor(Qt::DragCopyCursor));
        } else {
            m_zoneAreaProximity.setProximityCursor(QCursor());
        }
    } else {
        m_zoneAreaProximity.setProximityCursor(QCursor());
    }

    if (shiftState != ShiftStateUnknown) {
        m_lastMovingState = shiftState;
    }
}

void
ZoneDefaultInteraction::onMousePressEvent(QMouseEvent* event, InteractionState& interaction)
{
    if (interaction.captured()) {
        return;
    }

    if (interaction.proximityLeader(m_zoneAreaProximity)) {
        const Qt::KeyboardModifiers mask = event->modifiers();
        if (GlobalStaticSettings::checkModifiersMatch(ZoneMove, mask) ||
                GlobalStaticSettings::checkModifiersMatch(ZoneMoveHorizontally, mask) ||
                GlobalStaticSettings::checkModifiersMatch(ZoneMoveVertically, mask)) {

            // as ZoneMoveHorizontally matches ZoneEllipse by default
            // we could get here with m_ptrNearestVertex == null
            // if ellipse is created in ellipse in rect
            QPointF vertex = m_ptrNearestZone.isEllipse() ? m_ptrNearestZone.ellipse()->center() :
                                                            m_ptrNearestVertex? m_ptrNearestVertex->point() :
                                                                                m_ptrNearestZone.spline()->firstVertex()->point();
            makePeerPreceeder(
                        *m_rContext.createDragInteraction(
                            interaction, m_ptrNearestZone,
                            vertex
                            )
                        );
            delete this;
            event->accept();
            return;
        }

        if (event->button() == Qt::LeftButton && isDragCopyModifiers(mask)) {
            startDragCopy(event, interaction);
            return;
        }
    }

    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (interaction.proximityLeader(m_vertexProximity)) {
        makePeerPreceeder(
            *m_rContext.createVertexDragInteraction(
                interaction, m_ptrNearestVertexSpline, m_ptrNearestVertex
            )
        );
        delete this;
        event->accept();
    } else if (interaction.proximityLeader(m_segmentProximity)) {
        QTransform const from_screen(m_rContext.imageView().widgetToImage());
        SplineVertex::Ptr vertex(m_nearestSegment.splitAt(from_screen.map(m_screenPointOnSegment)));
        makePeerPreceeder(
            *m_rContext.createVertexDragInteraction(
                interaction, m_ptrNearestSegmentSpline, vertex
            )
        );
        delete this;
        event->accept();
    } else if (interaction.proximityLeader(m_ellipseCoVertexProximity) ||
               interaction.proximityLeader(m_ellipseCenterProximity)) {
        makePeerPreceeder(
                    *m_rContext.createEllipseVertexDragInteraction(
                        interaction, m_ptrNearestEllipse, m_nearestEllipseVertexId
                        )
                    );
        delete this;
        event->accept();
    } else if (m_rContext.zoneCreationMode() == ZoneInteractionContext::LASSO &&
               event->modifiers() == Qt::NoModifier) {
        makePeerPreceeder(*m_rContext.createZoneCreationInteraction(interaction, true));
        delete this;
        event->accept();
    }
}

void
ZoneDefaultInteraction::startDragCopy(QMouseEvent* event, InteractionState& interaction)
{
    EditableZoneSet& zones = m_rContext.zones();
    EditableSpline::Ptr new_spline;
    EditableEllipse::Ptr new_ellipse;
    if (m_ptrNearestZone.isEllipse()) {
        new_ellipse.reset(new EditableEllipse(SerializableEllipse(*m_ptrNearestZone.ellipse())));
        zones.addEllipse(new_ellipse, *m_ptrNearestZone.properties());
    } else {
        new_spline.reset(new EditableSpline(SerializableSpline(*m_ptrNearestZone.spline())));
        zones.addSpline(new_spline, *m_ptrNearestZone.properties());
    }

    for (EditableZoneSet::Zone const& zone : zones) {
        if ((new_spline && zone.spline() == new_spline) ||
                (new_ellipse && zone.ellipse() == new_ellipse)) {
            QPointF const vertex = new_ellipse ? new_ellipse->center() : new_spline->firstVertex()->point();
            makePeerPreceeder(*m_rContext.createDragInteraction(interaction, zone, vertex, true));
            delete this;
            event->accept();
            return;
        }
    }
}

void
ZoneDefaultInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    if (!interaction.captured()) {
        return;
    }
    if (!m_dragHandler.isActive() || m_dragWatcher.haveSignificantDrag()) {
        return;
    }

    makePeerPreceeder(*m_rContext.createZoneCreationInteraction(interaction));
    delete this;
    event->accept();
}

void
ZoneDefaultInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& /*interaction*/)
{
    QTransform const to_screen(m_rContext.imageView().imageToWidget());

    m_screenMousePos = to_screen.map(event->pos() + QPointF(0.5, 0.5));
    m_rContext.imageView().update();
    const Qt::KeyboardModifiers mask = event->modifiers();
    const bool moved = GlobalStaticSettings::checkModifiersMatch(ZoneMove, mask) ||
                       GlobalStaticSettings::checkModifiersMatch(ZoneMoveHorizontally, mask) ||
                       GlobalStaticSettings::checkModifiersMatch(ZoneMoveVertically, mask);
    m_lastMovingState = moved ? ShiftStatePressed : ShiftStateUnpressed;
    m_dragCopyModifiersHeld = isDragCopyModifiers(mask);
}

void
ZoneDefaultInteraction::onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction)
{
    event->accept();

    InteractionHandler* cm_interaction = m_rContext.createContextMenuInteraction(interaction);
    if (cm_interaction) {
        makePeerPreceeder(*cm_interaction);
        delete this;
        return;
    }

    m_pasteAction->setEnabled(!LocalClipboard::getInstance()->isEmpty());
    m_defaultMenu.popup(event->globalPos());

}

void
ZoneDefaultInteraction::onKeyPressEvent(QKeyEvent* event, InteractionState& interaction)
{
    const Qt::KeyboardModifiers mask = event->modifiers();
    const bool moved = GlobalStaticSettings::checkModifiersMatch(ZoneMove, mask) ||
                       GlobalStaticSettings::checkModifiersMatch(ZoneMoveHorizontally, mask) ||
                       GlobalStaticSettings::checkModifiersMatch(ZoneMoveVertically, mask);
    bool const copy_held = isDragCopyModifiers(mask);
    if (moved || copy_held != m_dragCopyModifiersHeld) {
        m_dragCopyModifiersHeld = copy_held;
        onProximityUpdate(m_screenMousePos, interaction, moved ? ShiftStatePressed : ShiftStateUnpressed);
    }

    if (GlobalStaticSettings::checkKeysMatch(ZoneDelete, mask, (Qt::Key) event->key())) {
        if (deleteUnderMouse(interaction)) {
            event->accept();
        }
    } else if (m_rContext.processZoneCreationModeKey(event)) {
        updateDefaultStatusTip();
        event->accept();
    }
}

bool
ZoneDefaultInteraction::deleteUnderMouse(InteractionState& interaction)
{
    EditableZoneSet& zones = m_rContext.zones();
    if (interaction.proximityLeader(m_vertexProximity)) {
        // A zone must keep at least 3 vertices.
        if (!m_ptrNearestVertex || !m_ptrNearestVertexSpline ||
                !m_ptrNearestVertexSpline->hasAtLeastSegments(4)) {
            return false;
        }
        m_ptrNearestVertex->remove();
        LocalClipboard::getInstance()->setLastZonePolygon(m_ptrNearestVertexSpline->toPolygon());
    } else if (m_mouse_over_zone) {
        if (m_ptrNearestZone.isEllipse()) {
            EditableEllipse::Ptr const ellipse(m_ptrNearestZone.ellipse());
            zones.removeEllipse(ellipse);
        } else {
            EditableSpline::Ptr const spline(m_ptrNearestZone.spline());
            zones.removeSpline(spline);
        }
    } else {
        return false;
    }

    bool const zone_deleted = !interaction.proximityLeader(m_vertexProximity);
    m_ptrNearestVertex.reset();
    m_ptrNearestVertexSpline.reset();
    m_ptrNearestEllipse.reset();
    m_nearestSegment = SplineSegment();
    m_ptrNearestSegmentSpline.reset();
    m_mouse_over_zone = false;
    interaction.resetProximity();

    zones.commit();
    if (zone_deleted) {
        emit zones.manuallyDeleted();
    }
    m_rContext.imageView().update();
    return true;
}

void
ZoneDefaultInteraction::onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction)
{
    const Qt::KeyboardModifiers mask = event->modifiers();
    const bool moved = GlobalStaticSettings::checkModifiersMatch(ZoneMove, mask) ||
                       GlobalStaticSettings::checkModifiersMatch(ZoneMoveHorizontally, mask) ||
                       GlobalStaticSettings::checkModifiersMatch(ZoneMoveVertically, mask);

    bool const copy_held = isDragCopyModifiers(mask);
    if (moved || copy_held != m_dragCopyModifiersHeld) {
        m_dragCopyModifiersHeld = copy_held;
        onProximityUpdate(m_screenMousePos, interaction, ShiftStateUnpressed);
    }

    if (GlobalStaticSettings::checkKeysMatch(ZonePaste, event->modifiers(), (Qt::Key) event->key())) {
        emit m_pasteAction->triggered();
    }
}
