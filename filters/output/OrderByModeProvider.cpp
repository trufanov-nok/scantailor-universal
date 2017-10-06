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

#include "OrderByModeProvider.h"
#include "Params.h"
#include <QSizeF>
#include <memory>
#include <assert.h>

namespace output
{

OrderByModeProvider::OrderByModeProvider(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings)
{
}

bool
OrderByModeProvider::precedes(
    PageId const& lhs_page, bool const /*lhs_incomplete*/,
    PageId const& rhs_page, bool const /*rhs_incomplete*/) const
{
    ColorParams const lclr_param(m_ptrSettings->getParams(lhs_page).colorParams());
    ColorParams const rclr_param(m_ptrSettings->getParams(rhs_page).colorParams());

    if (lclr_param.colorMode() != rclr_param.colorMode()) {
        return lclr_param.colorMode() < rclr_param.colorMode();
    }

    const ColorGrayscaleOptions& cgoptsl = lclr_param.colorGrayscaleOptions();
    const ColorGrayscaleOptions& cgoptsr = rclr_param.colorGrayscaleOptions();

    if (cgoptsl.autoLayerEnabled() != cgoptsr.autoLayerEnabled()) {
        return cgoptsl.autoLayerEnabled() < cgoptsr.autoLayerEnabled();
    }
    if (cgoptsl.colorLayerEnabled() != cgoptsr.colorLayerEnabled()) {
        return cgoptsl.colorLayerEnabled() < cgoptsr.colorLayerEnabled();
    }
    if (cgoptsl.normalizeIllumination() != cgoptsr.normalizeIllumination()) {
        return cgoptsl.normalizeIllumination() < cgoptsr.normalizeIllumination();
    }

    if (lclr_param.blackWhiteOptions().thresholdAdjustment() == rclr_param.blackWhiteOptions().thresholdAdjustment()) {
        return lhs_page < rhs_page;
    } else {
        return lclr_param.blackWhiteOptions().thresholdAdjustment() < rclr_param.blackWhiteOptions().thresholdAdjustment();
    }

}

} // namespace page_split
