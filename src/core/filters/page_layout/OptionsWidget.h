/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef PAGE_LAYOUT_OPTIONSWIDGET_H_
#define PAGE_LAYOUT_OPTIONSWIDGET_H_

#include "ui/ui_PageLayoutOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "PageSelectionAccessor.h"
#include "IntrusivePtr.h"
#include "Margins.h"
#include "Alignment.h"
#include "PageId.h"
#include "ApplySettingsWidget.h"
#include "ApplyToDialog.h"
#include <QIcon>
#include <memory>
#include <map>
#include <set>

class QToolButton;
class ProjectPages;

namespace page_layout
{

class Settings;

class OptionsWidget :
    public FilterOptionsWidget,
    public Ui::PageLayoutOptionsWidget
{
    Q_OBJECT
public:
    OptionsWidget(
        IntrusivePtr<Settings> const& settings,
        PageSelectionAccessor const& page_selection_accessor);

    virtual ~OptionsWidget();

    void preUpdateUI(PageId const& page_id,
                     const MarginsWithAuto& margins_mm, Alignment const& alignment);

    void postUpdateUI();

    bool leftRightLinked() const
    {
        return m_leftRightLinked;
    }

    bool topBottomLinked() const
    {
        return m_topBottomLinked;
    }

    Margins const& marginsMM() const
    {
        return m_marginsMM;
    }

    Alignment const& alignment() const
    {
        return m_alignment;
    }
signals:
    void alignmentChanged(Alignment const& alignment);
    void marginsSetLocally(Margins const& margins_mm);
    void aggregateHardSizeChanged();
    void topBottomLinkToggled(bool);
    void leftRightLinkToggled(bool);

public slots:
    void marginsSetExternally(const Margins& margins_mm, bool keep_auto_margins = false);
private slots:
    void unitsChanged(int idx);

    void horMarginsChanged(double val);

    void vertMarginsChanged(double val);

    void autoMarginsChanged(bool checked);

    void alignmentChangedExt();

    void topBottomLinkClicked();

    void leftRightLinkClicked();

    void showApplyMarginsDialog();

    void showApplyAlignmentDialog();

    void toBeRemoved(const std::set<PageId> pages);

private:

    void updateMarginsDisplay();

    void updateLinkDisplay(QToolButton* button, bool linked);

    void displayAlignmentText();

    void showApplyDialog(const ApplySettingsWidget::DialogType dlgType);

    void applyMargins(const ApplySettingsWidget::MarginsApplyType type, std::set<PageId> const& pages);

    void applyAlignment(std::set<PageId> const& pages);

    IntrusivePtr<Settings> m_ptrSettings;
    PageSelectionAccessor m_pageSelectionAccessor;
    QIcon m_chainIcon;
    QIcon m_brokenChainIcon;
    double m_mmToUnit;
    double m_unitToMM;
    PageId m_pageId;
    MarginsWithAuto m_marginsMM;
    Alignment m_alignment;
    int m_ignoreMarginChanges;
    bool m_leftRightLinked;
    bool m_topBottomLinked;
};

} // namespace page_layout

#endif
