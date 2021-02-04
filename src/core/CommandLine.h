/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    CommandLine - Interface for ScanTailor's parameters provided on CL.
    Copyright (C) 2011 Petr Kovar <pejuko@gmail.com>

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

#ifndef COMMANDLINE_H_
#define COMMANDLINE_H_

#include <QMap>
#include <QRectF>
#include <QStringList>
#include <iostream>

#include "Dpi.h"
#include "filters/page_split/LayoutType.h"
#include "filters/output/ColorParams.h"
#include "filters/output/Params.h"
#include "filters/output/DespeckleLevel.h"
#include "filters/output/DewarpingMode.h"
#include "filters/output/DepthPerception.h"
#include "filters/page_layout/Settings.h"
#include "filters/page_layout/Alignment.h"
#include "ImageFileInfo.h"
#include "AutoManualMode.h"
#include "Margins.h"
#include "Despeckle.h"

namespace page_layout
{
class Alignment;
}

/**
 * CommandLine is a singleton simulation.
 * use CommandLine::get() to get access to global class
 * use CommandLine::set(CommandLine const&) to set the global class
 */
class CommandLine
{
    // Member-wise copying is OK.
public:
    enum Orientation {TOP, LEFT, RIGHT, UPSIDEDOWN};

    static CommandLine const& get()
    {
        return m_globalInstance;
    }
    static void set(CommandLine const& cl);

    CommandLine(QStringList const& argv, bool g = true) : m_error(false), m_gui(g), m_global(false), m_defaultNull(false)
    {
        CommandLine::parseCli(argv);
    }

    bool isGui() const
    {
        return m_gui;
    }
    bool isVerbose() const
    {
        return contains("verbose");
    }
    bool isError() const
    {
        return m_error;
    }

    std::vector<ImageFileInfo> const& images() const
    {
        return m_images;
    }
    QString const& outputDirectory() const
    {
        return m_outputDirectory;
    }
    QString const& projectFile() const
    {
        return m_projectFile;
    }
    QString const& outputProjectFile() const
    {
        return m_outputProjectFile;
    }

    bool isContentDetectionEnabled() const
    {
        return !contains("disable-content-detection");
    }
    bool isPageDetectionEnabled() const
    {
        return (contains("enable-page-detection") ||
                QSettings().value(_key_content_sel_page_detection_enabled, _key_content_sel_page_detection_enabled_def).toBool());
    }
    bool isForcePageDetectionDisabled() const
    {
        return contains("force-disable-page-detection");
    }
    bool isFineTuningEnabled() const
    {
        if (contains("enable-fine-tuning")) {
            return true;
        } else {
            QSettings settings;
            if (isPageDetectionEnabled() &&
                    settings.value(_key_content_sel_page_detection_fine_tune_corners, _key_content_sel_page_detection_fine_tune_corners_def).toBool()) {
                return settings.value(_key_content_sel_page_detection_fine_tune_corners_is_on_by_def, _key_content_sel_page_detection_fine_tune_corners_is_on_by_def_def).toBool();
            }
            return false;
        }
    }
    bool isAutoMarginsEnabled() const
    {
        return contains("enable-auto-margins");
    }

    bool hasMargins(QString base = "margins") const;
    bool hasPageBorders() const
    {
        return (hasMargins("page-borders") ||
                (isPageDetectionEnabled() && QSettings().value(_key_content_sel_page_detection_borders, _key_content_sel_page_detection_borders_def).toBool()));
    }
    bool hasAlignment() const;
    bool hasOutputDpi() const;
    bool hasLanguage() const;

    bool hasHelp() const
    {
        return contains("help");
    }
    bool hasOutputProject() const
    {
        return contains("output-project") && !m_options["output-project"].isEmpty();
    }
    bool hasLayout() const
    {
        return contains("layout") && !m_options["layout"].isEmpty();
    }
    bool hasLayoutDirection() const
    {
        return contains("layout-direction") && !m_options["layout-direction"].isEmpty();
    }
    bool hasStartFilterIdx() const
    {
        return contains("start-filter") && !m_options["start-filter"].isEmpty();
    }
    bool hasEndFilterIdx() const
    {
        return contains("end-filter") && !m_options["end-filter"].isEmpty();
    }
    bool hasOrientation() const
    {
        return contains("orientation") && !m_options["orientation"].isEmpty();
    }
    bool hasDeskewAngle() const
    {
        return contains("rotate") && !m_options["rotate"].isEmpty();
    }
    bool hasDeskew() const
    {
        return contains("deskew") && !m_options["deskew"].isEmpty();
    }
    bool hasSkewDeviation() const
    {
        return contains("skew-deviation") && !m_options["skew-deviation"].isEmpty();
    }
    bool hasContentRect() const
    {
        return contains("content-box") && !m_options["content-box"].isEmpty();
    }
    bool hasContentDeviation() const
    {
        return contains("content-deviation") && !m_options["content-deviation"].isEmpty();
    }
    bool hasContentDetection() const
    {
        return ! contains("disable-content-detection");
    }
    bool hasContentText() const
    {
        return !contains("disable-content-text-mask");
    }
    bool hasColorMode() const
    {
        return contains("color-mode") && !m_options["color-mode"].isEmpty();
    }
    bool hasDefaultColorMode() const
    {
        return contains("default-color-mode") && !m_options["default-color-mode"].isEmpty();
    }
    bool hasAutoLayer() const
    {
        return contains("autoLayer");
    }
    bool hasPictureZonesLayer() const
    {
        return contains("pictureZonesLayer");
    }
    bool hasForegroundLayer() const
    {
        return contains("foregroundLayer");
    }
    bool hasWhiteMargins() const
    {
        return contains("white-margins");
    }
    bool hasNormalizeIllumination() const
    {
        return contains("normalize-illumination");
    }
    bool hasThreshold() const
    {
        return contains("threshold") && !m_options["threshold"].isEmpty();
    }
    bool hasDespeckle() const
    {
        return contains("despeckle") && !m_options["despeckle"].isEmpty();
    }
    bool hasDewarping() const
    {
        return contains("dewarping");
    }
    bool hasMatchLayoutTolerance() const
    {
        return contains("match-layout-tolerance") && !m_options["match-layout-tolerance"].isEmpty();
    }
    bool hasDepthPerception() const
    {
        return contains("depth-perception") && !m_options["depth-perception"].isEmpty();
    }
    bool hasTiffCompressionBW() const {
        return contains("tiff-compression") && !m_options["tiff-compression"].isEmpty();
    }
    bool hasTiffCompressionColor() const {
        return contains("tiff-compression-non-bw") && !m_options["tiff-compression-non-bw"].isEmpty();
    }
    bool hasTiffForceRGB() const
    {
        return contains("tiff-force-rgb");
    }
    bool hasTiffForceGrayscale() const
    {
        return contains("tiff-force-grayscale");
    }
    bool hasTiffForceKeepColorSpace() const
    {
        return contains("tiff-force-keep-color-space");
    }
    bool hasWindowTitle() const
    {
        return contains("window-title") && !m_options["window-title"].isEmpty();
    }
    bool hasPageDetectionBox() const
    {
        return ((contains("page-detection-box") && !m_options["page-detection-box"].isEmpty()) ||
                (isPageDetectionEnabled() && QSettings().value(_key_content_sel_page_detection_borders, _key_content_sel_page_detection_borders_def).toBool()));
    }
    bool hasPageDetectionTolerance() const
    {
        return contains("page-detection-tolerance") && !m_options["page-detection-tolerance"].isEmpty();
    }
    bool hasDisableCheckOutput() const
    {
        return contains("disable-check-output");
    }

    page_split::LayoutType getLayout() const
    {
        return m_layoutType;
    }
    Qt::LayoutDirection getLayoutDirection() const
    {
        return m_layoutDirection;
    }
    output::ColorParams::ColorMode getColorMode() const
    {
        return m_colorMode;
    }
    output::ColorParams::ColorMode getDefaultColorMode() const
    {
        return m_defaultColorMode;
    }
    bool getAutoLayer() const
    {
        return m_autoLayer;
    }
    bool getPictureZonesLayer() const
    {
        return m_pictureZonesLayer;
    }
    bool getForegroundLayer() const
    {
        return m_foregroundLayer;
    }
    Dpi getInputDpi() const
    {
        return m_dpi;
    }
    Dpi getOutputDpi() const
    {
        return m_outputDpi;
    }
    Dpi getDefaultOutputDpi() const
    {
        return m_defaultOutputDpi;
    }
    Margins getMargins() const
    {
        return m_margins;
    }
    Margins getDefaultMargins() const
    {
        return m_defaultMargins;
    }
    Margins getPageBorders() const
    {
        return m_pageBorders;
    }
    page_layout::Alignment getAlignment() const
    {
        return m_alignment;
    }
    Despeckle::Level getContentDetection() const
    {
        return m_contentDetection;
    }
    QRectF getContentRect() const
    {
        return m_contentRect;
    }
    double getContentDeviation() const
    {
        return m_contentDeviation;
    }
    Orientation getOrientation() const
    {
        return m_orientation;
    }
    int getThreshold() const
    {
        return m_threshold;
    }
    double getDeskewAngle() const
    {
        return m_deskewAngle;
    }
    AutoManualMode getDeskewMode() const
    {
        return m_deskewMode;
    }
    double getSkewDeviation() const
    {
        return m_skewDeviation;
    }
    int getStartFilterIdx() const
    {
        return m_startFilterIdx;
    }
    int getEndFilterIdx() const
    {
        return m_endFilterIdx;
    }
    output::DewarpingMode getDewarpingMode() const
    {
        return m_dewarpingMode;
    }
    output::DespeckleLevel getDespeckleLevel() const
    {
        return m_despeckleLevel;
    }
    output::DepthPerception getDepthPerception() const
    {
        return m_depthPerception;
    }
    float getMatchLayoutTolerance() const
    {
        return m_matchLayoutTolerance;
    }
    QString getTiffCompressionBW() const {
        return m_compressionBW;
    }
    QString getTiffCompressionColor() const {
        return m_compressionColor;
    }
    QString getLanguage() const
    {
        return m_language;
    }
    QString getWindowTitle() const
    {
        return m_windowTitle;
    }
    QSizeF getPageDetectionBox() const
    {
        return m_pageDetectionBox;
    }
    double getPageDetectionTolerance() const
    {
        return m_pageDetectionTolerance;
    }
    bool getDefaultNull() const
    {
        return m_defaultNull;
    }

    bool help()
    {
        return m_options.contains("help");
    }
    void printHelp();

    static void updateSettings();

private:
    CommandLine() : m_gui(true), m_global(false) {}

    static CommandLine m_globalInstance;
    bool m_error;
    bool m_gui;
    bool m_global;
    QString m_language;
    QString m_windowTitle;
    QSizeF m_pageDetectionBox;
    double m_pageDetectionTolerance;
    bool m_defaultNull;

    bool isGlobal()
    {
        return m_global;
    }
    void setGlobal()
    {
        m_global = true;
    }

    bool contains(QString const& key) const
    {
        return m_options.contains(key);
    }

    QMap<QString, QString> m_options;
    QString m_projectFile;
    QString m_outputProjectFile;
    std::vector<QFileInfo> m_files;
    std::vector<ImageFileInfo> m_images;
    QString m_outputDirectory;
    QString m_compressionBW;
    QString m_compressionColor;

    page_split::LayoutType m_layoutType;
    Qt::LayoutDirection m_layoutDirection;
    output::ColorParams::ColorMode m_colorMode;
    output::ColorParams::ColorMode m_defaultColorMode;
    bool m_autoLayer;
    bool m_pictureZonesLayer;
    bool m_foregroundLayer;
    Dpi m_dpi;
    Dpi m_outputDpi;
    Dpi m_defaultOutputDpi;
    Margins m_margins;
    Margins m_defaultMargins;
    Margins m_pageBorders;
    page_layout::Alignment m_alignment;
    Despeckle::Level m_contentDetection;
    QRectF m_contentRect;
    double m_contentDeviation;
    Orientation m_orientation;
    int m_threshold;
    double m_deskewAngle;
    AutoManualMode m_deskewMode;
    double m_skewDeviation;
    int m_startFilterIdx;
    int m_endFilterIdx;
    output::DewarpingMode m_dewarpingMode;
    output::DespeckleLevel m_despeckleLevel;
    output::DepthPerception m_depthPerception;
    float m_matchLayoutTolerance;

    bool parseCli(QStringList const& argv);
    void addImage(QString const& path);
    void setup();
    page_split::LayoutType fetchLayoutType();
    output::ColorParams::ColorMode fetchColorMode();
    output::ColorParams::ColorMode fetchDefaultColorMode();
    bool fetchAutoLayer();
    bool fetchForegroundLayer();
    bool fetchPictureZonesLayer();
    Qt::LayoutDirection fetchLayoutDirection();
    Dpi fetchDpi(QString oname = "dpi");
    MarginsWithAuto fetchMargins(QString base = "margins", MarginsWithAuto def = MarginsWithAuto(true));
    Margins fetchPageBorders()
    {
        Margins res = (Margins)fetchMargins("page-borders", MarginsWithAuto(0, 0, 0, 0, true));
        if (res == Margins(0, 0, 0, 0) && hasPageBorders()) {
            QSettings settings;
            res = Margins(settings.value(_key_content_sel_page_detection_borders_left, _key_content_sel_page_detection_borders_left_def).toDouble(),
                          settings.value(_key_content_sel_page_detection_borders_top, _key_content_sel_page_detection_borders_top_def).toDouble(),
                          settings.value(_key_content_sel_page_detection_borders_right, _key_content_sel_page_detection_borders_right).toDouble(),
                          settings.value(_key_content_sel_page_detection_borders_bottom, _key_content_sel_page_detection_borders_bottom_def).toDouble());
        }
        return res;
    }
    page_layout::Alignment fetchAlignment();
    Despeckle::Level fetchContentDetection();
    QRectF fetchContentRect();
    double fetchContentDeviation();
    Orientation fetchOrientation();
    QString fetchOutputProjectFile();
    int fetchThreshold();
    double fetchDeskewAngle();
    AutoManualMode fetchDeskewMode();
    double fetchSkewDeviation();
    int fetchStartFilterIdx();
    int fetchEndFilterIdx();
    output::DewarpingMode fetchDewarpingMode();
    output::DespeckleLevel fetchDespeckleLevel();
    output::DepthPerception fetchDepthPerception();
    float fetchMatchLayoutTolerance();
    QString fetchCompressionBW() const;
    QString fetchCompressionColor() const;
    QString fetchLanguage() const;
    QString fetchWindowTitle() const;
    QSizeF fetchPageDetectionBox() const;
    double fetchPageDetectionTolerance() const;
    bool fetchDefaultNull();
};

#endif
