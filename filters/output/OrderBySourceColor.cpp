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

#include "Params.h"
#include "PageSequence.h"
#include <QSizeF>
#include <memory>
#include <assert.h>

namespace output
{

PageSequence cached_pages_views;
bool sequence_cached = false;

OrderBySourceColor::OrderBySourceColor(IntrusivePtr<Settings> const& settings, const IntrusivePtr<ProjectPages>& pages)
    :   m_ptrSettings(settings), m_pages(pages)
{
    connect(&*m_pages, SIGNAL(modified()),
            this, SLOT(invalidate_metadata()));
}

bool
OrderBySourceColor::precedes(
    PageId const& lhs_page, bool const /*lhs_incomplete*/,
    PageId const& rhs_page, bool const /*rhs_incomplete*/) const
{
    if (!sequence_cached) {
        cached_pages_views = m_pages->toPageSequence(PAGE_VIEW);
        sequence_cached = true;
    }

    const bool gs1 = cached_pages_views.pageAt(lhs_page).metadata().isGrayScale();
    const bool gs2 = cached_pages_views.pageAt(rhs_page).metadata().isGrayScale();

    if (gs1 != gs2) {
        return gs1 > gs2;
    } else {
        return lhs_page < rhs_page;
    }

}

void
OrderBySourceColor::invalidate_metadata()
{
    sequence_cached = false;
}

QString
OrderBySourceColor::hint(PageId const& page) const
{
    if (!sequence_cached) {
        cached_pages_views = m_pages->toPageSequence(PAGE_VIEW);
        sequence_cached = true;
    }

    bool const gs = cached_pages_views.pageAt(page).metadata().isGrayScale();
    return gs ? QObject::tr("grayscale source") : QObject::tr("color source");
}

} // namespace page_split
