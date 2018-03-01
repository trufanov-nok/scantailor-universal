#include "globalstaticsettings.h"

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
QHotKeys GlobalStaticSettings::m_hotKeyManager = QHotKeys();
std::unique_ptr<int> GlobalStaticSettings::m_ForegroundLayerAdjustment = nullptr;
int  GlobalStaticSettings::m_highlightColorAdjustment = 140;

bool GlobalStaticSettings::m_thumbsListOrderAllowed = true;
int GlobalStaticSettings::m_thumbsMinSpacing = 3;
int GlobalStaticSettings::m_thumbsBoundaryAdjTop = 5;
int GlobalStaticSettings::m_thumbsBoundaryAdjBottom = 5;
int GlobalStaticSettings::m_thumbsBoundaryAdjLeft = 5;
int GlobalStaticSettings::m_thumbsBoundaryAdjRight = 3;
bool GlobalStaticSettings::m_fixedMaxLogicalThumbSize = false;
