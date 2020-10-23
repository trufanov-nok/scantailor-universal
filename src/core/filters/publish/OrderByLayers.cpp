/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2021 Alexander Trufanov <trufanovan@gmail.com>

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

#include "OrderByLayers.h"
#include <QObject>

namespace publish
{

OrderByLayers::OrderByLayers(ExportSuggestions const& exp)
:	m_ExportSuggestions(exp)
{
}

bool
OrderByLayers::precedes(
    PageId const& lhs_page, bool const lhs_incomplete,
    PageId const& rhs_page, bool const rhs_incomplete) const
{
    if (lhs_incomplete != rhs_incomplete) {
        // Pages with question mark go to the bottom.
        return rhs_incomplete;
    }

    const ExportSuggestion& l_es = m_ExportSuggestions[lhs_page];
    const ExportSuggestion& r_es = m_ExportSuggestions[rhs_page];

    int lval = l_es.hasBWLayer ? 1 : 0;
    int rval = r_es.hasBWLayer ? 1 : 0;

    if (l_es.hasColorLayer) {
        lval |= 2;
    }
    if (r_es.hasColorLayer) {
        rval |= 2;
    }

    if (lval != rval) {
        return lval < rval;
    }

    return lhs_page < rhs_page;
}


QString
OrderByLayers::hint(PageId const& page) const
{
    const ExportSuggestion& es = m_ExportSuggestions[page];
    QString val;
    if (es.hasBWLayer && es.hasColorLayer) {
        val = QObject::tr("bw + clr");
    } else if (es.hasBWLayer) {
        val = QObject::tr("bw");
    } else if (es.hasColorLayer) {
        val = QObject::tr("clr");
    } else {
        val = QObject::tr("none");
    }

    return QObject::tr("Layers: %1").arg(val);
}

} // namespace publish
