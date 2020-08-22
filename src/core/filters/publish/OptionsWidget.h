/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2020 Alexander Trufanov <trufanovan@gmail.com>

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

#ifndef PUBLISH_OPTIONSWIDGET_H_
#define PUBLISH_OPTIONSWIDGET_H_

#include "ui_PublishingOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "PageId.h"
#include "IntrusivePtr.h"
#include "PageSelectionAccessor.h"

namespace publish
{

class Settings;

class OptionsWidget :
    public FilterOptionsWidget, private Ui::PublishingOptionsWidget
{
    Q_OBJECT
public:
    OptionsWidget(IntrusivePtr<Settings> const& settings,
                  PageSelectionAccessor const& page_selection_accessor);

    virtual ~OptionsWidget();

    void preUpdateUI(QString const& filename);

    void postUpdateUI();

private:

    IntrusivePtr<Settings> m_ptrSettings;
    PageSelectionAccessor m_pageSelectionAccessor;
    QString m_filename;
};

} // namespace publish

#endif
