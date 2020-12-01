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

#ifndef ZONE_ELLIPSE_VERTEX_DRAG_INTERACTION_H_
#define ZONE_ELLIPSE_VERTEX_DRAG_INTERACTION_H_

#include "BasicSplineVisualizer.h"
#include "EditableEllipse.h"
#include "InteractionHandler.h"
#include "InteractionState.h"
#include <QPointF>
#include <QCoreApplication>

class ZoneInteractionContext;

class ZoneEllipseVertexDragInteraction : public InteractionHandler
{
    Q_DECLARE_TR_FUNCTIONS(ZoneEllipseVertexDragInteraction)
public:
    ZoneEllipseVertexDragInteraction(ZoneInteractionContext& context, InteractionState& interaction,
        EditableEllipse::Ptr const& ellipse, int vertexId);
protected:
    virtual void onPaint(QPainter& painter, InteractionState const& interaction) override;
    virtual void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;
    virtual void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;
private:

    ZoneInteractionContext& m_rContext;
    EditableEllipse::Ptr m_ptrEllipse;
    InteractionState::Captor m_interaction;
    BasicSplineVisualizer m_visualizer;
    QPointF m_dragOffset;
    int m_vertexId;
};

#endif
