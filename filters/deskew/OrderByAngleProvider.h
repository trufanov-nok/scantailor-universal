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

#ifndef PAGE_LAYOUT_ORDER_BY_ANGLE_PROVIDER_H_
#define PAGE_LAYOUT_ORDER_BY_ANGLE_PROVIDER_H_

#include "Settings.h"
#include "IntrusivePtr.h"
#include "PageOrderProvider.h"
#include "Params.h"
#include "PageId.h"
#include <assert.h>

namespace deskew
{
template <double (*func)(double)>
class OrderByAngleProviderBase : public PageOrderProvider
{
public:
    OrderByAngleProviderBase(IntrusivePtr<Settings> const& settings): m_ptrSettings(settings) { }

    virtual bool precedes(
        PageId const& lhs_page, bool lhs_incomplete,
        PageId const& rhs_page, bool rhs_incomplete) const
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

        double const lhs_angle = lhs_params.get() ? func(-1.0 * lhs_params->deskewAngle()) : 0.;
        double const rhs_angle = rhs_params.get() ? func(-1.0 * rhs_params->deskewAngle()) : 0.;

        if (lhs_angle != rhs_angle) {
            return lhs_angle < rhs_angle;
        } else {
            return lhs_page < rhs_page;
        }
    }

    virtual QString hint(PageId const& page) const
    {
        std::unique_ptr<Params> const params(m_ptrSettings->getPageParams(page));
        double const angle = params.get() ? func(-1.0 * params->deskewAngle()) : 0.;
        return QObject::tr("angle: %1°").arg(round(angle * 1000) / 1000);
    }

private:
    IntrusivePtr<Settings> m_ptrSettings;
};

double noop(double v)
{
    return v;
}

typedef OrderByAngleProviderBase<noop> OrderByAngleProvider;
typedef OrderByAngleProviderBase<std::abs> OrderByAbsAngleProvider;

} // namespace page_layout

#endif
