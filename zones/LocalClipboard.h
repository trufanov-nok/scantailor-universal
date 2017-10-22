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

#ifndef LOCAL_CLIPBOARD_H_
#define LOCAL_CLIPBOARD_H_

#include "SerializableSpline.h"

class LocalClipboard
{
public:
    enum ConentType {None, Spline};

    LocalClipboard(): m_content(None) {}

    void clear()
    {
        clear(m_content);
    }

    const ConentType getConentType() const { return m_content; }
    const QPolygonF& getSpline() const { return m_spline; }
    void setSpline(QPolygonF const& spline)
    {
        clear();
        m_spline = spline;
        m_content = Spline;
    }

    ~LocalClipboard() { clear(); }

    static LocalClipboard* getInstance()
    {
        if (!m_instance) {
            m_instance = new LocalClipboard();
        }
        return m_instance;
    }
private:

    void clear(const ConentType type)
    {
        if (type == Spline) {
            m_spline = QPolygonF();
        }
        m_content = None;
    }

    ConentType m_content;
    QPolygonF m_spline;
    static LocalClipboard* m_instance;
};

#endif
