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

#ifndef LOCAL_CLIPBOARD_H_
#define LOCAL_CLIPBOARD_H_

#include "SerializableSpline.h"
#include "SerializableEllipse.h"
#include "ZoneInteractionContext.h"

class LocalClipboard
{
public:
    enum ZoneType {None, Spline, Ellipse};

    LocalClipboard(): m_copiedZoneType(None), m_lastZoneType(None) {}

    ZoneType copiedZoneType() const { return m_copiedZoneType; }
    bool isEmpty() const { return m_copiedZoneType == None; }

    const QPolygonF& splineInVirtualCoords() const { return m_spline; }
    void setSplineInVirtualCoords(QPolygonF const& spline)
    {
        setCopiedZoneType(Spline);
        m_spline = spline;
    }

    const SerializableEllipse& ellipseInVirtualCoords() const { return m_ellipse; }
    void setEllipseInVirtualCoords(SerializableEllipse const & ellipse)
    {
        setCopiedZoneType(Ellipse);
        m_ellipse = ellipse;
    }

    ~LocalClipboard() { }

    static LocalClipboard* getInstance()
    {
        if (!m_instance) {
            m_instance = new LocalClipboard();
        }
        return m_instance;
    }

    ZoneType lastZoneType() const { return m_lastZoneType; }
    bool lastZoneIsValid() const {
        return !m_lastZonePolygon.isEmpty() || m_lastZoneEllipse.isValid();
    }

    // Last zones are expected in Image coords.
    const QPolygonF& lastZonePolygon() const { return m_lastZonePolygon; }
    void setLastZonePolygon(const QPolygonF& val) {
        setLastZoneType(Spline);
        m_lastZonePolygon = val;
    }

    const SerializableEllipse& lastZoneEllipse() const { return m_lastZoneEllipse; }
    void setLastZoneEllipse(const SerializableEllipse& val) {
        setLastZoneType(Ellipse);
        m_lastZoneEllipse = val;
    }

    void pasteZone(ZoneInteractionContext& context) const;
    void repeatLastZone(ZoneInteractionContext& context, const QPointF &mouse_pos) const;

private:

    void setCopiedZoneType(ZoneType val)
    {
        if (val != m_copiedZoneType) {
            if (m_copiedZoneType == Spline) {
                m_spline = QPolygonF();
            }
            if (m_copiedZoneType == Ellipse) {
                m_ellipse = SerializableEllipse();
            }
            m_copiedZoneType = val;
        }
    }

    void setLastZoneType(ZoneType val)
    {
        if (val != m_lastZoneType) {
            if (m_lastZoneType == Spline) {
                m_lastZonePolygon = QPolygonF();
            }
            if (m_lastZoneType == Ellipse) {
                m_lastZoneEllipse = SerializableEllipse();
            }
            m_lastZoneType = val;
        }
    }


    ZoneType m_copiedZoneType;
    ZoneType m_lastZoneType;

    QPolygonF m_spline;
    SerializableEllipse m_ellipse;

    QPolygonF m_lastZonePolygon;
    SerializableEllipse m_lastZoneEllipse;

    static LocalClipboard* m_instance;
};

#endif
