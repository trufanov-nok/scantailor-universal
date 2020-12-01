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

#include "ApplySettingsWidget.h"

#include "PageSelectionAccessor.h"
#include <QButtonGroup>
#include "settings/ini_keys.h"
#include <assert.h>

namespace page_layout
{

ApplySettingsWidget::ApplySettingsWidget(QWidget* parent,
        const DialogType dlg_type, bool const is_auto_margin_enabled)
    :   QWidget(parent),
        m_dlgType(dlg_type),
        m_empty(true)
{
    setupUi(this);

    if (m_dlgType == Margins) {
        const bool auto_margins_enabled = QSettings().value(_key_margins_auto_margins_enabled, _key_margins_auto_margins_enabled_def).toBool();
        groupBoxWhatToAppy->setVisible(auto_margins_enabled);
        autoMarginRB->setChecked(auto_margins_enabled && is_auto_margin_enabled);
        m_empty = !auto_margins_enabled;
    } else {
        groupBoxWhatToAppy->setVisible(false);
    }

}

ApplySettingsWidget::~ApplySettingsWidget()
{
}

} // namespace page_layout
