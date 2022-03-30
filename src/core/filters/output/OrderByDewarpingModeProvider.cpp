/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>
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

#include "OrderByDewarpingModeProvider.h"
#include "Params.h"
#include <QSizeF>
#include <memory>
#include <assert.h>

namespace output
{

OrderByDewarpingModeProvider::OrderByDewarpingModeProvider(IntrusivePtr<Settings> const& settings)
    :   m_ptrSettings(settings)
{
}

bool
OrderByDewarpingModeProvider::precedes(
    PageId const& lhs_page, bool const /*lhs_incomplete*/,
    PageId const& rhs_page, bool const /*rhs_incomplete*/) const
{
    DewarpingMode const l_param(m_ptrSettings->getParams(lhs_page).dewarpingMode());
    DewarpingMode const r_param(m_ptrSettings->getParams(rhs_page).dewarpingMode());

    if (l_param != r_param) {
        return l_param < r_param;
    }

    return lhs_page < rhs_page;
}

QString dewarpingMode2String(DewarpingMode const mode)
{
    switch (mode) {
    case DewarpingMode::OFF: return QObject::tr("off");
    case DewarpingMode::MANUAL: return QObject::tr("manual");
    case DewarpingMode::MARGINAL: return QObject::tr("marginal");
    case DewarpingMode::AUTO: return QObject::tr("auto");
    default: return QString();
    }
}

QString
OrderByDewarpingModeProvider::hint(PageId const& page) const
{
    DewarpingMode const param(m_ptrSettings->getParams(page).dewarpingMode());
    return dewarpingMode2String(param);
}

} // namespace output
