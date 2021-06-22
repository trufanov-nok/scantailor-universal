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

#ifndef SELECT_CONTENT_ORDER_BY_HEIGHT_PROVIDER_H_
#define SELECT_CONTENT_ORDER_BY_HEIGHT_PROVIDER_H_

#include "Settings.h"
#include "IntrusivePtr.h"
#include "PageOrderProvider.h"
#include "StatusBarProvider.h"

namespace select_content
{
class OrderBySizeProvider : public PageOrderProvider
{
public:
    OrderBySizeProvider(IntrusivePtr<Settings> const& settings, bool byHeight = true, bool isLogical = false);

    virtual bool precedes(
        PageId const& lhs_page, bool lhs_incomplete,
        PageId const& rhs_page, bool rhs_incomplete) const override;

    virtual QString hint(PageId const& page) const override;
    virtual bool hintIsUnitsDependant() const override { return m_isLogical; }
private:
    qreal adjustByDpi(qreal val, std::unique_ptr<Params> const& params,
                      StatusLabelPhysSizeDisplayMode mode = StatusLabelPhysSizeDisplayMode::Inch,
                      int* dpi_used = nullptr) const;
private:
    IntrusivePtr<Settings> m_ptrSettings;
    bool m_byHeight;
    bool m_isLogical;
};

} // namespace select_content

#endif
