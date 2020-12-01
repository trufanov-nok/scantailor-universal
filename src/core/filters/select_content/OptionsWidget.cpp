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

#include "OptionsWidget.h"

#include "ApplyToDialog.h"
#include "Settings.h"
#include "Params.h"
#include "ScopedIncDec.h"

#include "settings/ini_keys.h"
#include <iostream>

namespace select_content
{

OptionsWidget::OptionsWidget(
    IntrusivePtr<Settings> const& settings,
    PageSelectionAccessor const& page_selection_accessor)
    :   m_ptrSettings(settings),
        m_pageSelectionAccessor(page_selection_accessor),
        m_ignoreAutoManualToggle(0)
{
    setupUi(this);

    connect(autoBtn, SIGNAL(pressed()), this, SLOT(autoMode()));
    connect(manualBtn, SIGNAL(pressed()), this, SLOT(manualMode()));
    connect(disableBtn, SIGNAL(pressed()), this, SLOT(contentDetectionDisabled()));
    connect(pageDetectAutoBtn, SIGNAL(pressed()), this, SLOT(pageDetectionEnabled()));
    connect(pageDetectDisableBtn, SIGNAL(pressed()), this, SLOT(pageDetectionDisabled()));
    connect(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
    connect(fineTuneBtn, SIGNAL(toggled(bool)), this, SLOT(fineTuningChanged(bool)));

    connect(leftBorder, SIGNAL(valueChanged(double)), this, SLOT(borderChanged()));
    connect(rightBorder, SIGNAL(valueChanged(double)), this, SLOT(borderChanged()));
    connect(topBorder, SIGNAL(valueChanged(double)), this, SLOT(borderChanged()));
    connect(bottomBorder, SIGNAL(valueChanged(double)), this, SLOT(borderChanged()));

    m_force_reload = QSettings().value(_key_content_sel_STE_tweak, _key_content_sel_STE_tweak_def).toBool();
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
    ScopedIncDec<int> guard(m_ignoreAutoManualToggle);

    m_pageId = page_id;
    autoBtn->setChecked(true);
    autoBtn->setEnabled(false);
    manualBtn->setEnabled(false);
    disableBtn->setEnabled(false);
    pageDetectAutoBtn->setEnabled(false);
    pageDetectDisableBtn->setEnabled(false);

    QSettings setting;
    const bool page_detection_enabled = setting.value(_key_content_sel_page_detection_enabled, _key_content_sel_page_detection_enabled_def).toBool();

    bool fine_tune_corners_enabled = page_detection_enabled;
    if (fine_tune_corners_enabled) {
        fine_tune_corners_enabled = setting.value(_key_content_sel_page_detection_fine_tune_corners, _key_content_sel_page_detection_fine_tune_corners_def).toBool();
    }

    panelFineTuneCorners->setVisible(fine_tune_corners_enabled);
    if (!fine_tune_corners_enabled) {
        fineTuneBtn->setChecked(false);
    }

    bool border_panel_visible = page_detection_enabled;
    if (border_panel_visible) {
        border_panel_visible = setting.value(_key_content_sel_page_detection_borders, _key_content_sel_page_detection_borders_def).toBool();
    }

    gbBordersPanel->setVisible(border_panel_visible);

    gbPageBox->setVisible(page_detection_enabled);

}

void
OptionsWidget::postUpdateUI(UiData const& ui_data)
{
    Margins m = ui_data.pageBorders();

    m_uiData = ui_data;

    leftBorder->setValue(m.left());
    topBorder->setValue(m.top());
    rightBorder->setValue(m.right());
    bottomBorder->setValue(m.bottom());

    updateModeIndication(ui_data.mode());
    if (panelFineTuneCorners->isVisible()) {
        fineTuneBtn->setChecked(ui_data.fineTuning());
        fineTuneBtn->setEnabled(true);
    }
    pageDetectAutoBtn->setChecked(ui_data.pageDetection());
    pageDetectDisableBtn->setChecked(!ui_data.pageDetection());
    autoBtn->setEnabled(true);
    manualBtn->setEnabled(true);
    disableBtn->setEnabled(true);
    pageDetectAutoBtn->setEnabled(true);
    pageDetectDisableBtn->setEnabled(true);

}

void
OptionsWidget::manualContentRectSet(QRectF const& content_rect)
{
    m_uiData.setContentRect(content_rect);
    m_uiData.setMode(MODE_MANUAL);
    m_uiData.setContentDetection(true);

    updateModeIndication(MODE_MANUAL);
    commitCurrentParams();

    emit invalidateThumbnail(m_pageId);
    if (m_force_reload) {
        emit reloadRequested();
    }
}

void
OptionsWidget::modeChanged(bool const auto_mode)
{
    if (m_ignoreAutoManualToggle) {
        return;
    }

    bool need_reload = m_force_reload;

    if (auto_mode) {
        need_reload = true;
        //m_ptrSettings->clearPageParams(m_pageId);
        m_uiData.setMode(MODE_AUTO);
    } else {
        m_uiData.setMode(MODE_MANUAL);
    }

    if (!m_uiData.contentDetection()) {
        m_uiData.setContentDetection(true);
        need_reload = true;
    }

    commitCurrentParams();

    if (need_reload) {
        emit reloadRequested();
    }
}

void OptionsWidget::autoMode()
{
    modeChanged(true);
}

void OptionsWidget::manualMode()
{
    modeChanged(false);
}

void
OptionsWidget::fineTuningChanged(bool checked)
{
    m_uiData.setFineTuneCorners(checked);
    commitCurrentParams();
    if (m_uiData.pageDetection()) {
        emit reloadRequested();
    }
}

void
OptionsWidget::contentDetectionDisabled(void)
{
    bool old = m_ignoreAutoManualToggle;
    m_ignoreAutoManualToggle = true;

    m_uiData.setContentDetection(false);
    commitCurrentParams();
    autoBtn->setChecked(false);
    manualBtn->setChecked(false);
    disableBtn->setChecked(true);
    emit reloadRequested();

    m_ignoreAutoManualToggle = old;
}

void
OptionsWidget::pageDetectionDisabled(void)
{
    m_uiData.setPageDetection(false);
    pageDetectAutoBtn->setChecked(false);
    pageDetectDisableBtn->setChecked(true);
    commitCurrentParams();
    emit reloadRequested();
}

void
OptionsWidget::pageDetectionEnabled(void)
{
    m_uiData.setPageDetection(true);
    pageDetectAutoBtn->setChecked(true);
    pageDetectDisableBtn->setChecked(false);
    commitCurrentParams();
    emit reloadRequested();
}

void OptionsWidget::borderChanged()
{
    m_uiData.setPageBorders(leftBorder->value(), topBorder->value(), rightBorder->value(), bottomBorder->value());
    commitCurrentParams();
    if (!m_uiData.contentRect().isEmpty()) {
        emit reloadRequested();
    }
}

void
OptionsWidget::updateModeIndication(AutoManualMode const mode)
{
    ScopedIncDec<int> guard(m_ignoreAutoManualToggle);

    if (! m_uiData.contentDetection()) {
        disableBtn->setChecked(true);
        autoBtn->setChecked(false);
        manualBtn->setChecked(false);
    } else {
        disableBtn->setChecked(false);
        if (mode == MODE_AUTO) {
            autoBtn->setChecked(true);
            manualBtn->setChecked(false);
        } else {
            autoBtn->setChecked(false);
            manualBtn->setChecked(true);
        }
    }
}

void
OptionsWidget::commitCurrentParams()
{
    Params params(
        m_uiData.contentRect(), m_uiData.contentSizeMM(),
        //m_uiData.dependencies(), m_uiData.mode(), m_uiData.contentDetection(), m_uiData.pageDetection(), m_uiData.fineTuning()
        Dependencies(), m_uiData.mode(), m_uiData.contentDetection(), m_uiData.pageDetection(), m_uiData.fineTuning(),
        m_uiData.origDpi()
    );
    params.setPageRect(m_uiData.pageRect());
    params.setPageBorders(m_uiData.pageBorders());
    params.computeDeviation(m_ptrSettings->avg());
    m_ptrSettings->setPageParams(m_pageId, params);
}

void
OptionsWidget::showApplyToDialog()
{
    ApplyToDialog* dialog = new ApplyToDialog(
        this, m_pageId, m_pageSelectionAccessor
    );

    dialog->setWindowTitle(tr("Select Content"));

    QLayout& l = dialog->initNewTopSettingsPanel();
    QGroupBox* optionsBox = new QGroupBox(tr("Options"), dialog);
    optionsBox->setLayout(new QVBoxLayout);
    QCheckBox* applyContentBoxOption = new QCheckBox(tr("Apply content box"), optionsBox);
    optionsBox->layout()->addWidget(applyContentBoxOption);
    l.addWidget(optionsBox);

    connect(
        dialog, &ApplyToDialog::accepted, this,
    [ = ]() {
        std::vector<PageId> vec = dialog->getPageRangeSelectorWidget().result();
        std::set<PageId> pages(vec.begin(), vec.end());
        applySelection(pages, applyContentBoxOption->isChecked());
    }
    );
    dialog->show();
}

void
OptionsWidget::applySelection(std::set<PageId> const& pages, bool apply_content_box)
{
    if (pages.empty()) {
        return;
    }

    Dependencies deps;
    Params params(
        m_uiData.contentRect(), m_uiData.contentSizeMM(),
        deps, m_uiData.mode(), m_uiData.contentDetection(), m_uiData.pageDetection(), m_uiData.fineTuning()
    );

    for (PageId const& page_id : pages) {
        std::unique_ptr<Params> old_params = m_ptrSettings->getPageParams(page_id);

        if (old_params.get()) {
            if (!apply_content_box) {
                params.setContentRect(old_params->contentRect());
                params.setContentSizeMM(old_params->contentSizeMM());
            }
            params.setPageRect(old_params->pageRect());
        }
        params.setPageBorders(Margins(leftBorder->value(), topBorder->value(), rightBorder->value(), bottomBorder->value()));

        m_ptrSettings->setPageParams(page_id, params);
        //emit invalidateThumbnail(page_id);
    }
    emit invalidateAllThumbnails();
    emit reloadRequested();
}

/*========================= OptionsWidget::UiData ======================*/

OptionsWidget::UiData::UiData()
    :   m_mode(MODE_AUTO),
        m_contentDetection(true),
        m_pageDetection(false),
        m_fineTuneCorners(false),
        m_borders(0, 0, 0, 0)
{
}

OptionsWidget::UiData::~UiData()
{
}

void
OptionsWidget::UiData::setSizeCalc(PhysSizeCalc const& calc)
{
    m_sizeCalc = calc;
}

void
OptionsWidget::UiData::setContentRect(QRectF const& content_rect)
{
    m_contentRect = content_rect;
}

QRectF const&
OptionsWidget::UiData::contentRect() const
{
    return m_contentRect;
}

void
OptionsWidget::UiData::setPageRect(QRectF const& page_rect)
{
    m_pageRect = page_rect;
}

QRectF const&
OptionsWidget::UiData::pageRect() const
{
    return m_pageRect;
}

QSizeF
OptionsWidget::UiData::contentSizeMM() const
{
    return m_sizeCalc.sizeMM(m_contentRect);
}

void
OptionsWidget::UiData::setDependencies(Dependencies const& deps)
{
    m_deps = deps;
}

Dependencies const&
OptionsWidget::UiData::dependencies() const
{
    return m_deps;
}

void
OptionsWidget::UiData::setMode(AutoManualMode const mode)
{
    m_mode = mode;
}

AutoManualMode
OptionsWidget::UiData::mode() const
{
    return m_mode;
}

void
OptionsWidget::UiData::setContentDetection(bool detect)
{
    m_contentDetection = detect;
}

void
OptionsWidget::UiData::setPageDetection(bool detect)
{
    m_pageDetection = detect;
}

void
OptionsWidget::UiData::setFineTuneCorners(bool fine_tune)
{
    m_fineTuneCorners = fine_tune;
}

void OptionsWidget::UiData::setPageBorders(double left, double top, double right, double bottom)
{
    m_borders.setLeft(left);
    m_borders.setTop(top);
    m_borders.setRight(right);
    m_borders.setBottom(bottom);
}

} // namespace select_content
