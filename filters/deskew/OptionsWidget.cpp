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
#include "OptionsWidget.moc"
#include "Settings.h"
#include "ScopedIncDec.h"
#include "ApplyToDialog.h"
#include "DeskewApplyWidget.h"
#include "settings/globalstaticsettings.h"

#include <QString>
#include <Qt>
#include <math.h>

namespace deskew
{

double const OptionsWidget::MAX_ANGLE = 45.0;

OptionsWidget::OptionsWidget(IntrusivePtr<Settings> const& settings,
						PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings),
	m_ignoreAutoManualToggle(0),
	m_ignoreSpinBoxChanges(0),
	m_pageSelectionAccessor(page_selection_accessor)
{
	setupUi(this);
    wgtPageOrientationFix->setVisible(false);
	angleSpinBox->setSuffix(QChar(0x00B0)); // the degree symbol
	angleSpinBox->setRange(-MAX_ANGLE, MAX_ANGLE);
	angleSpinBox->adjustSize();
	setSpinBoxUnknownState();
	
	connect(
		angleSpinBox, SIGNAL(valueChanged(double)),
		this, SLOT(spinBoxValueChanged(double))
	);
	connect(autoBtn, SIGNAL(toggled(bool)), this, SLOT(modeChanged(bool)));
	connect(
		applyDeskewBtn, SIGNAL(clicked()),
		this, SLOT(showDeskewDialog())
	);
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::showDeskewDialog()
{
    ApplyToDialog* dialog = new ApplyToDialog(
                this, m_pageId, m_pageSelectionAccessor
                );

    dialog->setWindowTitle(tr("Apply Deskew parameters"));

    DeskewApplyWidget* options = nullptr;
    if (m_uiData.orientationFix() != Params::OrientationFixNone) {
        options = new DeskewApplyWidget(m_uiData.mode(), m_uiData.effectiveDeskewAngle(),
                                        m_uiData.pageRotationRelDegree(), dialog);
        QLayout& l = dialog->initNewTopSettingsPanel();
        l.addWidget(options);
    }

    connect(dialog, &ApplyToDialog::accepted, this, [=]() {

        std::vector<PageId> vec = dialog->getPageRangeSelectorWidget().result();
        std::set<PageId> pages(vec.begin(), vec.end());
        Settings::UpdateOpt opt = Settings::UpdateModeAndAngle;
        if (options) {
            if (options->isOrientationOptionSelected()) {
                opt = Settings::UpdateOrientationFix;
            } else if (options->isAllOptionSelected()) {
                opt = Settings::UpdateAll;
            }
        }

        if (!dialog->getPageRangeSelectorWidget().allPagesSelected()) {
            appliedTo(pages, opt);
        } else {
            appliedToAllPages(pages, opt);
        }
    }

    );

    dialog->show();
}

void
OptionsWidget::appliedTo(std::set<PageId> const& pages, Settings::UpdateOpt opt)
{
	if (pages.empty()) {
		return;
	}
	
	Params const params(
		m_uiData.effectiveDeskewAngle(),
        m_uiData.dependencies(), m_uiData.mode(), m_uiData.orientationFix(), m_uiData.requireRecalc()
	);
    m_ptrSettings->applyParams(pages, params, opt);
	for (PageId const& page_id: pages) {
		emit invalidateThumbnail(page_id);
	}
}

void
OptionsWidget::appliedToAllPages(std::set<PageId> const& pages, Settings::UpdateOpt opt)
{
	if (pages.empty()) {
		return;
	}
	
	Params const params(
		m_uiData.effectiveDeskewAngle(),
        m_uiData.dependencies(), m_uiData.mode(), m_uiData.orientationFix(), m_uiData.requireRecalc()
	);

    m_ptrSettings->applyParams(pages, params, opt);
	emit invalidateAllThumbnails();
}

void
OptionsWidget::manualDeskewAngleSetExternally(double const degrees)
{
	m_uiData.setEffectiveDeskewAngle(degrees);
	m_uiData.setMode(MODE_MANUAL);
	updateModeIndication(MODE_MANUAL);
	setSpinBoxKnownState(degreesToSpinBox(degrees));
	commitCurrentParams();
	
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	m_pageId = page_id;    
	setSpinBoxUnknownState();
	autoBtn->setEnabled(false);
	manualBtn->setEnabled(false);

    if (!GlobalStaticSettings::m_drawDeskewOrientFix) {
        gbPageOrientationFix->setVisible(false);
    } else {
        gbPageOrientationFix->setVisible(m_pageId.subPage() != PageId::SubPage::SINGLE_PAGE);
    }
}

void
OptionsWidget::postUpdateUI(UiData const& ui_data)
{
	m_uiData = ui_data;
	autoBtn->setEnabled(true);
	manualBtn->setEnabled(true);
	updateModeIndication(ui_data.mode());
	setSpinBoxKnownState(degreesToSpinBox(ui_data.effectiveDeskewAngle()));
    displayOrientationFix();
    if (!btnRotatonNone->isChecked()) {
        gbPageOrientationFix->setChecked(true);
    }
}

void
OptionsWidget::spinBoxValueChanged(double const value)
{
	if (m_ignoreSpinBoxChanges) {
		return;
	}
	
	double const degrees = spinBoxToDegrees(value);
	m_uiData.setEffectiveDeskewAngle(degrees);
	m_uiData.setMode(MODE_MANUAL);
	updateModeIndication(MODE_MANUAL);
	commitCurrentParams();
	
	emit manualDeskewAngleSet(degrees);
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::modeChanged(bool const auto_mode)
{
	if (m_ignoreAutoManualToggle) {
		return;
	}

    m_uiData.setMode(auto_mode? MODE_AUTO : MODE_MANUAL);
    commitCurrentParams();
	
	if (auto_mode) {
		emit reloadRequested();
	}
}

void
OptionsWidget::updateModeIndication(AutoManualMode const mode)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	if (mode == MODE_AUTO) {
		autoBtn->setChecked(true);
	} else {
		manualBtn->setChecked(true);
	}
}

void
OptionsWidget::setSpinBoxUnknownState()
{
	ScopedIncDec<int> guard(m_ignoreSpinBoxChanges);
	
	angleSpinBox->setSpecialValueText("?");
	angleSpinBox->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	angleSpinBox->setValue(angleSpinBox->minimum());
	angleSpinBox->setEnabled(false);
}

void
OptionsWidget::setSpinBoxKnownState(double const angle)
{
	ScopedIncDec<int> guard(m_ignoreSpinBoxChanges);
	
	angleSpinBox->setSpecialValueText("");
	angleSpinBox->setValue(angle);
	
	// Right alignment doesn't work correctly, so we use the left one.
	angleSpinBox->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	angleSpinBox->setEnabled(true);
}

void
OptionsWidget::commitCurrentParams()
{
	Params params(
		m_uiData.effectiveDeskewAngle(),
        m_uiData.dependencies(), m_uiData.mode(), m_uiData.orientationFix(), m_uiData.requireRecalc()
	);
	params.computeDeviation(m_ptrSettings->avg());
	m_ptrSettings->setPageParams(m_pageId, params);
}

void
OptionsWidget::displayOrientationFix()
{
    const Params::OrientationFix f = m_uiData.orientationFix();
    btnRotatonNone->setChecked(f == Params::OrientationFixNone);
    btnRotatonLeft->setChecked(f == Params::OrientationFixLeft);
    btnRotatonRight->setChecked(f == Params::OrientationFixRight);
}

double
OptionsWidget::spinBoxToDegrees(double const sb_value)
{
	// The spin box shows the angle in a usual geometric way,
	// with positive angles going counter-clockwise.
	// Internally, we operate with angles going clockwise,
	// because the Y axis points downwards in computer graphics.
	return -sb_value;
}

double
OptionsWidget::degreesToSpinBox(double const degrees)
{
	// See above.
	return -degrees;
}


/*========================== OptionsWidget::UiData =========================*/

OptionsWidget::UiData::UiData()
:	m_effDeskewAngle(0.0),
    m_mode(MODE_AUTO),
    m_orientationFix(Params::OrientationFixNone),
    m_requireRecalc(false)
{
}

OptionsWidget::UiData::~UiData()
{
}

void
OptionsWidget::UiData::setEffectiveDeskewAngle(double const degrees)
{
	m_effDeskewAngle = degrees;
}

double
OptionsWidget::UiData::effectiveDeskewAngle() const
{
	return m_effDeskewAngle;
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
    if (m_mode != mode) {
        m_requireRecalc = mode == MODE_AUTO;
        m_mode = mode;
    }
}

AutoManualMode
OptionsWidget::UiData::mode() const
{
	return m_mode;
}

void
OptionsWidget::UiData::setOrientationFix (Params::OrientationFix rotation)
{
    if (m_orientationFix != rotation) {
        if (m_mode == MODE_AUTO) {
            m_requireRecalc = true;
        }
        m_orientationFix = rotation;
    }
}

Params::OrientationFix
OptionsWidget::UiData::orientationFix() const
{
    if (!GlobalStaticSettings::m_drawDeskewOrientFix) {
        return Params::OrientationFixNone;
    }

    return m_orientationFix;
}

} // namespace deskew

void deskew::OptionsWidget::on_gbPageOrientationFix_toggled(bool arg1)
{
    wgtPageOrientationFix->setVisible(arg1);
    if (!arg1) {
        btnRotatonNone->setChecked(true);
    }
}

void deskew::OptionsWidget::on_btnRotatonLeft_toggled(bool checked)
{
    if (checked) {
        m_uiData.setOrientationFix(Params::OrientationFixLeft);
        commitCurrentParams();
        emit reloadRequested();
    }
}

void deskew::OptionsWidget::on_btnRotatonNone_toggled(bool checked)
{
    if (checked) {
        m_uiData.setOrientationFix(Params::OrientationFixNone);
        commitCurrentParams();
        emit reloadRequested();
    }
}

void deskew::OptionsWidget::on_btnRotatonRight_toggled(bool checked)
{
    if (checked) {
        m_uiData.setOrientationFix(Params::OrientationFixRight);
        commitCurrentParams();
        emit reloadRequested();
    }
}
