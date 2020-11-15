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

#include "SplitModeWidget.h"

#include "PageSelectionAccessor.h"
#include <QPixmap>
#include <QButtonGroup>
#include "settings/ini_keys.h"
#include <assert.h>
#include <iostream>

namespace page_split
{

SplitModeWidget::SplitModeWidget(
    QWidget* const parent,
    LayoutType const layout_type,
    PageLayout::Type const auto_detected_layout_type,
    bool const auto_detected_layout_type_valid)
    :   QWidget(parent),
        m_layoutType(layout_type),
        m_autoDetectedLayoutType(auto_detected_layout_type),
        m_autoDetectedLayoutTypeValid(auto_detected_layout_type_valid)
{
    setupUi(this);
    layoutTypeLabel->setPixmap(QPixmap(iconFor(m_layoutType)));
    if (m_layoutType == AUTO_LAYOUT_TYPE && !m_autoDetectedLayoutTypeValid) {
        modeAuto->setChecked(true);
    } else {
        modeManual->setChecked(true);
    }

    connect(modeAuto, SIGNAL(pressed()), this, SLOT(autoDetectionSelected()));
    connect(modeManual, SIGNAL(pressed()), this, SLOT(manualModeSelected()));
    QSettings settings;
    if (!settings.value(_key_page_split_apply_cut_enabled, _key_page_split_apply_cut_enabled_def).toBool()) {
        optionsBox->setVisible(false);
        applyCutOption->setChecked(false);
    } else if (modeAuto->isChecked()) {
        applyCutOption->setChecked(false);
        applyCutOption->setDisabled(true);
    } else {
        applyCutOption->setChecked(settings.value(_key_page_split_apply_cut_default, _key_page_split_apply_cut_default_def).toBool());
    }

}

SplitModeWidget::~SplitModeWidget()
{
}

bool
SplitModeWidget::isApplyCutChecked() const
{
    return applyCutOption->isChecked();
}

void
SplitModeWidget::autoDetectionSelected()
{
    layoutTypeLabel->setPixmap(QPixmap(":/icons/layout_type_auto.png"));
    applyCutOption->setChecked(false);
    applyCutOption->setDisabled(true);
}

void
SplitModeWidget::manualModeSelected()
{
    char const* resource = iconFor(combinedLayoutType());
    layoutTypeLabel->setPixmap(QPixmap(resource));
    applyCutOption->setDisabled(false);
}

LayoutType
SplitModeWidget::combinedLayoutType() const
{
    if (m_layoutType != AUTO_LAYOUT_TYPE) {
        return m_layoutType;
    }

    switch (m_autoDetectedLayoutType) {
    case PageLayout::SINGLE_PAGE_UNCUT:
        return SINGLE_PAGE_UNCUT;
    case PageLayout::SINGLE_PAGE_CUT:
        return PAGE_PLUS_OFFCUT;
    case PageLayout::TWO_PAGES:
        return TWO_PAGES;
    }

    assert(!"Unreachable");
    return AUTO_LAYOUT_TYPE;
}

char const*
SplitModeWidget::iconFor(LayoutType const layout_type)
{
    char const* resource = "";

    switch (layout_type) {
    case AUTO_LAYOUT_TYPE:
        resource = ":/icons/layout_type_auto.png";
        break;
    case SINGLE_PAGE_UNCUT:
        resource = ":/icons/single_page_uncut_selected.png";
        break;
    case PAGE_PLUS_OFFCUT:
        resource = ":/icons/right_page_plus_offcut_selected.png";
        break;
    case TWO_PAGES:
        resource = ":/icons/two_pages_selected.png";
        break;
    }

    return resource;
}

} // namespace page_split
