/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C) 2007-2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "ZoneCreationInteraction.h"
#include "ZoneInteractionContext.h"
#include "EditableZoneSet.h"
#include "ImageViewBase.h"
#include "settings/globalstaticsettings.h"
#include "SerializableSpline.h"
#include "LocalClipboard.h"
#include "settings/globalstaticsettings.h"
#include <QCursor>
#include <QTransform>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QLinearGradient>
#include <Qt>
#include <QLineF>
#include <QDebug>

ZoneCreationInteraction::ZoneCreationInteraction(
    ZoneInteractionContext& context, InteractionState& interaction, bool lasso_started)
    :   m_rContext(context),
        m_dragHandler(context.imageView(), [this](InteractionState const&) { return isDragHandlerPermitted(); }),
        m_dragWatcher(m_dragHandler),
        m_zoomHandler(context.imageView(), [](InteractionState const&) { return true; }),
        m_ptrSpline(new EditableSpline),
        m_ptrEllipse(new EditableEllipse),
        m_initialZoneCreationMode(context.zoneCreationMode()),
        m_leftMouseButtonPressed(lasso_started),
        m_mouseButtonModifiers(Qt::NoModifier),
        m_lassoStrokeVertices(0)
{
    QPointF const screen_mouse_pos(
        m_rContext.imageView().mapFromGlobal(QCursor::pos()) + QPointF(0.5, 0.5)
    );
    QTransform const from_screen(m_rContext.imageView().widgetToImage());
    m_nextVertexImagePos = from_screen.map(screen_mouse_pos);
//begin of modified by monday2000
//Square_Picture_Zones
//added:
    m_nextVertexImagePos_mid1 = m_nextVertexImagePos;
    m_nextVertexImagePos_mid2 = m_nextVertexImagePos;
    m_zoneMode = None;
//end of modified by monday2000

    makeLastFollower(m_dragHandler);
    m_dragHandler.makeFirstFollower(m_dragWatcher);

    makeLastFollower(m_zoomHandler);

    interaction.capture(m_interaction);
    m_ptrSpline->appendVertex(m_nextVertexImagePos);
    m_ptrEllipse->setCenter(m_nextVertexImagePos);

    updateStatusTip();
}

void
ZoneCreationInteraction::onPaint(QPainter& painter, InteractionState const& /*interaction*/)
{
    painter.setWorldMatrixEnabled(false);
    painter.setRenderHint(QPainter::Antialiasing);

    QTransform const to_screen(m_rContext.imageView().imageToWidget());
//    QTransform const from_screen(m_rContext.imageView().widgetToImage());

    m_visualizer.drawZones(painter, to_screen, m_rContext.zones());

    QPen solid_line_pen(m_visualizer.solidColor());
    solid_line_pen.setCosmetic(true);
    solid_line_pen.setWidthF(1.5);

    QLinearGradient gradient; // From inactive to active point.
    gradient.setColorAt(0.0, m_visualizer.solidColor());
    gradient.setColorAt(1.0, m_visualizer.highlightDarkColor());

    QPen gradient_pen;
    gradient_pen.setCosmetic(true);
    gradient_pen.setWidthF(1.5);

    painter.setPen(solid_line_pen);
    painter.setBrush(Qt::NoBrush);

//begin of modified by monday2000
//Square_Picture_Zones
//added:

    QColor start_color = m_visualizer.solidColor();
    QColor stop_color = m_visualizer.highlightDarkColor();
    QColor mid_color;

    int red = (start_color.red() + stop_color.red()) / 2;
    int green = (start_color.green() + stop_color.green()) / 2;
    int blue = (start_color.blue() + stop_color.blue()) / 2;

    mid_color.setRgb(red, green, blue);

    QLinearGradient gradient_mid1;
    gradient_mid1.setColorAt(0.0, start_color);
    gradient_mid1.setColorAt(1.0, mid_color);

    QLinearGradient gradient_mid2;
    gradient_mid2.setColorAt(0.0, mid_color);
    gradient_mid2.setColorAt(1.0, stop_color);

    if (m_nextVertexImagePos != m_ptrEllipse->center() && (m_zoneMode == Ellipse)) {

        QPointF to_screen_center = to_screen.map(m_ptrEllipse->center());
        QPointF to_screen_nextVertexImagePos = to_screen.map(m_ptrEllipse->const_data()[0]);
        qreal r = std::sqrt(std::pow(to_screen_center.x() - to_screen_nextVertexImagePos.x(), 2) +
                      std::pow(to_screen_center.y() - to_screen_nextVertexImagePos.y(), 2));

        painter.drawEllipse(to_screen_center, r, r);

        m_visualizer.drawVertex(
            painter, to_screen.map(m_ptrEllipse->center()), m_visualizer.highlightBrightColor()
        );

        for (const QPointF  &p: m_ptrEllipse->const_data()) {
            m_visualizer.drawVertex(
                        painter, to_screen.map(p), m_visualizer.highlightBrightColor()
                        );
        }

    } else
    if (m_nextVertexImagePos != m_nextVertexImagePos_mid1 &&
               m_nextVertexImagePos != m_nextVertexImagePos_mid2 && (m_zoneMode == Rectangle)) {
        m_visualizer.drawVertex(
            painter, to_screen.map(m_nextVertexImagePos_mid1), m_visualizer.highlightBrightColor()
        );

        m_visualizer.drawVertex(
            painter, to_screen.map(m_nextVertexImagePos_mid2), m_visualizer.highlightBrightColor()
        );

        QLineF const line1_mid1(
            to_screen.map(QLineF(m_ptrSpline->lastVertex()->point(), m_nextVertexImagePos_mid1))
        );
        gradient_mid1.setStart(line1_mid1.p1());
        gradient_mid1.setFinalStop(line1_mid1.p2());
        gradient_pen.setBrush(gradient_mid1);
        painter.setPen(gradient_pen);
        painter.drawLine(line1_mid1);

        QLineF const line2_mid1(
            to_screen.map(QLineF(m_nextVertexImagePos_mid1, m_nextVertexImagePos))
        );
        gradient_mid2.setStart(line2_mid1.p1());
        gradient_mid2.setFinalStop(line2_mid1.p2());
        gradient_pen.setBrush(gradient_mid2);
        painter.setPen(gradient_pen);
        painter.drawLine(line2_mid1);

        QLineF const line1_mid2(
            to_screen.map(QLineF(m_ptrSpline->lastVertex()->point(), m_nextVertexImagePos_mid2))
        );
        gradient_mid1.setStart(line1_mid2.p1());
        gradient_mid1.setFinalStop(line1_mid2.p2());
        gradient_pen.setBrush(gradient_mid1);
        painter.setPen(gradient_pen);
        painter.drawLine(line1_mid2);

        QLineF const line2_mid2(
            to_screen.map(QLineF(m_nextVertexImagePos_mid2, m_nextVertexImagePos))
        );
        gradient_mid2.setStart(line2_mid2.p1());
        gradient_mid2.setFinalStop(line2_mid2.p2());
        gradient_pen.setBrush(gradient_mid2);
        painter.setPen(gradient_pen);
        painter.drawLine(line2_mid2);
    } else {
//end of modified by monday2000

        for (EditableSpline::SegmentIterator it(*m_ptrSpline); it.hasNext();) {
            SplineSegment const segment(it.next());
            QLineF const line(to_screen.map(segment.toLine()));

            if (segment.prev == m_ptrSpline->firstVertex() &&
                    segment.prev->point() == m_nextVertexImagePos) {
                gradient.setStart(line.p2());
                gradient.setFinalStop(line.p1());
                gradient_pen.setBrush(gradient);
                painter.setPen(gradient_pen);
                painter.drawLine(line);
                painter.setPen(solid_line_pen);
            } else {
                painter.drawLine(line);
            }
        }

        QLineF const line(
            to_screen.map(QLineF(m_ptrSpline->lastVertex()->point(), m_nextVertexImagePos))
        );
        gradient.setStart(line.p1());
        gradient.setFinalStop(line.p2());
        gradient_pen.setBrush(gradient);
        painter.setPen(gradient_pen);
        painter.drawLine(line);

//begin of modified by monday2000
//Square_Picture_Zones
//added:
    }
//end of modified by monday2000

    m_visualizer.drawVertex(
        painter, to_screen.map(m_nextVertexImagePos), m_visualizer.highlightBrightColor()
    );
}

void
ZoneCreationInteraction::onKeyPressEvent(QKeyEvent* event, InteractionState& /*interaction*/)
{
    if (GlobalStaticSettings::checkKeysMatch(ZoneCancel, event->modifiers(), (Qt::Key) event->key())) {
        makePeerPreceeder(*m_rContext.createDefaultInteraction());
        m_rContext.imageView().update();
        delete this;
        event->accept();
    } else if (!m_leftMouseButtonPressed && m_rContext.processZoneCreationModeKey(event)) {
        updateStatusTip();
        m_rContext.imageView().update();
        event->accept();
    }
}

void
ZoneCreationInteraction::onMousePressEvent(QMouseEvent* event, InteractionState&)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    m_leftMouseButtonPressed = true;
    m_mouseButtonModifiers = event->modifiers();
    m_lassoStrokeVertices = 0;
}

void
ZoneCreationInteraction::onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    bool const lasso_stroke_done = isLassoDrawing() && m_lassoStrokeVertices > 0;
    m_leftMouseButtonPressed = false;
    if (lasso_stroke_done) {
        // The user continues the zone with clicks or with a new lasso stroke.
        event->accept();
        return;
    }

    if (m_dragWatcher.haveSignificantDrag()) {
        return;
    }

//    QTransform const to_screen(m_rContext.imageView().imageToWidget());
    QTransform const from_screen(m_rContext.imageView().widgetToImage());
    QPointF const screen_mouse_pos(event->pos() + QPointF(0.5, 0.5));
    QPointF const image_mouse_pos(from_screen.map(screen_mouse_pos));


    if (m_nextVertexImagePos != m_ptrEllipse->center() && (m_zoneMode == Ellipse)) {

        updateStatusTip();
        m_rContext.zones().addEllipse(m_ptrEllipse);
        LocalClipboard::getInstance()->setLastZoneEllipse(SerializableEllipse(*m_ptrEllipse));
        m_rContext.zones().commit();

        makePeerPreceeder(*m_rContext.createDefaultInteraction());
        m_rContext.imageView().update();
        delete this;

    } else

//begin of modified by monday2000
//Square_Picture_Zones
//added:

    if (m_nextVertexImagePos != m_nextVertexImagePos_mid1 &&
            m_nextVertexImagePos != m_nextVertexImagePos_mid2 && (m_zoneMode == Rectangle) &&
            m_ptrSpline->segmentsCount() == 0 &&
            m_nextVertexImagePos != m_ptrSpline->firstVertex()->point()) {
        m_ptrSpline->appendVertex(m_nextVertexImagePos_mid1);
        m_ptrSpline->appendVertex(image_mouse_pos);
        m_ptrSpline->appendVertex(m_nextVertexImagePos_mid2);
        updateStatusTip();

        // Finishing the spline.  Bridging the first and the last points
        // will create another segment.
        m_ptrSpline->setBridged(true);
        m_ptrSpline->simplify(GlobalStaticSettings::m_zone_editor_min_angle);
        m_rContext.zones().addSpline(m_ptrSpline);
        LocalClipboard::getInstance()->setLastZonePolygon(m_ptrSpline->toPolygon());
        m_rContext.zones().commit();

        makePeerPreceeder(*m_rContext.createDefaultInteraction());
        m_rContext.imageView().update();
        delete this;
    } else {
//end of modified by monday2000

        if (currentZoneCreationMode() == ZoneInteractionContext::RECTANGULAR &&
                m_nextVertexImagePos != m_ptrSpline->firstVertex()->point()) {
            // A rectangle with zero width or height is not valid, so wait for the next click.
            event->accept();
            return;
        }

        if (m_ptrSpline->hasAtLeastSegments(2) &&
                m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
            finishSpline();
            return;
        } else if (m_nextVertexImagePos == m_ptrSpline->lastVertex()->point()) {
            // Removing the last vertex.
            m_ptrSpline->lastVertex()->remove();
            if (!m_ptrSpline->firstVertex()) {
                // If it was the only vertex, cancelling spline creation.
                makePeerPreceeder(*m_rContext.createDefaultInteraction());
                m_rContext.imageView().update();
                delete this;
            }
        } else {
            // Adding a new vertex, provided we are not to close to the previous one.
            Proximity const prox(screen_mouse_pos, m_ptrSpline->lastVertex()->point());
            if (prox > interaction.proximityThreshold()) {
                m_ptrSpline->appendVertex(image_mouse_pos);
                m_ptrSpline->simplify(GlobalStaticSettings::m_zone_editor_min_angle);
                updateStatusTip();
            }
        }

    }

    event->accept();
}

void
ZoneCreationInteraction::onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction)
{
    QPointF const screen_mouse_pos(event->pos() + QPointF(0.5, 0.5));
    QTransform const to_screen(m_rContext.imageView().imageToWidget());
    QTransform const from_screen(m_rContext.imageView().widgetToImage());

    if (GlobalStaticSettings::checkModifiersMatch(ZoneEllipse, event->modifiers())) {
        if (m_ptrSpline->segmentsCount() == 0) {
            m_zoneMode = Ellipse;
            m_nextVertexImagePos = from_screen.map(screen_mouse_pos);
            m_ptrEllipse->changePoint(0, m_nextVertexImagePos, true);
        }
    } else {
        m_nextVertexImagePos = from_screen.map(screen_mouse_pos);
        QPointF const last(to_screen.map(m_ptrSpline->lastVertex()->point()));

//begin of modified by monday2000
//Square_Picture_Zones

        if (GlobalStaticSettings::checkModifiersMatch(ZoneRectangle, event->modifiers()) ||
                currentZoneCreationMode() == ZoneInteractionContext::RECTANGULAR) {
            if (m_ptrSpline->segmentsCount() == 0) {
                m_zoneMode = Rectangle;

                QPointF screen_mouse_pos_mid1;
                screen_mouse_pos_mid1.setX(last.x());
                screen_mouse_pos_mid1.setY(screen_mouse_pos.y());

                QPointF screen_mouse_pos_mid2;
                screen_mouse_pos_mid2.setX(screen_mouse_pos.x());
                screen_mouse_pos_mid2.setY(last.y());

                int dx = screen_mouse_pos.x() - last.x();
                int dy = screen_mouse_pos.y() - last.y();

                if ((dx > 0 && dy > 0) || (dx < 0 && dy < 0)) {
                    m_nextVertexImagePos_mid1 = from_screen.map(screen_mouse_pos_mid1);
                    m_nextVertexImagePos_mid2 = from_screen.map(screen_mouse_pos_mid2);
                } else {
                    m_nextVertexImagePos_mid2 = from_screen.map(screen_mouse_pos_mid1);
                    m_nextVertexImagePos_mid1 = from_screen.map(screen_mouse_pos_mid2);
                }
            }
        }
        //end of modified by monday2000

        if (Proximity(last, screen_mouse_pos) <= interaction.proximityThreshold()) {
            m_nextVertexImagePos = m_ptrSpline->lastVertex()->point();
        } else if (m_ptrSpline->hasAtLeastSegments(2)) {
            QPointF const first(to_screen.map(m_ptrSpline->firstVertex()->point()));
            if (Proximity(first, screen_mouse_pos) <= interaction.proximityThreshold()) {
                m_nextVertexImagePos = m_ptrSpline->firstVertex()->point();
                updateStatusTip();
            }
        }

        if (isLassoDrawing()) {
            QPointF const first(to_screen.map(m_ptrSpline->firstVertex()->point()));
            Proximity const min_distance(interaction.proximityThreshold());
            if (Proximity(last, screen_mouse_pos) > min_distance &&
                    Proximity(first, screen_mouse_pos) > min_distance) {
                m_ptrSpline->appendVertex(m_nextVertexImagePos);
                ++m_lassoStrokeVertices;
                updateStatusTip();
            } else if (m_ptrSpline->hasAtLeastSegments(2) &&
                       m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
                finishSpline();
                return;
            }
        }
    }

    m_rContext.imageView().update();
}

void
ZoneCreationInteraction::onMouseDoubleClickEvent(QMouseEvent* event, InteractionState& /*interaction*/)
{
    if (GlobalStaticSettings::checkModifiersMatch(ZoneClone, event->modifiers()) &&
            LocalClipboard::getInstance()->lastZoneIsValid()) {
        LocalClipboard::getInstance()->repeatLastZone(m_rContext, event->localPos());
    }
}

void
ZoneCreationInteraction::updateStatusTip()
{
    QString tip;

    if (currentZoneCreationMode() == ZoneInteractionContext::RECTANGULAR) {
        tip = tr("Click to finish this rectangular zone. %1 to cancel.")
              .arg(GlobalStaticSettings::getShortcutText(ZoneCancel));
    } else if (m_ptrSpline->hasAtLeastSegments(2)) {
        if (m_nextVertexImagePos == m_ptrSpline->firstVertex()->point()) {
            tip = tr("Click to finish this zone. %1 to cancel.")
                  .arg(GlobalStaticSettings::getShortcutText(ZoneCancel));
        } else {
            tip = tr("Connect first and last points to finish this zone. %1 to cancel.")
                  .arg(GlobalStaticSettings::getShortcutText(ZoneCancel));
        }
    } else if (currentZoneCreationMode() == ZoneInteractionContext::LASSO) {
        tip = tr("Hold the left mouse button and move the mouse to draw the zone. "
                 "Press P for polygonal mode. %1 to cancel.")
              .arg(GlobalStaticSettings::getShortcutText(ZoneCancel));
    } else {
        tip = tr("Zones need to have at least 3 points. Hold %2 for rectangle, %3 for ellipse. %1 to cancel.")
              .arg(GlobalStaticSettings::getShortcutText(ZoneCancel),
              GlobalStaticSettings::getShortcutText(ZoneRectangle),
              GlobalStaticSettings::getShortcutText(ZoneEllipse));
        if (m_initialZoneCreationMode != ZoneInteractionContext::RECTANGULAR) {
            tip.append(" ").append(tr("Press L for lasso mode."));
        }
    }

    m_interaction.setInteractionStatusTip(tip);
}

ZoneInteractionContext::ZoneCreationMode
ZoneCreationInteraction::currentZoneCreationMode() const
{
    // A zone that started as a rectangle stays a rectangle, but the other modes can change.
    if (m_initialZoneCreationMode == ZoneInteractionContext::RECTANGULAR) {
        return ZoneInteractionContext::RECTANGULAR;
    }
    if (m_rContext.zoneCreationMode() == ZoneInteractionContext::LASSO) {
        return ZoneInteractionContext::LASSO;
    }
    return ZoneInteractionContext::POLYGONAL;
}

bool
ZoneCreationInteraction::isLassoDrawing() const
{
    return currentZoneCreationMode() == ZoneInteractionContext::LASSO &&
           m_leftMouseButtonPressed && m_mouseButtonModifiers == Qt::NoModifier;
}

bool
ZoneCreationInteraction::isDragHandlerPermitted() const
{
    return !isLassoDrawing();
}

void
ZoneCreationInteraction::finishSpline()
{
    // A bridge between the first and the last points creates another segment.
    m_ptrSpline->setBridged(true);
    m_ptrSpline->simplify(GlobalStaticSettings::m_zone_editor_min_angle);
    m_rContext.zones().addSpline(m_ptrSpline);
    LocalClipboard::getInstance()->setLastZonePolygon(m_ptrSpline->toPolygon());
    m_rContext.zones().commit();

    makePeerPreceeder(*m_rContext.createDefaultInteraction());
    m_rContext.imageView().update();
    delete this;
}
