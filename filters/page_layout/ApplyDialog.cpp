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

#include "ApplyDialog.h"
#include "ApplyDialog.moc"
#include "PageSelectionAccessor.h"
#include <QButtonGroup>
#include "settings/ini_keys.h"
#include <assert.h>

namespace page_layout
{

ApplyDialog::ApplyDialog(QWidget* parent, PageId const& cur_page,
    PageSelectionAccessor const& page_selection_accessor,
    const DialogType dlg_type, bool const is_auto_margin_enabled)
:	QDialog(parent),
    m_dlgType(dlg_type)
{
	setupUi(this);
    widgetPageRangeSelector->setData(cur_page, page_selection_accessor);
	
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));

    if (m_dlgType == Margins) {
        const bool auto_margins_enabled = QSettings().value(_key_margins_auto_margins_enabled, _key_margins_auto_margins_enabled_def).toBool();
        groupBoxWhatToAppy->setVisible(auto_margins_enabled);
        autoMarginRB->setChecked(auto_margins_enabled && is_auto_margin_enabled);
    } else {
        groupBoxWhatToAppy->setVisible(false);
    }

}

ApplyDialog::~ApplyDialog()
{
}

void
ApplyDialog::onSubmit()
{	
    std::vector<PageId> vec = widgetPageRangeSelector->result();
    std::set<PageId> pages(vec.begin(), vec.end());
	
    if (m_dlgType == Margins) {
        emit accepted_margins(marginValuesRB->isChecked()?MarginsValues:AutoMarginState, pages);
    } else {
        emit accepted(pages);
    }
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

} // namespace page_layout
