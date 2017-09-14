/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>
    Copyright (C)  Vadim Kuznetsov ()DikBSD <dikbsd@gmail.com>

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

#include "OrderBySourceColor.h"
#include "OrderBySourceColor.h.moc"
#include "Params.h"
#include "PageSequence.h"
#include <QSizeF>
#include <memory>
#include <assert.h>

namespace output
{

PageSequence all_pages;
bool metadata_invalidated = true;

OrderBySourceColor::OrderBySourceColor(IntrusivePtr<Settings> const& settings, const IntrusivePtr<ProjectPages> &pages)
:	m_ptrSettings(settings), m_pages(pages)
{
    connect(&*m_pages, SIGNAL(modified()),
            this, SLOT(invalidate_metadata()));
}

bool
OrderBySourceColor::precedes(
    PageId const& lhs_page, bool const /*lhs_incomplete*/,
    PageId const& rhs_page, bool const /*rhs_incomplete*/) const
{
    if (metadata_invalidated)
    {
        all_pages = m_pages->toPageSequence(PAGE_VIEW);
        metadata_invalidated = false;
    }

    const bool gs1 = all_pages.pageAt(lhs_page).metadata().isGrayScale();
    const bool gs2 = all_pages.pageAt(rhs_page).metadata().isGrayScale();

    if (gs1 != gs2)
        return gs1 > gs2;
    else
        return lhs_page < rhs_page;

}

void
OrderBySourceColor::invalidate_metadata()
{
    metadata_invalidated = true;
}

} // namespace page_split
