#ifndef GLOBALSTATICSETTINGS_H
#define GLOBALSTATICSETTINGS_H
#include "settings/hotkeysmanager.h"
#include <QSettings>
#include <memory>

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
        if (settings.contains("foreground_layer_adj_override")) {
           m_ForegroundLayerAdjustment.reset(new int (settings.value("foreground_layer_adj_override", 0).toInt()));
        }

    }    

    static void updateHotkeys()
    {
        if (!m_hotKeyManager.load()) {
            m_hotKeyManager.resetToDefaults();
        }
    }

    static QKeySequence createShortcut(const HotKeysId& id, int idx = 0)
    {
        const HotKeySequence& seq = m_hotKeyManager.get(id)->sequences()[idx];
        QString str = QHotKeys::hotkeysToString(seq.m_modifierSequence, seq.m_keySequence);
        return QKeySequence(str);
    }

    static QString getShortcutText(const HotKeysId& id, int idx = 0)
    {
        const HotKeySequence& seq = m_hotKeyManager.get(id)->sequences()[idx];
        return QHotKeys::hotkeysToString(seq.m_modifierSequence, seq.m_keySequence);
    }

    static bool checkKeysMatch(const HotKeysId& id, const Qt::KeyboardModifiers& modifiers, const Qt::Key& key)
    {
        const HotKeyInfo* i = m_hotKeyManager.get(id);
        for (const HotKeySequence& seq: i->sequences()) {
            if (seq.m_modifierSequence == modifiers &&
                    seq.m_keySequence.count() == 1 &&
                    seq.m_keySequence[0] == key) {
                return true;
            }
        }
        return false;
    }

    static bool checkModifiersMatch(const HotKeysId& id, const Qt::KeyboardModifiers& modifiers)
    {
        if (modifiers == Qt::NoModifier ) {
            return false;
        }

        const HotKeyInfo* i = m_hotKeyManager.get(id);
        for (const HotKeySequence& seq: i->sequences()) {
            if (seq.m_modifierSequence == modifiers) {
                return true;
            }
        }
        return false;
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
    static QHotKeys m_hotKeyManager;
    static std::unique_ptr<int> m_ForegroundLayerAdjustment;
};

#endif // GLOBALSTATICSETTINGS_H
