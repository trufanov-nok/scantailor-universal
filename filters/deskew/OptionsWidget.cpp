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

#include "OptionsWidget.h"
#include "OptionsWidget.moc"
#include "ApplyDialog.h"
#include "Settings.h"
#include "DistortionType.h"
#include "ScopedIncDec.h"
#include <QSize>
#include <QString>
#include <QColor>
#include <QPalette>
#include <QIcon>
#include <QPixmap>
#include <QPainter>
#include <Qt>
#include <QtGlobal>
#include <math.h>
#include <boost/foreach.hpp>

namespace deskew
{

double const OptionsWidget::MAX_ANGLE = 45.0;

OptionsWidget::OptionsWidget(IntrusivePtr<Settings> const& settings,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings)
,	m_pageParams(Dependencies())
,	m_ignoreSignalsFromUiControls(0)
,	m_pageSelectionAccessor(page_selection_accessor)
{
	using namespace dewarping;

	ui.setupUi(this);
	setupDistortionTypeButtons();

	// Rotation angle UI.
	ui.angleSpinBox->setSuffix(QChar(0x00B0)); // the degree symbol
	ui.angleSpinBox->setRange(-MAX_ANGLE, MAX_ANGLE);
	ui.angleSpinBox->adjustSize();
	setSpinBoxUnknownState();
	connect(
		ui.angleSpinBox, SIGNAL(valueChanged(double)),
		this, SLOT(spinBoxValueChanged(double))
	);

	// Depth perception UI.
	ui.depthPerceptionSlider->setMinimum(
		depthPerceptionToSlider(DepthPerception::minValue())
	);
	ui.depthPerceptionSlider->setMaximum(
		depthPerceptionToSlider(DepthPerception::maxValue())
	);
	connect(
		ui.depthPerceptionSlider, SIGNAL(valueChanged(int)),
		SLOT(depthPerceptionSliderMoved(int))
	);
	connect(
		ui.depthPerceptionSlider, SIGNAL(sliderReleased()),
		SLOT(depthPerceptionSliderReleased())
	);
	connect(
		ui.applyDepthPerceptionBtn, SIGNAL(clicked()),
		SLOT(showApplyDepthPerceptionDialog())
	);

	// Distortion type UI.
	connect(
		ui.noDistortionButton, SIGNAL(toggled(bool)),
		SLOT(noDistortionToggled(bool))
	);
	connect(
		ui.rotationDistortionButton, SIGNAL(toggled(bool)),
		SLOT(rotationDistortionToggled(bool))
	);
	connect(
		ui.perspectiveDistortionButton, SIGNAL(toggled(bool)),
		SLOT(perspectiveDistortionToggled(bool))
	);
	connect(
		ui.warpDistortionButton, SIGNAL(toggled(bool)),
		SLOT(warpDistortionToggled(bool))
	);
	connect(
		ui.applyDistortionTypeBtn, SIGNAL(clicked()),
		this, SLOT(showApplyDistortionTypeDialog())
	);

	// Auto / Manual mode.
	connect(ui.autoBtn, SIGNAL(toggled(bool)), this, SLOT(modeChanged(bool)));

}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::showApplyDistortionTypeDialog()
{
	QAbstractButton* btn = m_distortionTypeButtons[m_pageParams.distortionType().get()];
	QPixmap pixmap = btn->icon().pixmap(btn->iconSize(), QIcon::Active, QIcon::On);

	ApplyDialog* dialog = new ApplyDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(tr("Apply Distortion Type"));
	dialog->setPixmap(pixmap);

	connect(
		dialog, SIGNAL(appliedTo(std::set<PageId> const&)),
		this, SLOT(distortionTypeAppliedTo(std::set<PageId> const&))
	);
	connect(
		dialog, SIGNAL(appliedToAllPages(std::set<PageId> const&)),
		this, SLOT(distortionTypeAppliedToAllPages(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::distortionTypeAppliedTo(std::set<PageId> const& pages)
{
	if (pages.empty()) {
		return;
	}

	m_ptrSettings->setDistortionType(pages, m_pageParams.distortionType());

	BOOST_FOREACH(PageId const& page_id, pages) {
		emit invalidateThumbnail(page_id);
	}
}

void
OptionsWidget::distortionTypeAppliedToAllPages(std::set<PageId> const& pages)
{
	if (pages.empty()) {
		return;
	}
	
	m_ptrSettings->setDistortionType(pages, m_pageParams.distortionType());

	emit invalidateAllThumbnails();
}

void
OptionsWidget::showApplyDepthPerceptionDialog()
{
	ApplyDialog* dialog = new ApplyDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(tr("Apply Depth Perception"));

	connect(
		dialog, SIGNAL(appliedTo(std::set<PageId> const&)),
		this, SLOT(depthPerceptionAppliedTo(std::set<PageId> const&))
	);
	connect(
		dialog, SIGNAL(appliedToAllPages(std::set<PageId> const&)),
		this, SLOT(depthPerceptionAppliedToAllPages(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::depthPerceptionAppliedTo(std::set<PageId> const& pages)
{
	if (pages.empty()) {
		return;
	}

	m_ptrSettings->setDepthPerception(pages, m_pageParams.dewarpingParams().depthPerception());

	for (PageId const& page_id : pages) {
		emit invalidateThumbnail(page_id);
	}
}

void
OptionsWidget::depthPerceptionAppliedToAllPages(std::set<PageId> const& pages)
{
	if (pages.empty()) {
		return;
	}

	m_ptrSettings->setDepthPerception(pages, m_pageParams.dewarpingParams().depthPerception());
	emit invalidateAllThumbnails();
}

void
OptionsWidget::manualDistortionModelSetExternally(
	dewarping::DistortionModel const& model)
{
	// As we reuse DewarpingView for DistortionType::PERSPECTIVE,
	// we get called both in dewarping and perspective modes.
	if (m_pageParams.distortionType() == DistortionType::WARP) {
		m_pageParams.dewarpingParams().setDistortionModel(model);
		m_pageParams.dewarpingParams().setMode(MODE_MANUAL);
	} else if (m_pageParams.distortionType() == DistortionType::PERSPECTIVE) {
		m_pageParams.perspectiveParams().setCorner(
			PerspectiveParams::TOP_LEFT, model.topCurve().polyline().front()
		);
		m_pageParams.perspectiveParams().setCorner(
			PerspectiveParams::TOP_RIGHT, model.topCurve().polyline().back()
		);
		m_pageParams.perspectiveParams().setCorner(
			PerspectiveParams::BOTTOM_LEFT, model.bottomCurve().polyline().front()
		);
		m_pageParams.perspectiveParams().setCorner(
			PerspectiveParams::BOTTOM_RIGHT, model.bottomCurve().polyline().back()
		);
		m_pageParams.perspectiveParams().setMode(MODE_MANUAL);
	} else {
		assert(!"unreachable");
	}

	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	updateModeIndication(MODE_MANUAL);
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::manualDeskewAngleSetExternally(double const degrees)
{
	m_pageParams.rotationParams().setCompensationAngleDeg(degrees);
	m_pageParams.rotationParams().setMode(MODE_MANUAL);
	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	
	updateModeIndication(MODE_MANUAL);
	setSpinBoxKnownState(degreesToSpinBox(degrees));
	
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::preUpdateUI(PageId const& page_id, DistortionType const& distortion_type)
{
	ScopedIncDec<int> guard(m_ignoreSignalsFromUiControls);

	m_pageId = page_id;
	m_pageParams.setDistortionType(distortion_type);

	setupUiForDistortionType(distortion_type);
	hideDistortionDependentUiElements();
}

void
OptionsWidget::postUpdateUI(Params const& page_params)
{
	ScopedIncDec<int> guard(m_ignoreSignalsFromUiControls);

	m_pageParams = page_params;
	ui.autoBtn->setEnabled(true);
	ui.manualBtn->setEnabled(true);
	updateModeIndication(page_params.mode());

	setupUiForDistortionType(page_params.distortionType());

	if (page_params.distortionType() == DistortionType::ROTATION) {
		double const angle = page_params.rotationParams().compensationAngleDeg();
		setSpinBoxKnownState(degreesToSpinBox(angle));
	} else if (page_params.distortionType() == DistortionType::WARP) {
		double const depth_perception = page_params.dewarpingParams().depthPerception().value();
		ui.depthPerceptionSlider->setValue(depthPerceptionToSlider(depth_perception));
	}
}

void
OptionsWidget::noDistortionToggled(bool checked)
{
	if (!checked || m_ignoreSignalsFromUiControls) {
		return;
	}

	m_pageParams.setDistortionType(DistortionType::NONE);
	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	setupUiForDistortionType(DistortionType::NONE);
	emit reloadRequested();
}

void
OptionsWidget::rotationDistortionToggled(bool checked)
{
	if (!checked || m_ignoreSignalsFromUiControls) {
		return;
	}

	m_pageParams.setDistortionType(DistortionType::ROTATION);
	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	setupUiForDistortionType(DistortionType::ROTATION);
	emit reloadRequested();
}

void
OptionsWidget::perspectiveDistortionToggled(bool checked)
{
	if (!checked || m_ignoreSignalsFromUiControls) {
		return;
	}

	m_pageParams.setDistortionType(DistortionType::PERSPECTIVE);
	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	setupUiForDistortionType(DistortionType::PERSPECTIVE);
	emit reloadRequested();
}

void
OptionsWidget::warpDistortionToggled(bool checked)
{
	if (!checked || m_ignoreSignalsFromUiControls) {
		return;
	}

	m_pageParams.setDistortionType(DistortionType::WARP);
	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	setupUiForDistortionType(DistortionType::WARP);
	emit reloadRequested();
}

void
OptionsWidget::spinBoxValueChanged(double const value)
{
	if (m_ignoreSignalsFromUiControls) {
		return;
	}

	double const degrees = spinBoxToDegrees(value);
	m_pageParams.rotationParams().setCompensationAngleDeg(degrees);
	m_pageParams.rotationParams().setMode(MODE_MANUAL);

	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	updateModeIndication(MODE_MANUAL);
	
	emit manualDeskewAngleSet(degrees);
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::modeChanged(bool const auto_mode)
{
	if (m_ignoreSignalsFromUiControls) {
		return;
	}
	
	switch (m_pageParams.distortionType().get()) {
		case DistortionType::NONE:
			break;
		case DistortionType::ROTATION:
			if (auto_mode) {
				m_pageParams.rotationParams().invalidate();
			} else {
				m_pageParams.rotationParams().setMode(MODE_MANUAL);
			}
			break;
		case DistortionType::PERSPECTIVE:
			if (auto_mode) {
				m_pageParams.perspectiveParams().invalidate();
			} else {
				m_pageParams.perspectiveParams().setMode(MODE_MANUAL);
			}
			break;
		case DistortionType::WARP:
			if (auto_mode) {
				m_pageParams.dewarpingParams().invalidate();
			} else {
				m_pageParams.dewarpingParams().setMode(MODE_MANUAL);
			}
			break;
	}

	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	if (auto_mode) {
		emit reloadRequested();
	}
}

void
OptionsWidget::depthPerceptionSliderMoved(int value)
{
	double const depth_perception = sliderToDepthPerception(value);
	m_pageParams.dewarpingParams().setDepthPerception(depth_perception);
	m_ptrSettings->setPageParams(m_pageId, m_pageParams);

	emit depthPerceptionSetByUser(depth_perception);

	if (!ui.depthPerceptionSlider->isSliderDown()) {
		emit invalidateThumbnail(m_pageId);
	}
}

void
OptionsWidget::depthPerceptionSliderReleased()
{
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::setupDistortionTypeButtons()
{
	static_assert(
		DistortionType::LAST + 1 - DistortionType::FIRST == 4,
		"Unexpected number of distortion types"
	);
	m_distortionTypeButtons[DistortionType::NONE] = ui.noDistortionButton;
	m_distortionTypeButtons[DistortionType::ROTATION] = ui.rotationDistortionButton;
	m_distortionTypeButtons[DistortionType::PERSPECTIVE] = ui.perspectiveDistortionButton;
	m_distortionTypeButtons[DistortionType::WARP] = ui.warpDistortionButton;

	char const* resource_names[DistortionType::LAST + 1];
	resource_names[DistortionType::NONE] = ":/icons/distortion-none.png";
	resource_names[DistortionType::ROTATION] = ":/icons/distortion-rotation.png";
	resource_names[DistortionType::PERSPECTIVE] = ":/icons/distortion-perspective.png";
	resource_names[DistortionType::WARP] = ":/icons/distortion-warp.png";

	// Take the "highlight" color from palette and whiten it quite a bit.
	QColor selected_color(palette().color(QPalette::Active, QPalette::Highlight).toHsl());
	selected_color.setHsl(selected_color.hslHue(), selected_color.hslSaturation(), 230);

	for (int i = DistortionType::FIRST; i <= DistortionType::LAST; ++i) {
		// The files on disk are black on transparent background.
		QPixmap black_on_transparent(resource_names[i]);

		QPixmap unselected(black_on_transparent.size());
		unselected.fill(Qt::white);
		QPainter(&unselected).drawPixmap(0, 0, black_on_transparent);

		QPixmap selected(black_on_transparent.size());
		selected.fill(selected_color);
		QPainter(&selected).drawPixmap(0, 0, black_on_transparent);

		QIcon icon;
		icon.addPixmap(unselected, QIcon::Active, QIcon::Off);
		icon.addPixmap(selected, QIcon::Active, QIcon::On);
		m_distortionTypeButtons[i]->setIcon(icon);
	}
}

void
OptionsWidget::hideDistortionDependentUiElements()
{
	ui.autoManualPanel->setVisible(false);
	ui.rotationPanel->setVisible(false);
	ui.depthPerceptionPanel->setVisible(false);
}

void
OptionsWidget::setupUiForDistortionType(DistortionType::Type type)
{
	ScopedIncDec<int> guard(m_ignoreSignalsFromUiControls);

	m_distortionTypeButtons[type]->setChecked(true);

	ui.autoManualPanel->setVisible(type != DistortionType::NONE);
	ui.rotationPanel->setVisible(type == DistortionType::ROTATION);
	ui.depthPerceptionPanel->setVisible(type == DistortionType::WARP);
}

void
OptionsWidget::updateModeIndication(AutoManualMode const mode)
{
	ScopedIncDec<int> guard(m_ignoreSignalsFromUiControls);
	
	if (mode == MODE_AUTO) {
		ui.autoBtn->setChecked(true);
	} else {
		ui.manualBtn->setChecked(true);
	}
}

void
OptionsWidget::setSpinBoxUnknownState()
{
	ScopedIncDec<int> guard(m_ignoreSignalsFromUiControls);
	
	ui.angleSpinBox->setSpecialValueText("?");
	ui.angleSpinBox->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	ui.angleSpinBox->setValue(ui.angleSpinBox->minimum());
	ui.angleSpinBox->setEnabled(false);
}

void
OptionsWidget::setSpinBoxKnownState(double const angle)
{
	ScopedIncDec<int> guard(m_ignoreSignalsFromUiControls);
	
	ui.angleSpinBox->setSpecialValueText("");
	ui.angleSpinBox->setValue(angle);
	
	// Right alignment doesn't work correctly, so we use the left one.
	ui.angleSpinBox->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	ui.angleSpinBox->setEnabled(true);
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

int
OptionsWidget::depthPerceptionToSlider(double depth_perception)
{
	return qRound(depth_perception * 10);
}

double
OptionsWidget::sliderToDepthPerception(int slider_value)
{
	return slider_value / 10.0;
}

} // namespace deskew
