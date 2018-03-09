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

#include "ChangeDewarpingDialog.h"
#include "ChangeDewarpingDialog.moc"
#include "PageSelectionAccessor.h"
#include "QtSignalForwarder.h"
#ifndef Q_MOC_RUN
#include <boost/function.hpp>
#include <boost/lambda/lambda.hpp>
#endif

namespace output
{

ChangeDewarpingDialog::ChangeDewarpingDialog(
	QWidget* parent, PageId const& cur_page, DewarpingMode const& mode,
	PageSelectionAccessor const& page_selection_accessor)
:	QDialog(parent),
    m_mode(mode)
{
	using namespace boost::lambda;

	ui.setupUi(this);
    ui.widgetPageRangeSelector->setData(cur_page, page_selection_accessor);

	switch (mode) {
		case DewarpingMode::OFF:
			ui.offRB->setChecked(true);
			break;
		case DewarpingMode::AUTO:
			ui.autoRB->setChecked(true);
			break;
//begin of modified by monday2000
//Marginal_Dewarping
		case DewarpingMode::MARGINAL:
			ui.marginalRB->setChecked(true);
			break;
//end of modified by monday2000
		case DewarpingMode::MANUAL:
			ui.manualRB->setChecked(true);
			break;
	}

	// No, we don't leak memory here.
	new QtSignalForwarder(ui.offRB, SIGNAL(clicked(bool)), var(m_mode) = DewarpingMode::OFF);
	new QtSignalForwarder(ui.autoRB, SIGNAL(clicked(bool)), var(m_mode) = DewarpingMode::AUTO);
	new QtSignalForwarder(ui.manualRB, SIGNAL(clicked(bool)), var(m_mode) = DewarpingMode::MANUAL);
//begin of modified by monday2000
//Marginal_Dewarping
	new QtSignalForwarder(ui.marginalRB, SIGNAL(clicked(bool)), var(m_mode) = DewarpingMode::MARGINAL);
//end of modified by monday2000

	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ChangeDewarpingDialog::~ChangeDewarpingDialog()
{
}

void
ChangeDewarpingDialog::onSubmit()
{
    std::vector<PageId> vec = ui.widgetPageRangeSelector->result();
    std::set<PageId> pages(vec.begin(), vec.end());
	
	emit accepted(pages, m_mode);
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

} // namespace output
