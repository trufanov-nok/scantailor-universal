/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Alexander Trufanov <trufanovan@gmail.com>

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

#include "OrderByPageSizeProvider.h"
#include "Params.h"
#include "PageLayout.h"
#include "Utils.h" // for Utils::adjustByDpiAndUnits
#include <QObject>
#include <assert.h>
#include <cmath>

namespace page_split
{

OrderByPageSizeProvider::OrderByPageSizeProvider(IntrusivePtr<Settings> const& settings)
    :   m_ptrSettings(settings)
{
}

qreal
getMaxPageWidth(page_split::PageLayout const val, qreal dpi, StatusLabelPhysSizeDisplayMode mode)
{
    qreal res = 0.;
    switch (val.type()) {
    case page_split::PageLayout::SINGLE_PAGE_UNCUT:
    case page_split::PageLayout::SINGLE_PAGE_CUT: res = val.singlePageOutline().boundingRect().width();
        break;
    case page_split::PageLayout::TWO_PAGES: res = qMax<qreal>(val.leftPageOutline().boundingRect().width(),
                val.rightPageOutline().boundingRect().width());
    }

    if (dpi != 0. && res != 0.) {
        res = Utils::adjustByDpiAndUnits(res, dpi, mode);
    }

    return res;
}

qreal
getMaxPageWidth(Settings::Record const record)
{
    Params const* params = record.params();
    if (params) {
        const bool keep_in_pixels = StatusBarProvider::statusLabelPhysSizeDisplayMode ==
                                    StatusLabelPhysSizeDisplayMode::Pixels;
        const qreal dpi = (!keep_in_pixels) ? params->origDpi().horizontal() : 0;
        return getMaxPageWidth(params->pageLayout(), dpi,  StatusBarProvider::statusLabelPhysSizeDisplayMode);
    }
    return 0;
}

bool
OrderByPageSizeProvider::precedes(
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

    qreal const lhs_max_page_size = getMaxPageWidth(m_ptrSettings->getPageRecord(lhs_page.imageId()));
    qreal const rhs_max_page_size = getMaxPageWidth(m_ptrSettings->getPageRecord(rhs_page.imageId()));

    if (lhs_max_page_size != rhs_max_page_size) {
        return lhs_max_page_size < rhs_max_page_size;
    } else {
        return lhs_page < rhs_page;
    }
}

QString
OrderByPageSizeProvider::hint(PageId const& page) const
{
    Settings::Record const record(m_ptrSettings->getPageRecord(page.imageId()));
    qreal const max_page_size = getMaxPageWidth(record);

    if (StatusBarProvider::statusLabelPhysSizeDisplayMode ==
            StatusLabelPhysSizeDisplayMode::Pixels) {
        return QObject::tr("max width: %1 px").arg((int) max_page_size);
    }

    Params const* params = record.params();
    return QObject::tr("max width: %1 %2 (%3 dpi)").arg(round(max_page_size * 100) / 100)
           .arg(StatusBarProvider::getStatusLabelPhysSizeDisplayModeSuffix())
           .arg((params) ? params->origDpi().horizontal() : defaultDpi.horizontal());
}

} // namespace page_split
