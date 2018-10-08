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
    :	m_ptrSettings(settings)
{
}

qreal
getMaxPageWidth(page_split::PageLayout const val, qreal dpi)
{
    qreal res = 0;
    switch (val.type()) {
    case page_split::PageLayout::SINGLE_PAGE_UNCUT:
    case page_split::PageLayout::SINGLE_PAGE_CUT: res = val.singlePageOutline().boundingRect().width();
        break;
    case page_split::PageLayout::TWO_PAGES: res = qMax<qreal>(val.leftPageOutline().boundingRect().width(),
                                                               val.rightPageOutline().boundingRect().width());
    }

    if (res != 0) {
        res = Utils::adjustByDpiAndUnits( res, dpi, StatusBarProvider::statusLabelPhysSizeDisplayMode);
    }

    return res;
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

    Settings::Record const lhs_record(m_ptrSettings->getPageRecord(lhs_page.imageId()));
    Settings::Record const rhs_record(m_ptrSettings->getPageRecord(rhs_page.imageId()));
    Params const* lhs_params = lhs_record.params();
    Params const* rhs_params = rhs_record.params();
    qreal lhs_max_page_size = (lhs_params)? getMaxPageWidth(lhs_params->pageLayout(), lhs_params->origDpi().horizontal()) : 0;
    qreal rhs_max_page_size = (lhs_params)? getMaxPageWidth(rhs_params->pageLayout(), rhs_params->origDpi().horizontal()) : 0;

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
    Params const* params = record.params();
    qreal const max_page_size = ((params)? getMaxPageWidth(params->pageLayout(), params->origDpi().horizontal()) : 0);

    QString res;
    if (StatusBarProvider::statusLabelPhysSizeDisplayMode != StatusLabelPhysSizeDisplayMode::Pixels) {
        res = QObject::tr("max width: %1 %2 (%3 dpi)");
        res = res.arg(round(max_page_size*100)/100)
                .arg(StatusBarProvider::getStatusLabelPhysSizeDisplayModeSuffix())
                .arg((params)?params->origDpi().horizontal() : defaultDpi.horizontal());
    } else {
        res = QObject::tr("max width: %1 px").arg((int) max_page_size);
    }

    return res;
}

} // namespace page_split
