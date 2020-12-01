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

#ifndef PAGE_SPLIT_SPLITMODEWIDGET_H_
#define PAGE_SPLIT_SPLITMODEWIDGET_H_

#include "ui_PageSplitModeWidget.h"
#include "LayoutType.h"
#include "PageLayout.h"
#include "PageId.h"
#include "PageSequence.h"
#include "PageRange.h"
#include "IntrusivePtr.h"
#include <set>

class ProjectPages;
class PageSelectionAccessor;
class QButtonGroup;

namespace page_split
{

class SplitModeWidget : public QWidget, private Ui::PageSplitModeApplyWidget
{
    Q_OBJECT
public:
    SplitModeWidget(QWidget* parent, LayoutType layout_type, PageLayout::Type auto_detected_layout_type,
                    bool auto_detected_layout_type_valid);

    LayoutType layoutType() const
    {
        LayoutType layout_type = AUTO_LAYOUT_TYPE;
        if (modeManual->isChecked()) {
            layout_type = combinedLayoutType();
        }
        return layout_type;
    }

    bool isApplyCutChecked() const;

    virtual ~SplitModeWidget();
signals:
    void accepted(std::set<PageId> const& pages,
                  bool all_pages, LayoutType layout_type, bool apply_cut);
private slots:
    void autoDetectionSelected();

    void manualModeSelected();
private:
    LayoutType combinedLayoutType() const;

    static char const* iconFor(LayoutType layout_type);

    LayoutType m_layoutType;
    PageLayout::Type m_autoDetectedLayoutType;
    bool m_autoDetectedLayoutTypeValid;
};

} // namespace page_split

#endif
