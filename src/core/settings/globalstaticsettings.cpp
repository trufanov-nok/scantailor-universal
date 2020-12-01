/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) Joseph Artsimovich <joseph_a@mail.ru>

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

#include "globalstaticsettings.h"

QString GlobalStaticSettings::m_tiff_compr_method;
QStringList GlobalStaticSettings::m_tiff_list;
int GlobalStaticSettings::m_tiff_compression_id = 5;
bool GlobalStaticSettings::m_drawDeskewDeviants = false;
bool GlobalStaticSettings::m_drawContentDeviants = false;
bool GlobalStaticSettings::m_drawMarginDeviants = false;
bool GlobalStaticSettings::m_drawDeviants = false;
int  GlobalStaticSettings::m_currentStage = 0;
int  GlobalStaticSettings::m_binrization_threshold_control_default = 0;
bool GlobalStaticSettings::m_use_horizontal_predictor = false;
bool GlobalStaticSettings::m_disable_bw_smoothing = false;
qreal GlobalStaticSettings::m_zone_editor_min_angle = 3.0;
float GlobalStaticSettings::m_picture_detection_sensitivity = 100.;
QColor GlobalStaticSettings::m_deskew_controls_color;
QColor GlobalStaticSettings::m_deskew_controls_color_pen;
QColor GlobalStaticSettings::m_deskew_controls_color_thumb;
QColor GlobalStaticSettings::m_content_sel_content_color;
QColor GlobalStaticSettings::m_content_sel_content_color_pen;
QHotKeys GlobalStaticSettings::m_hotKeyManager = QHotKeys();
int  GlobalStaticSettings::m_highlightColorAdjustment = 140;

bool GlobalStaticSettings::m_thumbsListOrderAllowed = true;
int GlobalStaticSettings::m_thumbsMinSpacing = 3;
int GlobalStaticSettings::m_thumbsBoundaryAdjTop = 5;
int GlobalStaticSettings::m_thumbsBoundaryAdjBottom = 5;
int GlobalStaticSettings::m_thumbsBoundaryAdjLeft = 5;
int GlobalStaticSettings::m_thumbsBoundaryAdjRight = 3;
bool GlobalStaticSettings::m_fixedMaxLogicalThumbSize = false;
bool GlobalStaticSettings::m_displayOrderHints = true;

bool GlobalStaticSettings::m_dewarpAutoVertHalfCorrection = false;
bool GlobalStaticSettings::m_dewarpAutoDeskewAfterDewarp = false;
bool GlobalStaticSettings::m_simulateSelectionModifier = false;
bool GlobalStaticSettings::m_simulateSelectionModifierHintEnabled = true;
bool GlobalStaticSettings::m_inversePageOrder = false;

bool GlobalStaticSettings::m_DontUseNativeDialog = true;
