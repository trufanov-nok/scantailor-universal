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

#ifndef ZONE_DRAG_INTERACTION_H_
#define ZONE_DRAG_INTERACTION_H_

#include "BasicSplineVisualizer.h"
#include "EditableSpline.h"
#include "SerializableSpline.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include <QPointF>
#include <QCoreApplication>

class ZoneInteractionContext;

class ZoneDragInteraction : public InteractionHandler
{
    Q_DECLARE_TR_FUNCTIONS(ZoneDragInteraction)
public:
    ZoneDragInteraction(
        ZoneInteractionContext& context, InteractionState& interaction,
        EditableSpline::Ptr const& spline, SplineVertex::Ptr const& vertex);
protected:
    virtual void onPaint(QPainter& painter, InteractionState const& interaction);

    virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction);

    virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction);

    virtual void onKeyPressEvent(QKeyEvent* event, InteractionState& interaction) override;
private:
    void checkProximity(InteractionState const& interaction);

    ZoneInteractionContext& m_rContext;
    EditableSpline::Ptr m_ptrSpline;
    SerializableSpline m_savedSpline;
    SplineVertex::Ptr m_ptrVertex;
    InteractionState::Captor m_interaction;
    BasicSplineVisualizer m_visualizer;
    QPointF m_dragOffset;
    QPointF m_moveStart;
};

#endif
