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
#include "ChangeDpiDialog.h"
#include "ChangeDewarpingDialog.h"
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
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
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
#include <QSettings>
#include <tiff.h>
#include <QMenu>
#include <QWheelEvent>
#include <QPainter>

namespace output
{

OptionsWidget::OptionsWidget(
	IntrusivePtr<Settings> const& settings,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings),
	m_pageSelectionAccessor(page_selection_accessor),
	m_despeckleLevel(DESPECKLE_NORMAL),
	m_lastTab(TAB_OUTPUT),
	m_ignoreThresholdChanges(0)
{
    setupUi(this);

    setDespeckleLevel(DESPECKLE_NORMAL);

	depthPerceptionSlider->setMinimum(qRound(DepthPerception::minValue() * 10));
	depthPerceptionSlider->setMaximum(qRound(DepthPerception::maxValue() * 10));

	thresholdSlider->setToolTip(QString::number(thresholdSlider->value()));
    thresholdSlider->addAction(actionReset_to_default_value);
    m_ignore_system_wheel_settings = QSettings().value("mouse/ignore_system_wheel_settings", true).toBool();
    thresholdSlider->installEventFilter(this);
    if (m_ignore_system_wheel_settings){
        despeckleSlider->installEventFilter(this);
        depthPerceptionSlider->installEventFilter(this);
    }

    m_menuMode.addAction(actionModeBW);
    m_menuMode.addAction(actionModeColorOrGrayscale);
    m_menuMode.addAction(actionModeMixed);

    m_menuPictureShape.addAction(actionPictureShapeFree);
    m_menuPictureShape.addAction(actionPictureShapeRectangular);
    m_menuPictureShape.addAction(actionPictureShapeQuadro);
	
	updateDpiDisplay();
	updateColorsDisplay();
	updateDewarpingDisplay();
	
    connect(
        colorLayerCB, SIGNAL(clicked(bool)),
        this, SLOT(colorLayerCBToggled(bool))
    );
    connect(
        autoLayerCB, SIGNAL(clicked(bool)),
        this, SLOT(autoLayerCBToggled(bool))
    );
	connect(
		whiteMarginsCB, SIGNAL(clicked(bool)),
		this, SLOT(whiteMarginsToggled(bool))
	);
	connect(
		equalizeIlluminationCB, SIGNAL(clicked(bool)),
		this, SLOT(equalizeIlluminationToggled(bool))
    );

    despeckleSliderPanel->setVisible(false);
    connect(
        despeckleOffBtn, SIGNAL(clicked()),
        this, SLOT(despeckleOffSelected())
    );
    connect(
        despeckleCautiousBtn, SIGNAL(clicked()),
        this, SLOT(despeckleCautiousSelected())
    );
    connect(
        despeckleNormalBtn, SIGNAL(clicked()),
        this, SLOT(despeckleNormalSelected())
    );
    connect(
        despeckleAggressiveBtn, SIGNAL(clicked()),
        this, SLOT(despeckleAggressiveSelected())
    );

    connect(
        thresholdSlider, &QSlider::sliderReleased,
        this, &OptionsWidget::on_thresholdSlider_valueChanged
    );

    settingsChanged();
}

void
OptionsWidget::settingsChanged()
{
    QSettings s;
    thresholdSlider->setMinimum(s.value("output/binrization_threshold_control_min", -50).toInt());
    thresholdSlider->setMaximum(s.value("output/binrization_threshold_control_max", 50).toInt());
    thresholdLabel->setText(QString::number(thresholdSlider->value()));
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	Params const params(m_ptrSettings->getParams(page_id));
	m_pageId = page_id;
	m_outputDpi = params.outputDpi();
	m_colorParams = params.colorParams();
    setCurrentPictureShape(params.pictureShape());
    m_dewarpingMode = params.dewarpingMode();
	m_depthPerception = params.depthPerception();
    setDespeckleLevel(params.despeckleLevel());
	updateDpiDisplay();
	updateColorsDisplay();
	updateDewarpingDisplay();

    if(m_currentMode == ColorParams::MIXED) {

        bool picture_shape_visible = QSettings().value("picture_shape_detection/enabled", true).toBool();
        pictureShapeOptions->setVisible(picture_shape_visible);

        if (!picture_shape_visible && m_currentPictureShape != FREE_SHAPE) {
            setCurrentPictureShape(FREE_SHAPE);
        }

        bool quadro_visible = false;
        if (picture_shape_visible) {
            quadro_visible = QSettings().value("picture_shape_detection/smaller_rect", true).toBool();
        }

        if (!quadro_visible && m_menuPictureShape.actions().contains(actionPictureShapeQuadro)) {
            m_menuPictureShape.removeAction(actionPictureShapeQuadro);
            if (m_currentPictureShape == QUADRO_SHAPE) {
                setCurrentPictureShape(FREE_SHAPE);
            }
        }

        if (quadro_visible && !m_menuPictureShape.actions().contains(actionPictureShapeQuadro)) {
            m_menuPictureShape.addAction(actionPictureShapeQuadro);
        }
    }

}

void
OptionsWidget::updatePictureShapeValueText()
{
    switch (m_currentPictureShape) {
    case FREE_SHAPE: pictureShapeValue->setText(Utils::richTextForLink(actionPictureShapeFree->toolTip())); break;
    case RECTANGULAR_SHAPE: pictureShapeValue->setText(Utils::richTextForLink(actionPictureShapeRectangular->toolTip())); break;
    case QUADRO_SHAPE: pictureShapeValue->setText(Utils::richTextForLink(actionPictureShapeQuadro->toolTip())); break;
    default: ;
    }
}

void
OptionsWidget::postUpdateUI()
{
}

void
OptionsWidget::tabChanged(ImageViewTab const tab)
{
	m_lastTab = tab;
	updateDpiDisplay();
	updateColorsDisplay();
	updateDewarpingDisplay();
	reloadIfNecessary();
}

void
OptionsWidget::distortionModelChanged(dewarping::DistortionModel const& model)
{
	m_ptrSettings->setDistortionModel(m_pageId, model);
	
	// Note that OFF remains OFF while AUTO becomes MANUAL.
//begin of modified by monday2000
// Manual_Dewarp_Auto_Switch
// OFF becomes MANUAL too.
// Commented the code below.
	/*if (m_dewarpingMode == DewarpingMode::AUTO)*/ {
//end of modified by monday2000
		m_ptrSettings->setDewarpingMode(m_pageId, DewarpingMode::MANUAL);
		m_dewarpingMode = DewarpingMode::MANUAL;
		updateDewarpingDisplay();
	}
}

void
OptionsWidget::changeColorMode(ColorParams::ColorMode const mode)
{   
    setModeValue(mode);
	m_colorParams.setColorMode((ColorParams::ColorMode)mode);

    ColorGrayscaleOptions opt = m_colorParams.colorGrayscaleOptions();
    if (opt.colorLayerEnabled()) {
        opt.setColorLayerEnabled(false);
        m_colorParams.setColorGrayscaleOptions(opt);
    }

    m_ptrSettings->setColorParams(m_pageId, m_colorParams, ColorParamsApplyFilter::CopyMode);
    colorLayerCB->setCheckState(Qt::Unchecked);
    autoLayerCB->setCheckState(Qt::Checked);
	updateColorsDisplay();
	emit reloadRequested();
}

void
OptionsWidget::changePictureShape(PictureShape const shape)
{
    setCurrentPictureShape(shape);
    m_ptrSettings->setPictureShape(m_pageId, m_currentPictureShape);
    updatePictureShapeValueText();
	emit reloadRequested();
}

void
OptionsWidget::colorLayerCBToggled(bool const checked)
{
    ColorGrayscaleOptions opt = m_colorParams.colorGrayscaleOptions();
    opt.setColorLayerEnabled(checked);
    m_colorParams.setColorGrayscaleOptions(opt);

    m_ptrSettings->setColorParams(m_pageId, m_colorParams, ColorParamsApplyFilter::CopyMode);

    updateColorsDisplay();
    emit reloadRequested();
}

void
OptionsWidget::autoLayerCBToggled(bool const checked)
{
    pictureShapeOptions->setVisible(checked);

    ColorGrayscaleOptions opt = m_colorParams.colorGrayscaleOptions();
    opt.setAutoLayerEnabled(checked);
    m_colorParams.setColorGrayscaleOptions(opt);

    m_ptrSettings->setColorParams(m_pageId, m_colorParams, ColorParamsApplyFilter::CopyMode);

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
    m_ptrSettings->setColorParams(m_pageId, m_colorParams, ColorParamsApplyFilter::CopyMode);
	equalizeIlluminationCB->setEnabled(checked);
	emit reloadRequested();
}

void
OptionsWidget::equalizeIlluminationToggled(bool const checked)
{
	ColorGrayscaleOptions opt(m_colorParams.colorGrayscaleOptions());
	opt.setNormalizeIllumination(checked);
	m_colorParams.setColorGrayscaleOptions(opt);
    m_ptrSettings->setColorParams(m_pageId, m_colorParams, ColorParamsApplyFilter::CopyMode);
	emit reloadRequested();
}

void
OptionsWidget::dpiChanged(std::set<PageId> const& pages, Dpi const& dpi)
{
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setDpi(page_id, dpi);
	}
	emit invalidateAllThumbnails();
	
	if (pages.find(m_pageId) != pages.end()) {
		m_outputDpi = dpi;
		updateDpiDisplay();
		emit reloadRequested();
	}
}

void
OptionsWidget::applyColorsConfirmed(std::set<PageId> const& pages)
{
	BOOST_FOREACH(PageId const& page_id, pages) {
        m_ptrSettings->setColorParams(page_id, m_colorParams, ColorParamsApplyFilter::CopyMode);
        m_ptrSettings->setPictureShape(page_id, m_currentPictureShape);
        emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::despeckleOffSelected()
{
    handleDespeckleLevelChange(DESPECKLE_OFF);
}

void
OptionsWidget::despeckleCautiousSelected()
{
    handleDespeckleLevelChange(DESPECKLE_CAUTIOUS);
}

void
OptionsWidget::despeckleNormalSelected()
{
    handleDespeckleLevelChange(DESPECKLE_NORMAL);
}

void
OptionsWidget::despeckleAggressiveSelected()
{
    handleDespeckleLevelChange(DESPECKLE_AGGRESSIVE);
}

void
OptionsWidget::handleDespeckleLevelChange(DespeckleLevel const level)
{
    setDespeckleLevel(level);
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
OptionsWidget::applyDespeckleConfirmed(std::set<PageId> const& pages)
{
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setDespeckleLevel(page_id, m_despeckleLevel);
	}
	emit invalidateAllThumbnails();
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::dewarpingChanged(std::set<PageId> const& pages, DewarpingMode const& mode)
{
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setDewarpingMode(page_id, mode);
	}
	emit invalidateAllThumbnails();
	
	if (pages.find(m_pageId) != pages.end()) {
		if (m_dewarpingMode != mode) {
			m_dewarpingMode = mode;
			
			// We reload when we switch to auto dewarping, even if we've just
			// switched to manual, as we don't store the auto-generated distortion model.
			// We also have to reload if we are currently on the "Fill Zones" tab,
			// as it makes use of original <-> dewarped coordinate mapping,
			// which is too hard to update without reloading.  For consistency,
			// we reload not just on TAB_FILL_ZONES but on all tabs except TAB_DEWARPING.
			// PS: the static original <-> dewarped mappings are constructed
			// in Task::UiUpdater::updateUI().  Look for "new DewarpingPointMapper" there.
			if (mode == DewarpingMode::AUTO || m_lastTab != TAB_DEWARPING
//begin of modified by monday2000
//Marginal_Dewarping
				|| mode == DewarpingMode::MARGINAL
//end of modified by monday2000
				) {
				// Switch to the Output tab after reloading.
				m_lastTab = TAB_OUTPUT; 

				// These depend on the value of m_lastTab.
				updateDpiDisplay();
				updateColorsDisplay();
				updateDewarpingDisplay();

				emit reloadRequested();
			} else {
				// This one we have to call anyway, as it depends on m_dewarpingMode.
				updateDewarpingDisplay();
			}
		}
	}
}

void
OptionsWidget::applyDepthPerceptionConfirmed(std::set<PageId> const& pages)
{
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setDepthPerception(page_id, m_depthPerception);
	}
	emit invalidateAllThumbnails();
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::reloadIfNecessary()
{
	ZoneSet saved_picture_zones;
	ZoneSet saved_fill_zones;
	DewarpingMode saved_dewarping_mode;
	dewarping::DistortionModel saved_distortion_model;
	DepthPerception saved_depth_perception;
	DespeckleLevel saved_despeckle_level = DESPECKLE_CAUTIOUS;
	
	std::auto_ptr<OutputParams> output_params(m_ptrSettings->getOutputParams(m_pageId));
	if (output_params.get()) {
		saved_picture_zones = output_params->pictureZones();
		saved_fill_zones = output_params->fillZones();
		saved_dewarping_mode = output_params->outputImageParams().dewarpingMode();
		saved_distortion_model = output_params->outputImageParams().distortionModel();
		saved_depth_perception = output_params->outputImageParams().depthPerception();
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

	if (saved_dewarping_mode == DewarpingMode::OFF && params.dewarpingMode() == DewarpingMode::OFF) {
		// In this case the following two checks don't matter.
	} else if (saved_depth_perception.value() != params.depthPerception().value()) {
		emit reloadRequested();
		return;
	} else if (saved_dewarping_mode == DewarpingMode::AUTO && params.dewarpingMode() == DewarpingMode::AUTO) {
		// The check below doesn't matter in this case.
//begin of modified by monday2000
//Marginal_Dewarping
	} else if (saved_dewarping_mode == DewarpingMode::MARGINAL && params.dewarpingMode() == DewarpingMode::MARGINAL) {
		// The check below doesn't matter in this case.
//end of modified by monday2000
	} else if (!saved_distortion_model.matches(params.distortionModel())) {
		emit reloadRequested();
		return;
	} else if ((saved_dewarping_mode == DewarpingMode::OFF) != (params.dewarpingMode() == DewarpingMode::OFF)) {
		emit reloadRequested();
		return;
	}
}

void
OptionsWidget::updateDpiDisplay()
{
    if (m_outputDpi.horizontal() != m_outputDpi.vertical()) {
        QString dpi_label = tr("%1 x %2 dpi")
                .arg(m_outputDpi.horizontal())
                .arg(m_outputDpi.vertical());
        dpiValue->setText(Utils::richTextForLink(dpi_label));
    } else {
        QString dpi_label = tr("%1 dpi").arg(QString::number(m_outputDpi.horizontal()));
        dpiValue->setText(Utils::richTextForLink(dpi_label));
    }
}

void
OptionsWidget::updateModeValueText()
{
    switch (m_currentMode) {
        case ColorParams::BLACK_AND_WHITE:
            modeValue->setText(Utils::richTextForLink(actionModeBW->toolTip()));
            break;
        case ColorParams::COLOR_GRAYSCALE:
            modeValue->setText(Utils::richTextForLink(actionModeColorOrGrayscale->toolTip()));
            break;
        case ColorParams::MIXED:
            modeValue->setText(Utils::richTextForLink(actionModeMixed->toolTip()));
            break;
    }
}

void
OptionsWidget::updateColorsDisplay()
{	
    setModeValue(m_colorParams.colorMode());

	bool color_grayscale_options_visible = false;
	bool bw_options_visible = false;
	bool picture_shape_visible = false;



    switch (m_currentMode) {
        case ColorParams::BLACK_AND_WHITE:
            bw_options_visible = true;
            break;
        case ColorParams::COLOR_GRAYSCALE:
            color_grayscale_options_visible = true;
            break;
        case ColorParams::MIXED:
            bw_options_visible = true;
            picture_shape_visible = m_colorParams.colorGrayscaleOptions().autoLayerEnabled();
            color_grayscale_options_visible = true;
            break;
    }
	
    illuminationPanel->setVisible(color_grayscale_options_visible);
	if (color_grayscale_options_visible) {
		ColorGrayscaleOptions const opt(
			m_colorParams.colorGrayscaleOptions()
		);
		whiteMarginsCB->setChecked(opt.whiteMargins());
        whiteMarginsCB->setEnabled(m_currentMode != ColorParams::MIXED); // Mixed must have margins
		equalizeIlluminationCB->setChecked(opt.normalizeIllumination());
		equalizeIlluminationCB->setEnabled(opt.whiteMargins());
	}
	
	modePanel->setVisible(m_lastTab != TAB_DEWARPING);
	pictureShapeOptions->setVisible(picture_shape_visible);
    layersPanel->setVisible(m_currentMode == ColorParams::MIXED);
    bwOptions->setVisible(bw_options_visible);
	despecklePanel->setVisible(bw_options_visible && m_lastTab != TAB_DEWARPING);

	if (picture_shape_visible) {
        updatePictureShapeValueText();
	}
	

	if (bw_options_visible) {
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

		ScopedIncDec<int> const guard(m_ignoreThresholdChanges);
        thresholdSlider->setValue(m_colorParams.blackWhiteOptions().thresholdAdjustment());
	}

    autoLayerCB->setEnabled(m_dewarpingMode == DewarpingMode::OFF);
    colorLayerCB->setEnabled(m_dewarpingMode == DewarpingMode::OFF);
    colorLayerCB->setCheckState(m_colorParams.colorGrayscaleOptions().colorLayerEnabled() && m_dewarpingMode == DewarpingMode::OFF? Qt::Checked : Qt::Unchecked);
    autoLayerCB->setCheckState(m_colorParams.colorGrayscaleOptions().autoLayerEnabled() || m_dewarpingMode != DewarpingMode::OFF? Qt::Checked : Qt::Unchecked);
}

void
OptionsWidget::updateDewarpingDisplay()
{
	depthPerceptionPanel->setVisible(m_lastTab == TAB_DEWARPING);

	switch (m_dewarpingMode) {
		case DewarpingMode::OFF:
            dewarpingStatusLabel->setText(Utils::richTextForLink(tr("Off")));
			break;
		case DewarpingMode::AUTO:
            dewarpingStatusLabel->setText(Utils::richTextForLink(tr("Auto")));
			break;
		case DewarpingMode::MANUAL:
            dewarpingStatusLabel->setText(Utils::richTextForLink(tr("Manual")));
			break;
//begin of modified by monday2000
//Marginal_Dewarping
		case DewarpingMode::MARGINAL:
            dewarpingStatusLabel->setText(Utils::richTextForLink(tr("Marginal")));
			break;
//end of modified by monday2000
	}

	depthPerceptionSlider->blockSignals(true);
	depthPerceptionSlider->setValue(qRound(m_depthPerception.value() * 10));
	depthPerceptionSlider->blockSignals(false);
}

void
OptionsWidget::updateDespeckleValueText()
{
    switch(m_despeckleLevel) {
    case DESPECKLE_OFF: despeckleValue->setText(tr("Off")); break;
    case DESPECKLE_CAUTIOUS: despeckleValue->setText(tr("Cautious")); break;
    case DESPECKLE_NORMAL: despeckleValue->setText(tr("Normal")); break;
    case DESPECKLE_AGGRESSIVE: despeckleValue->setText(tr("Aggresive")); break;
    default: ;
    }
}

} // namespace output

void output::OptionsWidget::on_depthPerceptionSlider_valueChanged(int value)
{
    m_depthPerception.setValue(0.1 * value);
    QString const tooltip_text(QString::number(m_depthPerception.value()));
    depthPerceptionSlider->setToolTip(tooltip_text);

    // Show the tooltip immediately.
    QPoint const center(depthPerceptionSlider->rect().center());
    QPoint tooltip_pos(depthPerceptionSlider->mapFromGlobal(QCursor::pos()));
    tooltip_pos.setY(center.y());
    tooltip_pos.setX(qBound(0, tooltip_pos.x(), depthPerceptionSlider->width()));
    tooltip_pos = depthPerceptionSlider->mapToGlobal(tooltip_pos);
    QToolTip::showText(tooltip_pos, tooltip_text, depthPerceptionSlider);

    depthPerceptionValue->setText(QString::number(0.1 * value));

    // Propagate the signal.
    emit depthPerceptionChanged(m_depthPerception.value());
}

void output::OptionsWidget::on_applyDepthPerception_linkActivated(const QString &/*link*/)
{
    ApplyColorsDialog* dialog = new ApplyColorsDialog(
        this, m_pageId, m_pageSelectionAccessor
    );
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Apply Depth Perception"));
    connect(
        dialog, SIGNAL(accepted(std::set<PageId> const&)),
        this, SLOT(applyDepthPerceptionConfirmed(std::set<PageId> const&))
    );
    dialog->show();
}

void output::OptionsWidget::on_dewarpingStatusLabel_linkActivated(const QString &/*link*/)
{
    ChangeDewarpingDialog* dialog = new ChangeDewarpingDialog(
        this, m_pageId, m_dewarpingMode, m_pageSelectionAccessor
    );
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(
        dialog, SIGNAL(accepted(std::set<PageId> const&, DewarpingMode const&)),
        this, SLOT(dewarpingChanged(std::set<PageId> const&, DewarpingMode const&))
    );
    dialog->show();
}

void output::OptionsWidget::on_applyDespeckleButton_linkActivated(const QString &/*link*/)
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

void output::OptionsWidget::on_applyColorsButton_linkActivated(const QString &/*link*/)
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


void output::OptionsWidget::on_modeValue_linkActivated(const QString &/*link*/)
{
    m_menuMode.popup(modeValue->mapToGlobal(QPoint(0,modeValue->geometry().height())));
}

void output::OptionsWidget::on_actionModeBW_triggered()
{
    modeValue->setText(Utils::richTextForLink(actionModeBW->toolTip()));
    changeColorMode(ColorParams::BLACK_AND_WHITE);
}

void output::OptionsWidget::on_actionModeColorOrGrayscale_triggered()
{
    modeValue->setText(Utils::richTextForLink(actionModeColorOrGrayscale->toolTip()));
    changeColorMode(ColorParams::COLOR_GRAYSCALE);
}

void output::OptionsWidget::on_actionModeMixed_triggered()
{
    modeValue->setText(Utils::richTextForLink(actionModeMixed->toolTip()));
    changeColorMode(ColorParams::MIXED);
}

void output::OptionsWidget::on_actionPictureShapeFree_triggered()
{
    changePictureShape(PictureShape::FREE_SHAPE);
}

void output::OptionsWidget::on_actionPictureShapeRectangular_triggered()
{
    changePictureShape(PictureShape::RECTANGULAR_SHAPE);
}

void output::OptionsWidget::on_actionPictureShapeQuadro_triggered()
{
    changePictureShape(PictureShape::QUADRO_SHAPE);
}

void output::OptionsWidget::on_pictureShapeValue_linkActivated(const QString &/*link*/)
{
    m_menuPictureShape.popup(pictureShapeValue->mapToGlobal(QPoint(0,pictureShapeValue->geometry().height())));
}

void output::OptionsWidget::on_despeckleSlider_valueChanged(int value)
{
    switch(value) {
    case 0: handleDespeckleLevelChange(DESPECKLE_OFF); break;
    case 1: handleDespeckleLevelChange(DESPECKLE_CAUTIOUS); break;
    case 2: handleDespeckleLevelChange(DESPECKLE_NORMAL); break;
    case 3: handleDespeckleLevelChange(DESPECKLE_AGGRESSIVE); break;
    default: ;
    }
}

int sum_y = 0;

bool output::OptionsWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (!(obj && event
          && (QString(obj->metaObject()->className()) == "QSlider"))) {
        return false;
    }

    if (m_ignore_system_wheel_settings && event->type() == QEvent::Wheel) {
        QWheelEvent* e = (QWheelEvent*) event;
        if (e->modifiers() == Qt::NoModifier) {
            const QPoint& angleDelta = e->angleDelta();
            if (!angleDelta.isNull()) {
                sum_y += angleDelta.y();
                if (abs(sum_y) >= 30) {
                    QSlider* slider= (QSlider*) obj;
                    int dy = (sum_y > 0) ? slider->singleStep() : -1*slider->singleStep();
                    slider->setValue(slider->value() + dy);
                    sum_y = 0;
                    e->accept();
                    return true;
                }
            }
        }
    } else if (event->type() == QEvent::Paint) {
        QSlider* slider= (QSlider*) obj;
        if (slider->minimum() <=0 && slider->maximum() >= 0) {
            int position = QStyle::sliderPositionFromValue(slider->minimum(),
                                                           slider->maximum(),
                                                           0,
                                                           slider->width());
            QPainter painter(slider);
            QPen p(painter.pen());
            p.setColor(QColor(Qt::blue));
            p.setWidth(3);
            painter.setPen(p);
            //        painter.drawText(QPointF(position-5, 0, position+5, slider->height()/2), "0");
            painter.drawLine(position, 0, position, slider->height()/2-6);
        }
    }

    return false;
}

void output::OptionsWidget::on_thresholdSlider_valueChanged()
{
    int value = thresholdSlider->value();
    QString const tooltip_text(QString::number(value));
    thresholdSlider->setToolTip(tooltip_text);

    thresholdLabel->setNum(value);

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
    m_ptrSettings->setColorParams(m_pageId, m_colorParams, ColorParamsApplyFilter::CopyThreshold);
    emit reloadRequested();

    emit invalidateThumbnail(m_pageId);
}

void output::OptionsWidget::on_dpiValue_linkActivated(const QString &/*link*/)
{
    ChangeDpiDialog* dialog = new ChangeDpiDialog(
        this, m_outputDpi, m_pageId, m_pageSelectionAccessor
    );
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(
        dialog, SIGNAL(accepted(std::set<PageId> const&, Dpi const&)),
        this, SLOT(dpiChanged(std::set<PageId> const&, Dpi const&))
    );
    dialog->show();
}

void output::OptionsWidget::on_actionReset_to_default_value_triggered()
{
    thresholdSlider->setValue(0);
}

void output::OptionsWidget::applyThresholdConfirmed(std::set<PageId> const& pages)
{
    BOOST_FOREACH(PageId const& page_id, pages) {
        m_ptrSettings->setColorParams(page_id, m_colorParams, ColorParamsApplyFilter::CopyThreshold);
        emit invalidateThumbnail(page_id);
    }

    if (pages.find(m_pageId) != pages.end()) {
        emit reloadRequested();
    }
}

void output::OptionsWidget::on_applyThresholdButton_linkActivated(const QString &/*link*/)
{
    ApplyColorsDialog* dialog = new ApplyColorsDialog(
        this, m_pageId, m_pageSelectionAccessor
    );
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Apply Threshold"));
    connect(
        dialog, SIGNAL(accepted(std::set<PageId> const&)),
        this, SLOT(applyThresholdConfirmed(std::set<PageId> const&))
    );
    dialog->show();
}
