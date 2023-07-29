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

    ImageViewTab lastTab() const
    {
        return m_lastTab;
    }

    DepthPerception const& depthPerception() const
    {
        return m_depthPerception;
    }
signals:
    void despeckleLevelChanged(DespeckleLevel level, bool* handled);

    void depthPerceptionChanged(double val);
public slots:
    void tabChanged(ImageViewTab tab);

    void distortionModelChanged(dewarping::DistortionModel const& model);

    void settingsChanged();

    void disablePictureLayer();

    void copyZoneToPagesDlgRequest(void* zone);
    void deleteZoneFromPagesDlgRequest(void* zone);
private slots:

    void whiteMarginsToggled(bool checked);

    void equalizeIlluminationToggled(bool checked);

    void despeckleOffSelected();

    void despeckleCautiousSelected();

    void despeckleNormalSelected();

    void despeckleAggressiveSelected();

    void on_depthPerceptionSlider_valueChanged(int value);

    void on_applyDepthPerception_linkActivated(const QString&);

    void on_dewarpingStatusLabel_linkActivated(const QString&);

    void on_applyDespeckleButton_linkActivated(const QString&);

    void on_applyColorsButton_linkActivated(const QString&);

    void on_modeValue_linkActivated(const QString&);

    void on_actionModeBW_triggered();

    void on_actionModeColorOrGrayscale_triggered();

    void on_actionModeMixed_triggered();

    void on_despeckleSlider_valueChanged(int value);

    void on_thresholdSlider_valueChanged();

    void on_thresholdForegroundSlider_valueChanged();

    void thresholdMethodChanged(int idx);

    void thresholdWindowSizeChanged(int value);

    void thresholdCoefChanged(double value);

    void on_dpiValue_linkActivated(const QString&);

    void on_actionReset_to_default_value_triggered();

    void on_applyThresholdButton_linkActivated(const QString&);

    void on_pictureZonesLayerCB_toggled(bool checked);

    void on_foregroundLayerCB_toggled(bool checked);

    void on_autoLayerCB_toggled(bool checked);

    void on_applyForegroundThresholdButton_linkActivated(const QString& link);

    void on_actionReset_to_default_value_foeground_triggered();

    void on_actionactionDespeckleOff_triggered();

    void on_actionactionDespeckleCautious_triggered();

    void on_actionactionDespeckleNormal_triggered();

    void on_actionactionDespeckleAggressive_triggered();

private:

    void dpiChanged(std::set<PageId> const& pages, Dpi const& dpi);

    void applyColorsConfirmed(std::set<PageId> const& pages);

    void applyThresholdConfirmed(std::set<PageId> const& pages, ColorParamsApplyFilter const& paramFilter);

    void applyDespeckleConfirmed(std::set<PageId> const& pages);

    void dewarpingChanged(std::set<PageId> const& pages, DewarpingMode const& mode);

    void applyDepthPerceptionConfirmed(std::set<PageId> const& pages);

    void changeColorMode(ColorParams::ColorMode const mode);

    bool eventFilter(QObject* obj, QEvent* event);

    void handleDespeckleLevelChange(DespeckleLevel level);

    void reloadIfNecessary();

    void updateDpiDisplay();

    void updateColorsDisplay();

    void updateLayersDisplay();

    void updateDewarpingDisplay();

    void updateModeValueText();

    void updateDespeckleValueText();

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

    void updateShortcuts();

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
    bool m_ignore_system_wheel_settings;
};

} // namespace output

#endif
