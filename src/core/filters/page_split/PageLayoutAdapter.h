/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
    Backported from ScanTailor Advanced (src/core/filters/page_split/PageLayoutAdapter.h).

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

#ifndef PAGE_SPLIT_PAGE_LAYOUT_ADAPTER_H_
#define PAGE_SPLIT_PAGE_LAYOUT_ADAPTER_H_

#include "PageLayout.h"
#include <QLineF>
#include <QRectF>
#include <QVector>

namespace page_split
{

class Params;
class Dependencies;

class PageLayoutAdapter
{
public:
    // Moves the cutters of a layout to another outline, so that a cut can go to a page of a different size.
    static PageLayout adaptPageLayout(PageLayout const& layout, QRectF const& outline);

    // Changes an invalid cut, for example crossed cutters or a split line on the page edge, to an uncut page.
    static void correctPageLayoutType(PageLayout* layout);

    // Returns true if the params differ from deps only in the image size, and then fits the cut to the outline.
    static bool adaptParams(Params& params, Dependencies const& deps, QRectF const& outline);
private:
    // The result goes from the top edge (p1) to the bottom edge (p2) of the new outline.
    static QLineF adaptCutter(QLineF const& cutter, QRectF const& new_rect);

    static QVector<QLineF> adaptCutters(QVector<QLineF> const& cutters, QRectF const& new_rect);
};

} // namespace page_split

#endif
