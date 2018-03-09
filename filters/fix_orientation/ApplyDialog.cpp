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
#include <QDebug>
#include <assert.h>

namespace fix_orientation
{

ApplyDialog::ApplyDialog(
	QWidget* parent,
	PageId const& cur_page, PageSelectionAccessor const& page_selection_accessor):	QDialog(parent)
{
	setupUi(this);
    widgetPageRangeSelector->setData(cur_page, page_selection_accessor, PageView::IMAGE_VIEW);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyDialog::~ApplyDialog()
{
}

void
ApplyDialog::onSubmit()
{
    std::vector<PageId> vec = widgetPageRangeSelector->result();
    std::set<PageId> pages(vec.begin(), vec.end());
    if (!widgetPageRangeSelector->allPagesSelected()) {
        emit appliedTo(pages);
    } else {
        emit appliedToAllPages(pages);
    }
    accept();
}

} // namespace fix_orientation
