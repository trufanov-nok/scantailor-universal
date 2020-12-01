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

#ifndef PAGE_LAYOUT_APPLYSETTINGSWIDGET_H_
#define PAGE_LAYOUT_APPLYSETTINGSWIDGET_H_

#include "ui_PageLayoutApplyWidget.h"
#include "PageId.h"
#include "PageRange.h"
#include "PageSequence.h"
#include "IntrusivePtr.h"
#include <QWidget>
#include <set>

class PageSelectionAccessor;
class QButtonGroup;

namespace page_layout
{

class ApplySettingsWidget : public QWidget, private Ui::PageLayoutApplyWidget
{
    Q_OBJECT
public:
    enum DialogType {
        Margins,
        Alignment
    };
    enum MarginsApplyType {
        MarginsValues,
        AutoMarginState
    };
public:
    ApplySettingsWidget(QWidget* parent, const DialogType dlg_type, const bool is_auto_margin_enabled = false);
    ApplySettingsWidget::MarginsApplyType getMarginsTypeVal() const
    {
        return (autoMarginRB->isChecked()) ? AutoMarginState : MarginsValues;
    }
    bool isEmpty() const
    {
        return m_empty;
    }
    virtual ~ApplySettingsWidget();
private:
    DialogType m_dlgType;
    bool m_empty;
};

} // namespace page_layout

#endif
