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

#ifndef OUTPUT_OPTIONSWIDGET_H_
#define OUTPUT_OPTIONSWIDGET_H_

#include "ui_OutputOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "IntrusivePtr.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "ColorParams.h"
#include "DewarpingMode.h"
#include "DepthPerception.h"
#include "DespeckleLevel.h"
#include "Dpi.h"
#include "ImageViewTab.h"
#include "Params.h"
#include <set>
//begin of modified by monday2000
//Picture_Shape
#include "Params.h"
//end of modified by monday2000
#include <QMenu>

namespace dewarping
{
	class DistortionModel;
}

namespace output
{

class Settings;
class DewarpingParams;

class OptionsWidget
	: public FilterOptionsWidget, private Ui::OutputOptionsWidget
{
	Q_OBJECT
public:
	OptionsWidget(IntrusivePtr<Settings> const& settings,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~OptionsWidget();
	
	void preUpdateUI(PageId const& page_id);
	
	void postUpdateUI();

	ImageViewTab lastTab() const { return m_lastTab; }

    DepthPerception const& depthPerception() const { return m_depthPerception; }
signals:
	void despeckleLevelChanged(DespeckleLevel level, bool* handled);

	void depthPerceptionChanged(double val);
public slots:
	void tabChanged(ImageViewTab tab);

	void distortionModelChanged(dewarping::DistortionModel const& model);
private slots:
	
	void dpiChanged(std::set<PageId> const& pages, Dpi const& dpi);

	void applyColorsConfirmed(std::set<PageId> const& pages);

    void applyThresholdConfirmed(std::set<PageId> const& pages);

    void changeColorMode(ColorParams::ColorMode const mode);

    void changePictureShape(PictureShape const shape);

    void colorLayerCBToggled(bool checked);

    void autoLayerCBToggled(bool checked);
	
	void whiteMarginsToggled(bool checked);
	
	void equalizeIlluminationToggled(bool checked);	

    void despeckleOffSelected();

    void despeckleCautiousSelected();

    void despeckleNormalSelected();

    void despeckleAggressiveSelected();

	void applyDespeckleConfirmed(std::set<PageId> const& pages);

	void dewarpingChanged(std::set<PageId> const& pages, DewarpingMode const& mode);

	void applyDepthPerceptionConfirmed(std::set<PageId> const& pages);

    void on_depthPerceptionSlider_valueChanged(int value);

    void on_applyDepthPerception_linkActivated(const QString &);

    void on_dewarpingStatusLabel_linkActivated(const QString &);

    void on_applyDespeckleButton_linkActivated(const QString &);

    void on_applyColorsButton_linkActivated(const QString &);

    void on_modeValue_linkActivated(const QString &);

    void on_actionModeBW_triggered();

    void on_actionModeColorOrGrayscale_triggered();

    void on_actionModeMixed_triggered();

    void on_actionPictureShapeFree_triggered();

    void on_actionPictureShapeRectangular_triggered();

    void on_actionPictureShapeQuadro_triggered();

    void on_pictureShapeValue_linkActivated(const QString &);

    void on_despeckleSlider_valueChanged(int value);

    void on_thresholdSlider_valueChanged();

    void on_dpiValue_linkActivated(const QString &);

    void on_actionReset_to_default_value_triggered();

    void on_applyThresholdButton_linkActivated(const QString &);

private:
    bool eventFilter(QObject *obj, QEvent *event);

	void handleDespeckleLevelChange(DespeckleLevel level);

	void reloadIfNecessary();

	void updateDpiDisplay();

	void updateColorsDisplay();

	void updateDewarpingDisplay();

    void updateModeValueText();

    void updatePictureShapeValueText();

    void updateDespeckleValueText();

    void setCurrentPictureShape(PictureShape v)
    {
        m_currentPictureShape = v;
        updatePictureShapeValueText();
    }

    void setModeValue(ColorParams::ColorMode v)
    {
        m_currentMode = v;
        updateModeValueText();
    }

    void setDespeckleLevel(DespeckleLevel v)
    {
        m_despeckleLevel = v;
        if (despeckleSlider->value() != (int)v) {
            despeckleSlider->setValue((int)v);
        }
        updateDespeckleValueText();
    }
	
	IntrusivePtr<Settings> m_ptrSettings;
	PageSelectionAccessor m_pageSelectionAccessor;
	PageId m_pageId;
	Dpi m_outputDpi;
	ColorParams m_colorParams;
	DepthPerception m_depthPerception;
	DewarpingMode m_dewarpingMode;
	DespeckleLevel m_despeckleLevel;
	ImageViewTab m_lastTab;
	int m_ignoreThresholdChanges;
    QMenu m_menuMode;
    ColorParams::ColorMode m_currentMode;
    QMenu m_menuPictureShape;
    PictureShape m_currentPictureShape;
    bool m_ignore_system_wheel_settings;
};

} // namespace output

#endif
