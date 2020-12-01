/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef PAGE_LAYOUT_ALIGNMENT_H_
#define PAGE_LAYOUT_ALIGNMENT_H_

#include "settings/ini_keys.h"
#include <iostream>

class QDomDocument;
class QDomElement;
class QString;

class CommandLine;

namespace page_layout
{

class Alignment
{
public:

    static const int maskVertical = 0x00FF;
    static const int maskHorizontal = 0xFF00;

    enum Vertical {
        TOP = 1,
        VCENTER = 1 << 1,
        BOTTOM = 1 << 2,
        VAUTO = 1 << 3,
        VORIGINAL = 1 << 4
    };

    enum Horizontal {
        LEFT = 1 << 8,
        HCENTER = 1 << 9,
        RIGHT = 1 << 10,
        HAUTO = 1 << 11,
        HORIGINAL = 1 << 12
    };

    static const int maskAdvanced = HAUTO | VAUTO | HORIGINAL | VORIGINAL;

    /**
     * \brief Constructs a null alignment.
     */
    Alignment();

    Alignment(Vertical vert, Horizontal hor);

    Alignment(Vertical vert, Horizontal hor, bool is_null, double tolerance);

    Alignment(QDomElement const& el);

    Vertical vertical() const
    {
        return static_cast<Vertical>(m_val & maskVertical);
    }

    void setVertical(Vertical vert)
    {
        m_val = vert | horizontal();
    }

    Horizontal horizontal() const
    {
        return static_cast<Horizontal>(m_val & maskHorizontal);
    }

    void setHorizontal(Horizontal hor)
    {
        m_val = vertical() | hor;
    }

    int compositeAlignment() const
    {
        return m_val;
    }

    void setCompositeAlignment(int val)
    {
        m_val = val;
    }

    bool isNull() const
    {
        return m_isNull;
    }

    void setNull(bool is_null)
    {
        m_isNull = is_null;
    }

    double tolerance() const
    {
        return m_tolerance;
    }

    void setTolerance(double t)
    {
        m_tolerance = t;
    }

    bool operator==(Alignment const& other) const
    {
        return m_val == other.m_val && m_isNull == other.m_isNull;
    }

    bool operator!=(Alignment const& other) const
    {
        return !(*this == other);
    }

    QDomElement toXml(QDomDocument& doc, QString const& name) const;

    void save(QSettings* settings = nullptr) const;
    static Alignment load(QSettings* settings = nullptr);

    static QString getVerboseDescription(const Alignment& alignment);
    static QString getShortDescription(const Alignment& alignment);
    static const QString verticalToStr(Vertical val);
    static const QString horizontalToStr(Horizontal val);
    static Vertical strToVertical(const QString& val);
    static Horizontal strToHorizontal(const QString& val);
private:
    int m_val;
    bool m_isNull;
    double m_tolerance;
};

} // namespace page_layout

#endif
