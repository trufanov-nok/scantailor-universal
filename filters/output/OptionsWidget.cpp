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
#include "ApplyColorsDialog.h"
#include "Settings.h"
#include "Params.h"
#include "dewarping/DistortionModel.h"
#include "DespeckleLevel.h"
#include "ZoneSet.h"
#include "PictureZoneComparator.h"
#include "FillZoneComparator.h"
#include "../../Utils.h"
#include "ScopedIncDec.h"
#include "config.h"
#include <boost/foreach.hpp>
#include <QtGlobal>
#include <QVariant>
#include <QColorDialog>
#include <QToolTip>
#include <QString>
#include <QCursor>
#include <QPoint>
#include <QSize>
#include <Qt>
#include <QDebug>

using namespace dewarping;

namespace output
{

OptionsWidget::OptionsWidget(
	IntrusivePtr<Settings> const& settings,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings),
	m_pageSelectionAccessor(page_selection_accessor),
	m_despeckleLevel(DESPECKLE_NORMAL),
	m_lastTab(TAB_OUTPUT),
	m_ignoreThresholdChanges(0),
	m_ignoreDespeckleLevelChanges(0),
	m_ignoreScaleChanges(0)
{
	setupUi(this);
	
	colorModeSelector->addItem(tr("Black and White"), ColorParams::BLACK_AND_WHITE);
	colorModeSelector->addItem(tr("Color / Grayscale"), ColorParams::COLOR_GRAYSCALE);
	colorModeSelector->addItem(tr("Mixed"), ColorParams::MIXED);
	
	darkerThresholdLink->setText(
		Utils::richTextForLink(darkerThresholdLink->text())
	);
	lighterThresholdLink->setText(
		Utils::richTextForLink(lighterThresholdLink->text())
	);
	thresholdSlider->setToolTip(QString::number(thresholdSlider->value()));
	
	updateColorsDisplay();
	updateScaleDisplay();
	
	connect(
		scale1xBtn, &QAbstractButton::toggled,
		[this](bool checked) { if (checked) scaleChanged(1.0); }
	);
	connect(
		scale15xBtn, &QAbstractButton::toggled,
		[this](bool checked) { if (checked) scaleChanged(1.5); }
	);
	connect(
		scale2xBtn, &QAbstractButton::toggled,
		[this](bool checked) { if (checked) scaleChanged(2.0); }
	);
	connect(
		colorModeSelector, SIGNAL(currentIndexChanged(int)),
		this, SLOT(colorModeChanged(int))
	);
	connect(
		whiteMarginsCB, SIGNAL(clicked(bool)),
		this, SLOT(whiteMarginsToggled(bool))
	);
	connect(
		equalizeIlluminationCB, SIGNAL(clicked(bool)),
		this, SLOT(equalizeIlluminationToggled(bool))
	);
	connect(
		lighterThresholdLink, SIGNAL(linkActivated(QString const&)),
		this, SLOT(setLighterThreshold())
	);
	connect(
		darkerThresholdLink, SIGNAL(linkActivated(QString const&)),
		this, SLOT(setDarkerThreshold())
	);
	connect(
		neutralThresholdBtn, SIGNAL(clicked()),
		this, SLOT(setNeutralThreshold())
	);
	connect(
		thresholdSlider, SIGNAL(valueChanged(int)),
		this, SLOT(bwThresholdChanged())
	);
	connect(
		thresholdSlider, SIGNAL(sliderReleased()),
		this, SLOT(bwThresholdChanged())
	);
	connect(
		applyColorsButton, SIGNAL(clicked()),
		this, SLOT(applyColorsButtonClicked())
	);
	connect(
		despeckleOffBtn, &QAbstractButton::toggled,
		[this](bool checked) { if (checked) despeckleLevelSelected(DESPECKLE_OFF); }
	);
	connect(
		despeckleCautiousBtn, &QAbstractButton::toggled,
		[this](bool checked) { if (checked) despeckleLevelSelected(DESPECKLE_CAUTIOUS); }
	);
	connect(
		despeckleNormalBtn, &QAbstractButton::toggled,
		[this](bool checked) { if (checked) despeckleLevelSelected(DESPECKLE_NORMAL); }
	);
	connect(
		despeckleAggressiveBtn, &QAbstractButton::toggled,
		[this](bool checked) { if (checked) despeckleLevelSelected(DESPECKLE_AGGRESSIVE); }
	);
	connect(
		applyDespeckleButton, SIGNAL(clicked()),
		this, SLOT(applyDespeckleButtonClicked())
	);
	
	thresholdSlider->setMinimum(-50);
	thresholdSlider->setMaximum(50);
	thresholLabel->setText(QString::number(thresholdSlider->value()));
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	Params const params(m_ptrSettings->getParams(page_id));
	m_pageId = page_id;
	m_colorParams = params.colorParams();
	m_despeckleLevel = params.despeckleLevel();
	m_thisPageOutputSize.reset();
	updateColorsDisplay();
	updateScaleDisplay();
}

void
OptionsWidget::postUpdateUI(QSize const& output_size)
{
	m_thisPageOutputSize = output_size;
	updateScaleDisplay();
}

void
OptionsWidget::tabChanged(ImageViewTab const tab)
{
	m_lastTab = tab;
	updateColorsDisplay();
	reloadIfNecessary();
}

void
OptionsWidget::colorModeChanged(int const idx)
{
	int const mode = colorModeSelector->itemData(idx).toInt();
	m_colorParams.setColorMode((ColorParams::ColorMode)mode);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	updateColorsDisplay();
	emit reloadRequested();
}

void
OptionsWidget::whiteMarginsToggled(bool const checked)
{
	ColorGrayscaleOptions opt(m_colorParams.colorGrayscaleOptions());
	opt.setWhiteMargins(checked);
	if (!checked) {
		opt.setNormalizeIllumination(false);
		equalizeIlluminationCB->setChecked(false);
	}
	m_colorParams.setColorGrayscaleOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	equalizeIlluminationCB->setEnabled(checked);
	emit reloadRequested();
}

void
OptionsWidget::equalizeIlluminationToggled(bool const checked)
{
	ColorGrayscaleOptions opt(m_colorParams.colorGrayscaleOptions());
	opt.setNormalizeIllumination(checked);
	m_colorParams.setColorGrayscaleOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::setLighterThreshold()
{
	thresholdSlider->setValue(thresholdSlider->value() - 1);
}

void
OptionsWidget::setDarkerThreshold()
{
	thresholdSlider->setValue(thresholdSlider->value() + 1);
}

void
OptionsWidget::setNeutralThreshold()
{
	thresholdSlider->setValue(0);
}

void
OptionsWidget::bwThresholdChanged()
{
	int const value = thresholdSlider->value();
	QString const tooltip_text(QString::number(value));
	thresholdSlider->setToolTip(tooltip_text);
	
	thresholLabel->setText(QString::number(value));
	
	if (m_ignoreThresholdChanges) {
		return;
	}
	
	// Show the tooltip immediately.
	QPoint const center(thresholdSlider->rect().center());
	QPoint tooltip_pos(thresholdSlider->mapFromGlobal(QCursor::pos()));
	tooltip_pos.setY(center.y());
	tooltip_pos.setX(qBound(0, tooltip_pos.x(), thresholdSlider->width()));
	tooltip_pos = thresholdSlider->mapToGlobal(tooltip_pos);
	QToolTip::showText(tooltip_pos, tooltip_text, thresholdSlider);
	
	if (thresholdSlider->isSliderDown()) {
		// Wait for it to be released.
		// We could have just disabled tracking, but in that case we wouldn't
		// be able to show tooltips with a precise value.
		return;
	}

	BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
	if (opt.thresholdAdjustment() == value) {
		// Didn't change.
		return;
	}

	opt.setThresholdAdjustment(value);
	m_colorParams.setBlackWhiteOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
	
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::applyColorsButtonClicked()
{
	ApplyColorsDialog* dialog = new ApplyColorsDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&)),
		this, SLOT(applyColorsConfirmed(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::applyColorsConfirmed(std::set<PageId> const& pages)
{
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setColorParams(page_id, m_colorParams);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::despeckleLevelSelected(DespeckleLevel const level)
{
	if (m_ignoreDespeckleLevelChanges) {
		return;
	}

	m_despeckleLevel = level;
	m_ptrSettings->setDespeckleLevel(m_pageId, level);

	bool handled = false;
	emit despeckleLevelChanged(level, &handled);
	
	if (handled) {
		// This means we are on the "Despeckling" tab.
		emit invalidateThumbnail(m_pageId);
	} else {
		emit reloadRequested();
	}
}

void
OptionsWidget::scaleChanged(double const scale)
{
	if (m_ignoreScaleChanges) {
		return;
	}

	m_ptrSettings->setScalingFactor(scale);
	updateScaleDisplay();

	emit invalidateAllThumbnails();
	emit reloadRequested();
}

void
OptionsWidget::applyDespeckleButtonClicked()
{
	ApplyColorsDialog* dialog = new ApplyColorsDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(tr("Apply Despeckling Level"));
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&)),
		this, SLOT(applyDespeckleConfirmed(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::applyDespeckleConfirmed(std::set<PageId> const& pages)
{
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setDespeckleLevel(page_id, m_despeckleLevel);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::reloadIfNecessary()
{
	ZoneSet saved_picture_zones;
	ZoneSet saved_fill_zones;
	dewarping::DistortionModel saved_distortion_model;
	DespeckleLevel saved_despeckle_level = DESPECKLE_CAUTIOUS;
	
	std::auto_ptr<OutputParams> output_params(m_ptrSettings->getOutputParams(m_pageId));
	if (output_params.get()) {
		saved_picture_zones = output_params->pictureZones();
		saved_fill_zones = output_params->fillZones();
		saved_despeckle_level = output_params->outputImageParams().despeckleLevel();
	}

	if (!PictureZoneComparator::equal(saved_picture_zones, m_ptrSettings->pictureZonesForPage(m_pageId))) {
		emit reloadRequested();
		return;
	} else if (!FillZoneComparator::equal(saved_fill_zones, m_ptrSettings->fillZonesForPage(m_pageId))) {
		emit reloadRequested();
		return;
	}

	Params const params(m_ptrSettings->getParams(m_pageId));

	if (saved_despeckle_level != params.despeckleLevel()) {
		emit reloadRequested();
		return;
	}
}

void
OptionsWidget::updateColorsDisplay()
{
	colorModeSelector->blockSignals(true);
	
	ColorParams::ColorMode const color_mode = m_colorParams.colorMode();
	int const color_mode_idx = colorModeSelector->findData(color_mode);
	colorModeSelector->setCurrentIndex(color_mode_idx);
	
	bool color_grayscale_options_visible = false;
	bool bw_options_visible = false;
	switch (color_mode) {
		case ColorParams::BLACK_AND_WHITE:
			bw_options_visible = true;
			break;
		case ColorParams::COLOR_GRAYSCALE:
			color_grayscale_options_visible = true;
			break;
		case ColorParams::MIXED:
			bw_options_visible = true;
			break;
	}
	
	colorGrayscaleOptions->setVisible(color_grayscale_options_visible);
	if (color_grayscale_options_visible) {
		ColorGrayscaleOptions const opt(
			m_colorParams.colorGrayscaleOptions()
		);
		whiteMarginsCB->setChecked(opt.whiteMargins());
		equalizeIlluminationCB->setChecked(opt.normalizeIllumination());
		equalizeIlluminationCB->setEnabled(opt.whiteMargins());
	}
	
	bwOptions->setVisible(bw_options_visible);
	despecklePanel->setVisible(bw_options_visible);

	if (bw_options_visible) {
		ScopedIncDec<int> const despeckle_guard(m_ignoreDespeckleLevelChanges);

		switch (m_despeckleLevel) {
			case DESPECKLE_OFF:
				despeckleOffBtn->setChecked(true);
				break;
			case DESPECKLE_CAUTIOUS:
				despeckleCautiousBtn->setChecked(true);
				break;
			case DESPECKLE_NORMAL:
				despeckleNormalBtn->setChecked(true);
				break;
			case DESPECKLE_AGGRESSIVE:
				despeckleAggressiveBtn->setChecked(true);
				break;
		}

		ScopedIncDec<int> const threshold_guard(m_ignoreThresholdChanges);
		thresholdSlider->setValue(
			m_colorParams.blackWhiteOptions().thresholdAdjustment()
		);
	}
	
	colorModeSelector->blockSignals(false);
}

void
OptionsWidget::updateScaleDisplay()
{
	ScopedIncDec<int> const guard(m_ignoreScaleChanges);

	double const scale = m_ptrSettings->scalingFactor();
	if (scale < 1.25) {
		scale1xBtn->setChecked(true);
	} else if (scale < 1.75) {
		scale15xBtn->setChecked(true);
	} else {
		scale2xBtn->setChecked(true);
	}

	if (m_thisPageOutputSize) {
		int const width = m_thisPageOutputSize->width();
		int const height = m_thisPageOutputSize->height();
		scaleLabel->setText(tr("This page: %1 x %2 px").arg(width).arg(height));
	} else {
		scaleLabel->setText(QString());
	}
}

} // namespace output
