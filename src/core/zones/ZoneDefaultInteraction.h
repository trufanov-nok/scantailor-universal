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

#ifndef ZONE_DEFAULT_INTERACTION_H_
#define ZONE_DEFAULT_INTERACTION_H_

#include "InteractionHandler.h"
#include "InteractionState.h"
#include "DragHandler.h"
#include "DragWatcher.h"
#include "BasicSplineVisualizer.h"
#include "EditableZoneSet.h"
#include "EditableSpline.h"
#include "SplineVertex.h"
#include "SplineSegment.h"
#include <QPointF>
#include <QMenu>
#include <QCoreApplication>

class ZoneInteractionContext;

class ZoneDefaultInteraction : public InteractionHandler
{
    enum ShiftState {ShiftStatePressed, ShiftStateUnpressed, ShiftStateUnknown};

    Q_DECLARE_TR_FUNCTIONS(ZoneDefaultInteraction)
public:
    ZoneDefaultInteraction(ZoneInteractionContext& context);
protected:
    ZoneInteractionContext& context()
    {
        return m_rContext;
    }

    virtual void onPaint(QPainter& painter, InteractionState const& interaction) override;

    virtual void onProximityUpdate(QPointF const& mouse_pos, InteractionState& interaction) override;

    virtual void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

    virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

    virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

    virtual void onContextMenuEvent(QContextMenuEvent* event, InteractionState& interaction) override;

    virtual void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;

    virtual void onKeyReleaseEvent(QKeyEvent* event, InteractionState& interaction) override;
private:
    void onProximityUpdate(QPointF const& mouse_pos, InteractionState& interaction, ShiftState forseShiftHold);
private:
    ZoneInteractionContext& m_rContext;
    BasicSplineVisualizer m_visualizer;
    InteractionState::Captor m_vertexProximity;
    InteractionState::Captor m_segmentProximity;
    InteractionState::Captor m_ellipseCenterProximity;
    InteractionState::Captor m_ellipseCoVertexProximity;
    InteractionState::Captor m_zoneAreaProximity;
    QPointF m_screenMousePos;

    /**
     * We want our own drag handler, to be able to monitor it
     * and decide if we should go into zone creation state
     * after the left mouse button is released.
     */
    DragHandler m_dragHandler;

    /**
     * Because we hold an interaction state from constructor to destructor,
     * we have to have our own zoom handler with explicit interaction permission
     * if we want zoom to work.
     */
    DragWatcher m_dragWatcher;

    // These are valid if m_vertexProximity is the proximity leader.
    SplineVertex::Ptr m_ptrNearestVertex;
    EditableSpline::Ptr m_ptrNearestVertexSpline;
    EditableZoneSet::Zone m_ptrNearestZone;
    EditableEllipse::Ptr m_ptrNearestEllipse;
    bool m_mouse_over_zone;

    // These are valid if m_segmentProximity is the proximity leader.
    SplineSegment m_nearestSegment;
    EditableSpline::Ptr m_ptrNearestSegmentSpline;
    QPointF m_screenPointOnSegment;
    ShiftState m_lastMovingState;
    QMenu m_defaultMenu;
    QAction* m_pasteAction;

    // These are valid if m_ellipseCoVertexProximity is the proximity leader.
    int m_nearestEllipseVertexId;
};

#endif
