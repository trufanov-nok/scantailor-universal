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

#include "LocalClipboard.h"
#include "ImageViewBase.h"
#include "settings/globalstaticsettings.h"

LocalClipboard*
LocalClipboard::m_instance = nullptr;

void
LocalClipboard::pasteZone(ZoneInteractionContext& context) const
{
    QTransform virtual_to_image(context.imageView().virtualToImage());
    QTransform shift = QTransform().translate(100, 100);

    if (m_copiedZoneType == LocalClipboard::Spline) {
        QPolygonF new_spline = m_spline;
        new_spline = virtual_to_image.map(new_spline);

        do {
            bool existing = false;
            for (const EditableZoneSet::Zone& zone : context.zones()) {
                if (!zone.isEllipse()) {
                    const QPolygonF z = SerializableSpline(*zone.spline().get()).toPolygon();
                    if (new_spline == z) {
                        // we shouldn't mix SerializableSpline::toPolygon and EditableSpline::toPolygon
                        // as order of vertexes might be different.
                        existing = true;
                        new_spline = shift.map(new_spline);
                        break;
                    }
                }
            }
            if (!existing) {
                context.zones().addSpline(EditableSpline::Ptr(new EditableSpline(SerializableSpline(new_spline))));
                context.zones().commit();
                break;
            }
        } while (true);
    } else if (m_copiedZoneType == LocalClipboard::Ellipse) {
        SerializableEllipse new_ellipse = m_ellipse.transformed(virtual_to_image);
        do {
            bool existing = false;
            for (const EditableZoneSet::Zone& zone : context.zones()) {
                if (zone.isEllipse()) {
                    const SerializableEllipse e(*zone.ellipse());
                    if (e == new_ellipse) {
                        existing = true;
                        EditableEllipse shifted(new_ellipse);
                        shifted.setCenter(shift.map(new_ellipse.center()));
                        new_ellipse = SerializableEllipse(shifted);
                        break;
                    }
                }
            }
            if (!existing) {
                context.zones().addEllipse(EditableEllipse::Ptr(new EditableEllipse(new_ellipse)));
                context.zones().commit();
                break;
            }
        } while (true);

    }

}

void
LocalClipboard::repeatLastZone(ZoneInteractionContext& context, const QPointF &mouse_pos) const
{
    // Paste latest created/changed zone. Middle of zone should be ~ mouse_pos
    // This is useful for mass creation of fill zones.
    QTransform const from_screen(context.imageView().widgetToImage());
    const QPointF pos = from_screen.map(mouse_pos);

    if (m_lastZoneType == LocalClipboard::Spline) {
        QPolygonF new_zone = m_lastZonePolygon;
        QRectF r = new_zone.boundingRect();
        new_zone.translate(pos.x() - r.left() - r.width() / 2,
                           pos.y() - r.top() - r.height() / 2);

        EditableSpline::Ptr spline(new EditableSpline(SerializableSpline(new_zone)));
        spline->simplify(GlobalStaticSettings::m_zone_editor_min_angle);
        context.zones().addSpline(spline);
        context.zones().commit();
    } else if (m_lastZoneType == LocalClipboard::Ellipse) {
        EditableEllipse::Ptr ellipse(new EditableEllipse(m_lastZoneEllipse));
        ellipse->setCenter(pos);
        context.zones().addEllipse(ellipse);
        context.zones().commit();
    }
}
