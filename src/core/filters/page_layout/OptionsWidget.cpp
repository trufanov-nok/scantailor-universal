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

#include "Settings.h"
#include "../../Utils.h"
#include "ScopedIncDec.h"
#include "PageInfo.h"
#include "PageId.h"
#include "imageproc/Constants.h"
#include "alignmentwidget.h"
#include <QPixmap>
#include <QString>
#include "settings/ini_keys.h"
#include <QVariant>
#include <assert.h>

using namespace imageproc::constants;

namespace page_layout
{

OptionsWidget::OptionsWidget(
    IntrusivePtr<Settings> const& settings,
    PageSelectionAccessor const& page_selection_accessor)
    :   m_ptrSettings(settings),
        m_pageSelectionAccessor(page_selection_accessor),
        m_mmToUnit(1.0),
        m_unitToMM(1.0),
        m_ignoreMarginChanges(0),
        m_leftRightLinked(true),
        m_topBottomLinked(true)
{

    connect(&m_pageSelectionAccessor, &PageSelectionAccessor::toBeRemoved,
            this, &OptionsWidget::toBeRemoved, Qt::DirectConnection);

    {
        QSettings app_settings;
        m_leftRightLinked = app_settings.value(_key_margins_linked_hor, _key_margins_linked_hor_def).toBool();
        m_topBottomLinked = app_settings.value(_key_margins_linked_ver, _key_margins_linked_ver_def).toBool();
    }

    m_chainIcon.addPixmap(
        QPixmap(QLatin1String(":/icons/stock-vchain-24.png"))
    );
    m_brokenChainIcon.addPixmap(
        QPixmap(QLatin1String(":/icons/stock-vchain-broken-24.png"))
    );

    setupUi(this);
    updateLinkDisplay(topBottomLink, m_topBottomLinked);
    updateLinkDisplay(leftRightLink, m_leftRightLinked);

    connect(
        unitsComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(unitsChanged(int))
    );
    connect(
        topMarginSpinBox, SIGNAL(valueChanged(double)),
        this, SLOT(vertMarginsChanged(double))
    );
    connect(
        bottomMarginSpinBox, SIGNAL(valueChanged(double)),
        this, SLOT(vertMarginsChanged(double))
    );
    connect(
        leftMarginSpinBox, SIGNAL(valueChanged(double)),
        this, SLOT(horMarginsChanged(double))
    );
    connect(
        rightMarginSpinBox, SIGNAL(valueChanged(double)),
        this, SLOT(horMarginsChanged(double))
    );
    connect(
        autoMargins, SIGNAL(toggled(bool)),
        this, SLOT(autoMarginsChanged(bool))
    );
    connect(
        topBottomLink, SIGNAL(clicked()),
        this, SLOT(topBottomLinkClicked())
    );
    connect(
        leftRightLink, SIGNAL(clicked()),
        this, SLOT(leftRightLinkClicked())
    );
    connect(
        applyMarginsBtn, SIGNAL(clicked()),
        this, SLOT(showApplyMarginsDialog())
    );
    connect(
        applyAlignmentBtn, SIGNAL(clicked()),
        this, SLOT(showApplyAlignmentDialog())
    );

    unitsComboBox->setCurrentIndex(QSettings().value(_key_margins_default_units, _key_margins_default_units_def).toUInt());

    connect(widgetAlignment, &AlignmentWidget::alignmentChanged, this, &OptionsWidget::alignmentChangedExt);
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(
    PageId const& page_id, MarginsWithAuto const& margins_mm, Alignment const& alignment)
{
    m_pageId = page_id;
    m_marginsMM = margins_mm;
    m_alignment = alignment;

    bool old_ignore = m_ignoreMarginChanges;
    m_ignoreMarginChanges = true;

    updateMarginsDisplay();

    m_ignoreMarginChanges = old_ignore;

    const bool auto_margins_enabled = QSettings().value(_key_margins_auto_margins_enabled, _key_margins_auto_margins_enabled_def).toBool();
    autoMarginsLayout_2->setVisible(auto_margins_enabled);
    autoMargins->setChecked(auto_margins_enabled &&
                            m_marginsMM.isAutoMarginsEnabled());

    m_ignoreMarginChanges = true;

    QSettings setting;
    widgetAlignment->setUseAutoMagnetAlignment(setting.value(_key_alignment_automagnet_enabled, _key_alignment_automagnet_enabled_def).toBool());
    widgetAlignment->setUseOriginalProportionsAlignment(setting.value(_key_alignment_original_enabled, _key_alignment_original_enabled_def).toBool());
    widgetAlignment->setAlignment(&m_alignment);

    displayAlignmentText();

    m_leftRightLinked = m_leftRightLinked && (margins_mm.left() == margins_mm.right());
    m_topBottomLinked = m_topBottomLinked && (margins_mm.top() == margins_mm.bottom());
    updateLinkDisplay(topBottomLink, m_topBottomLinked);
    updateLinkDisplay(leftRightLink, m_leftRightLinked);

    marginsGroup->setEnabled(false);
    alignmentGroup->setEnabled(false);

    m_ignoreMarginChanges = old_ignore;
}

void
OptionsWidget::postUpdateUI()
{
    marginsGroup->setEnabled(true);
    alignmentGroup->setEnabled(true);
}

void
OptionsWidget::marginsSetExternally(Margins const& margins_mm, bool keep_auto_margins)
{
    if (!(static_cast<Margins const&>(m_marginsMM) == margins_mm)) {
        if (!keep_auto_margins && m_marginsMM.isAutoMarginsEnabled()) {
            m_marginsMM.setAutoMargins(false);
        }
        m_marginsMM = margins_mm;
    }
    updateMarginsDisplay();
}

void
OptionsWidget::unitsChanged(int const idx)
{
    ScopedIncDec<int> const ingore_scope(m_ignoreMarginChanges);

    int decimals = 0;
    double step = 0.0;

    if (idx == 0) { // mm
        m_mmToUnit = 1.0;
        m_unitToMM = 1.0;
        decimals = 1;
        step = 1.0;
    } else { // in
        m_mmToUnit = MM2INCH;
        m_unitToMM = INCH2MM;
        decimals = 2;
        step = 0.01;
    }

    topMarginSpinBox->setDecimals(decimals);
    topMarginSpinBox->setSingleStep(step);
    bottomMarginSpinBox->setDecimals(decimals);
    bottomMarginSpinBox->setSingleStep(step);
    leftMarginSpinBox->setDecimals(decimals);
    leftMarginSpinBox->setSingleStep(step);
    rightMarginSpinBox->setDecimals(decimals);
    rightMarginSpinBox->setSingleStep(step);

    updateMarginsDisplay();
}

void
OptionsWidget::horMarginsChanged(double const val)
{
    if (m_ignoreMarginChanges) {
        return;
    }

    if (m_leftRightLinked  && !m_marginsMM.isAutoMarginsEnabled()) {
        ScopedIncDec<int> const ingore_scope(m_ignoreMarginChanges);
        leftMarginSpinBox->setValue(val);
        rightMarginSpinBox->setValue(val);
    }

    m_marginsMM.setLeft(leftMarginSpinBox->value() * m_unitToMM);
    m_marginsMM.setRight(rightMarginSpinBox->value() * m_unitToMM);

    emit marginsSetLocally(static_cast<Margins>(m_marginsMM));
}

void
OptionsWidget::vertMarginsChanged(double const val)
{
    if (m_ignoreMarginChanges) {
        return;
    }

    if (m_topBottomLinked && !m_marginsMM.isAutoMarginsEnabled()) {
        ScopedIncDec<int> const ingore_scope(m_ignoreMarginChanges);
        topMarginSpinBox->setValue(val);
        bottomMarginSpinBox->setValue(val);
    }

    m_marginsMM.setTop(topMarginSpinBox->value() * m_unitToMM);
    m_marginsMM.setBottom(bottomMarginSpinBox->value() * m_unitToMM);

    emit marginsSetLocally(static_cast<Margins>(m_marginsMM));
}

void
OptionsWidget::topBottomLinkClicked()
{
    m_topBottomLinked = !m_topBottomLinked;
    QSettings().setValue(_key_margins_linked_ver, m_topBottomLinked);
    updateLinkDisplay(topBottomLink, m_topBottomLinked);
    emit this->topBottomLinkToggled(m_topBottomLinked);
}

void
OptionsWidget::leftRightLinkClicked()
{
    m_leftRightLinked = !m_leftRightLinked;
    QSettings().setValue(_key_margins_linked_hor, m_leftRightLinked);
    updateLinkDisplay(leftRightLink, m_leftRightLinked);
    emit this->leftRightLinkToggled(m_leftRightLinked);
}

void
OptionsWidget::autoMarginsChanged(bool checked)
{
    if (m_ignoreMarginChanges) {
        return;
    }

    m_marginsMM.setAutoMargins(checked);
    m_ptrSettings->setHardMarginsMM(m_pageId, m_marginsMM);
    if (checked) {
        emit reloadRequested();
    } else {
        updateMarginsDisplay();
        emit marginsSetLocally(static_cast<Margins>(m_marginsMM));
    }
}

void
OptionsWidget::alignmentChangedExt()
{
    displayAlignmentText();
    emit alignmentChanged(m_alignment);
}

void
OptionsWidget::showApplyDialog(const ApplySettingsWidget::DialogType dlgType)
{
    ApplyToDialog* dialog = new ApplyToDialog(
        this, m_pageId, m_pageSelectionAccessor
    );

    dialog->setWindowTitle((dlgType == ApplySettingsWidget::Margins) ?
                           tr("Apply Margins") :
                           tr("Apply Alignment"));

    ApplySettingsWidget* options = new ApplySettingsWidget(dialog, dlgType,
            m_marginsMM.isAutoMarginsEnabled());
    const bool empty_options = options->isEmpty();
    if (!empty_options) {
        QLayout& l = dialog->initNewLeftSettingsPanel();
        l.addWidget(options);
    } else {
        delete options;
    }

    connect(
        dialog, &ApplyToDialog::accepted,
    this, [ = ]() {

        std::vector<PageId> vec = dialog->getPageRangeSelectorWidget().result();
        std::set<PageId> pages(vec.begin(), vec.end());
        if (dlgType == ApplySettingsWidget::Margins) {
            if (!empty_options) {
                applyMargins(options->getMarginsTypeVal(), pages);
            } else {
                applyMargins(ApplySettingsWidget::MarginsValues, pages);
            }
        } else {
            applyAlignment(pages);
        }

    }
    );

    dialog->show();
}

void
OptionsWidget::showApplyMarginsDialog()
{
    showApplyDialog(ApplySettingsWidget::Margins);
}

void
OptionsWidget::showApplyAlignmentDialog()
{
    showApplyDialog(ApplySettingsWidget::Alignment);
}

void
OptionsWidget::applyMargins(ApplySettingsWidget::MarginsApplyType const type, std::set<PageId> const& pages)
{
    if (pages.empty()) {
        return;
    }

    if (!QSettings().value(_key_margins_auto_margins_enabled, _key_margins_auto_margins_enabled_def).toBool()) {
        for (PageId const& page_id : pages) {
            m_ptrSettings->setHardMarginsMM(page_id, m_marginsMM);
        }
    } else if (type == ApplySettingsWidget::MarginsValues) {
        const Margins& val = static_cast<Margins&>(m_marginsMM);
        for (PageId const& page_id : pages) {
            MarginsWithAuto mh =  m_ptrSettings->getHardMarginsMM(page_id);
            if (!mh.isAutoMarginsEnabled()) {
                static_cast<Margins&>(mh) = val;
                m_ptrSettings->setHardMarginsMM(page_id, mh);
            }
        }
    } else { //type == ApplyDialog::AutoMarginState
        const bool val = m_marginsMM.isAutoMarginsEnabled();
        for (PageId const& page_id : pages) {
            MarginsWithAuto mh =  m_ptrSettings->getHardMarginsMM(page_id);
            if (mh.isAutoMarginsEnabled() != val) {
                mh.setAutoMargins(val);
                m_ptrSettings->setHardMarginsMM(page_id, mh);
            }
        }
    }

    emit aggregateHardSizeChanged();
    emit invalidateAllThumbnails();
}

void
OptionsWidget::applyAlignment(std::set<PageId> const& pages)
{
    if (pages.empty()) {
        return;
    }

    for (PageId const& page_id : pages) {
        m_ptrSettings->setPageAlignment(page_id, m_alignment);
    }

    emit invalidateAllThumbnails();
}

void
OptionsWidget::updateMarginsDisplay()
{
    ScopedIncDec<int> const ignore_scope(m_ignoreMarginChanges);

    topMarginSpinBox->setValue(m_marginsMM.top() * m_mmToUnit);
    bottomMarginSpinBox->setValue(m_marginsMM.bottom() * m_mmToUnit);
    leftMarginSpinBox->setValue(m_marginsMM.left() * m_mmToUnit);
    rightMarginSpinBox->setValue(m_marginsMM.right() * m_mmToUnit);

    panelMarginsControls->setEnabled(!m_marginsMM.isAutoMarginsEnabled());
    autoMargins->setChecked(m_marginsMM.isAutoMarginsEnabled());
}

void
OptionsWidget::updateLinkDisplay(QToolButton* button, bool const linked)
{
    button->setIcon(linked ? m_chainIcon : m_brokenChainIcon);
}

void OptionsWidget::displayAlignmentText()
{
    QString res = Alignment::getVerboseDescription(m_alignment);
    if (res.isEmpty()) {
        lblCurrentAlignment->setText(tr("No alignment needed."));
        return;
    }

    res = tr("Alignment: %1").arg(res);
    lblCurrentAlignment->setText(res);
}

void OptionsWidget::toBeRemoved(const std::set<PageId> pages)
{
    // This supposed to keep aggregate hard margins size updated
    // In case user has removed pages with biggest sizes from project
    m_ptrSettings->removePages(pages);
    emit invalidateAllThumbnails();
}

} // namespace page_layout
