#ifndef GLOBALSTATICSETTINGS_H
#define GLOBALSTATICSETTINGS_H

#include <QSettings>

class GlobalStaticSettings
{
public:
    inline static bool doDrawDeviants()
    {
        return m_drawDeviants;
    }

    inline static void setDrawDeskewDeviants(bool val)
    {
        if (m_drawDeskewDeviants != val) {
            m_drawDeskewDeviants = val;
            updateParams();
        }
    }

    inline static void setDrawContentDeviants(bool val)
    {
        if (m_drawContentDeviants != val) {
            m_drawContentDeviants = val;
            updateParams();
        }
    }

    inline static void setDrawMarginDeviants(bool val)
    {
        if (m_drawMarginDeviants != val) {
            m_drawMarginDeviants = val;
            updateParams();
        }
    }

    inline static void stageChanged(int i)
    {
        if (m_currentStage != i) {
            m_currentStage = i;
            updateParams();
        }
    }

    static void updateSettings()
    {
        QSettings settings;
        setDrawDeskewDeviants(settings.value("deskew_deviant/enabled", false).toBool());
        setDrawContentDeviants(settings.value("select_content_deviant/enabled", false).toBool());
        setDrawMarginDeviants(settings.value("margins_deviant/enabled", false).toBool());

        m_binrization_threshold_control_default = settings.value("output/binrization_threshold_control_default", 0).toInt();
        m_use_horizontal_predictor = settings.value("tiff_compression/use_horizontal_predictor", false).toBool();
        m_disable_bw_smoothing = settings.value("mode_bw/disable_smoothing", false).toBool();
        m_zone_editor_min_angle = settings.value("zone_editor/min_angle", 3.0).toReal();        
        m_picture_detection_sensitivity = settings.value("picture_zones_layer/sensitivity", 100).toInt();        
    }

private:
    inline static void updateParams()
    {
        switch (m_currentStage) {
        case 2:
            m_drawDeviants = m_drawDeskewDeviants;
            break;
        case 3:
            m_drawDeviants = m_drawContentDeviants;
            break;
        case 4:
            m_drawDeviants = m_drawMarginDeviants;
            break;
        default:
            m_drawDeviants = false;
        }
    }

    GlobalStaticSettings() {}

    static bool m_drawDeviants;
    static bool m_drawDeskewDeviants;
    static bool m_drawContentDeviants;
    static bool m_drawMarginDeviants;
    static int m_currentStage;
public:
    static int m_binrization_threshold_control_default;
    static bool m_use_horizontal_predictor;
    static bool m_disable_bw_smoothing;
    static qreal m_zone_editor_min_angle;
    static float m_picture_detection_sensitivity;
};

#endif // GLOBALSTATICSETTINGS_H
