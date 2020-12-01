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

#include "OrderBySizeProvider.h"
#include "Params.h"
#include "PageId.h"
#include "Utils.h"
#include <QSizeF>
#include <memory>
#include <assert.h>

namespace select_content
{

OrderBySizeProvider::OrderBySizeProvider(IntrusivePtr<Settings> const& settings, bool byHeight, bool isLogical)
    :   m_ptrSettings(settings), m_byHeight(byHeight), m_isLogical(isLogical)
{
}

qreal
OrderBySizeProvider::adjustByDpi(qreal val, std::unique_ptr<Params> const& params,
                                 StatusLabelPhysSizeDisplayMode mode, int* dpi_used) const
{
    if (!m_isLogical) {
        return val;
    }

    Dpi const dpi = params.get() ? params->origDpi() : select_content::dafaultDpi;
    qreal const d = m_byHeight ? dpi.vertical() : dpi.horizontal();

    if (dpi_used) {
        *dpi_used = d;
    }

    return Utils::adjustByDpiAndUnits(val, d, mode);
}

bool
OrderBySizeProvider::precedes(
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

    QSizeF lhs_size;
    if (lhs_params.get()) {
        lhs_size = lhs_params->contentRect().size();
    }
    QSizeF rhs_size;
    if (rhs_params.get()) {
        rhs_size = rhs_params->contentRect().size();
    }

    bool const lhs_invalid = !lhs_size.isValid();
    bool const rhs_invalid = !rhs_size.isValid();

    if (lhs_invalid != rhs_invalid) {
        // Pages with question mark go to the bottom.
        return rhs_invalid;
    } else if (lhs_invalid) {
        assert(rhs_invalid);
        // Two pages with question marks are ordered naturally.
        return lhs_page < rhs_page;
    }

    assert(lhs_invalid == false);
    assert(rhs_incomplete == false);

    qreal rv = m_byHeight ? rhs_size.height() : rhs_size.width();
    qreal lv = m_byHeight ? lhs_size.height() : lhs_size.width();
    rv = adjustByDpi(rv, rhs_params, StatusBarProvider::statusLabelPhysSizeDisplayMode);
    lv = adjustByDpi(lv, lhs_params, StatusBarProvider::statusLabelPhysSizeDisplayMode);

    if (lv != rv) {
        return lv < rv;
    } else {
        return lhs_page < rhs_page;
    }
}

QString _unknown = QObject::tr("?");

QString
OrderBySizeProvider::hint(PageId const& page) const
{
    std::unique_ptr<Params> const params(m_ptrSettings->getPageParams(page));
    QSizeF size;
    if (params.get()) {
        size = params->contentRect().size();
    }

    QString res = m_byHeight ? QObject::tr("height: %1") : QObject::tr("width: %1");
    qreal val = m_byHeight ? size.height() : size.width();

    if (!m_isLogical) {
        return size.isValid() ? res.arg(val) : res.arg(_unknown);
    } else {
        StatusLabelPhysSizeDisplayMode units_mode = StatusBarProvider::statusLabelPhysSizeDisplayMode;
        if (units_mode == StatusLabelPhysSizeDisplayMode::Pixels) {
            units_mode = StatusLabelPhysSizeDisplayMode::SM;
        }

        int dpi_used = 0;
        val = round(adjustByDpi(val, params, units_mode, &dpi_used) * 100) / 100;

        return size.isValid() ? QObject::tr("%1 %2 (%3 dpi)").arg(val)
               .arg(StatusBarProvider::getStatusLabelPhysSizeDisplayModeSuffix(units_mode))
               .arg(dpi_used)
               : res.arg(_unknown);
    }
}

} // namespace select_content
