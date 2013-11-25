/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2015  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef SELECT_CONTENT_OPTIONSWIDGET_H_
#define SELECT_CONTENT_OPTIONSWIDGET_H_

#include "ui_SelectContentOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "IntrusivePtr.h"
#include "AutoManualMode.h"
#include "ContentBox.h"
#include "Dependencies.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "Params.h"
#include <boost/optional.hpp>
#include <QSizeF>
#include <QRectF>
#include <memory>

namespace select_content
{

class Settings;

class OptionsWidget :
	public FilterOptionsWidget, private Ui::SelectContentOptionsWidget
{
	Q_OBJECT
public:
	OptionsWidget(IntrusivePtr<Settings> const& settings,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~OptionsWidget();
	
	void preUpdateUI(PageId const& page_id);
	
	void postUpdateUI(Params const& params);
public slots:
	void manualContentBoxSet(
		ContentBox const& content_box, QSizeF const& content_size_px);
private slots:
	void showApplyToDialog();

	void applySelection(std::set<PageId> const& pages);

	void modeChanged(bool auto_mode);
private:
	void updateModeIndication(AutoManualMode const mode);
	
	void commitCurrentParams();
	
	IntrusivePtr<Settings> m_ptrSettings;
	boost::optional<Params> m_params;
	PageSelectionAccessor m_pageSelectionAccessor;
	PageId m_pageId;
	int m_ignoreAutoManualToggle;
};

} // namespace select_content

#endif
