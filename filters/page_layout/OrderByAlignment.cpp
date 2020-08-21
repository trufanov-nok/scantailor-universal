/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "OrderByAlignment.h"
#include "Params.h"
#include "Margins.h"
#include "PageId.h"
#include <QSizeF>
#include <memory>
#include <assert.h>

namespace page_layout
{

OrderByAlignment::OrderByAlignment(IntrusivePtr<Settings> const& settings)
    :   m_ptrSettings(settings)
{
}

bool
OrderByAlignment::precedes(
    PageId const& lhs_page, bool const lhs_incomplete,
    PageId const& rhs_page, bool const rhs_incomplete) const
{

    if (lhs_incomplete != rhs_incomplete) {
        // Pages with question mark go to the bottom.
        return rhs_incomplete;
    } else if (lhs_incomplete) {
        assert(rhs_incomplete);
        // Two pages with question marks are ordered naturally.
        return lhs_page < rhs_page;
    }

    assert(lhs_incomplete == false);
    assert(rhs_incomplete == false);

    std::unique_ptr<Params> const lhs_params(m_ptrSettings->getPageParams(lhs_page));
    std::unique_ptr<Params> const rhs_params(m_ptrSettings->getPageParams(rhs_page));

    bool lhs_is_null = true;
    int lhs_alignment;
    if (lhs_params.get()) {
        lhs_is_null = lhs_params->alignment().isNull();
        lhs_alignment = lhs_params->alignment().compositeAlignment();
    }

    bool rhs_is_null = true;
    int rhs_alignment;
    if (rhs_params.get()) {
        rhs_is_null = rhs_params->alignment().isNull();
        rhs_alignment = rhs_params->alignment().compositeAlignment();
    }

    if (lhs_is_null != rhs_is_null) {
        // Pages with no alignment go to the top.
        return lhs_is_null;
    } else if (lhs_is_null) {
        assert(rhs_is_null);
        // Two pages with no alignmen are ordered naturally.
        return lhs_page < rhs_page;
    }

    if (lhs_alignment != rhs_alignment) {
        return (!(lhs_alignment < rhs_alignment));
    } else {
        return lhs_page < rhs_page;
    }
}

QString
OrderByAlignment::hint(PageId const& page) const
{
    std::unique_ptr<Params> const params(m_ptrSettings->getPageParams(page));
    if (params.get() && !params->alignment().isNull()) {
        return Alignment::getShortDescription(params->alignment());
    } else {
        return QObject::tr("not defined");
    }
}

} // namespace page_layout
