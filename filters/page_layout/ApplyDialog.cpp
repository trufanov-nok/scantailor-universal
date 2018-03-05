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
	m_pages(page_selection_accessor.allPages()),
	m_selectedPages(page_selection_accessor.selectedPages()),
	m_selectedRanges(page_selection_accessor.selectedRanges()),
	m_curPage(cur_page),
    m_pScopeGroup(new QButtonGroup(this)),
    m_dlgType(dlg_type)
{
	setupUi(this);
	m_pScopeGroup->addButton(thisPageRB);
	m_pScopeGroup->addButton(allPagesRB);
	m_pScopeGroup->addButton(thisPageAndFollowersRB);
	m_pScopeGroup->addButton(selectedPagesRB);
	m_pScopeGroup->addButton(everyOtherRB);
	m_pScopeGroup->addButton(thisEveryOtherRB);
	m_pScopeGroup->addButton(everyOtherSelectedRB);


	if (m_selectedPages.size() <= 1) {
		selectedPagesWidget->setEnabled(false);
		everyOtherSelectedWidget->setEnabled(false);
        everyOtherSelectedHint->setText(selectedPagesHint->text());
    } else if (m_selectedRanges.size() > 1) {
		everyOtherSelectedWidget->setEnabled(false);
		everyOtherSelectedHint->setText(tr("Can't do: more than one group is selected."));
    }

	
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
	std::set<PageId> pages;
	
	// thisPageRB is intentionally not handled.
	if (allPagesRB->isChecked()) {
		m_pages.selectAll().swap(pages);
	} else if (thisPageAndFollowersRB->isChecked()) {
		m_pages.selectPagePlusFollowers(m_curPage).swap(pages);
	} else if (selectedPagesRB->isChecked()) {
        if (m_dlgType == Margins) {
            emit accepted_margins(marginValuesRB->isChecked()?MarginsValues:AutoMarginState, m_selectedPages);
        } else {
            emit accepted(m_selectedPages);
        }
		accept();
		return;
	} else if (everyOtherRB->isChecked()) {
		m_pages.selectEveryOther(m_curPage).swap(pages);
	} else if (thisEveryOtherRB->isChecked()) {
		std::set<PageId> tmp;
		m_pages.selectPagePlusFollowers(m_curPage).swap(tmp);
		std::set<PageId>::iterator it = tmp.begin();
		for (int i=0; it != tmp.end(); ++it, ++i) {
			if (i % 2 == 0) {
				pages.insert(*it);
			}
		}
	} else if (everyOtherSelectedRB->isChecked()) {
        if (m_selectedRanges.size() == 1) {
            m_selectedRanges.front().selectEveryOther(m_curPage).swap(pages);
        } else {
            std::set<PageId> tmp;
            m_pages.selectEveryOther(m_curPage).swap(tmp);
            for (PageRange const& range: m_selectedRanges) {
                for (PageId const& page: range.pages) {
                    if (tmp.find(page) != tmp.end()) {
                        pages.insert(page);
                    }
                }
            }
        }
	}
	
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
