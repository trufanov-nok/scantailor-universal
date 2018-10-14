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

#ifndef GLOBALSTATICSETTINGS_H
#define GLOBALSTATICSETTINGS_H
#include "settings/hotkeysmanager.h"
#include "settings/ini_keys.h"
#include "settings/ini_keys.h"
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
        setDrawDeskewDeviants(settings.value(_key_deskew_deviant_enabled, _key_deskew_deviant_enabled_def).toBool());
        setDrawContentDeviants(settings.value(_key_select_content_deviant_enabled, _key_select_content_deviant_enabled_def).toBool());
        setDrawMarginDeviants(settings.value(_key_margins_deviant_enabled, _key_margins_deviant_enabled_def).toBool());

        m_binrization_threshold_control_default = settings.value(_key_output_bin_threshold_default, _key_output_bin_threshold_default_def).toInt();
        m_use_horizontal_predictor = settings.value(_key_tiff_compr_horiz_pred, _key_tiff_compr_horiz_pred_def).toBool();
        m_disable_bw_smoothing = settings.value(_key_mode_bw_disable_smoothing, _key_mode_bw_disable_smoothing_def).toBool();
        m_zone_editor_min_angle = settings.value(_key_zone_editor_min_angle, _key_zone_editor_min_angle_def).toReal();
        m_picture_detection_sensitivity = settings.value(_key_picture_zones_layer_sensitivity, _key_picture_zones_layer_sensitivity_def).toInt();

        m_highlightColorAdjustment = 100 + settings.value(_key_thumbnails_non_focused_selection_highlight_color_adj, _key_thumbnails_non_focused_selection_highlight_color_adj_def).toInt();

        m_thumbsListOrderAllowed = settings.value(_key_thumbnails_multiple_items_in_row, _key_thumbnails_multiple_items_in_row_def).toBool();
        m_thumbsMinSpacing = settings.value(_key_thumbnails_min_spacing, _key_thumbnails_min_spacing_def).toInt();
        m_thumbsBoundaryAdjTop = settings.value(_key_thumbnails_boundary_adj_top, _key_thumbnails_boundary_adj_top_def).toInt();
        m_thumbsBoundaryAdjBottom = settings.value(_key_thumbnails_boundary_adj_bottom, _key_thumbnails_boundary_adj_bottom_def).toInt();
        m_thumbsBoundaryAdjLeft = settings.value(_key_thumbnails_boundary_adj_left, _key_thumbnails_boundary_adj_left_def).toInt();
        m_thumbsBoundaryAdjRight = settings.value(_key_thumbnails_boundary_adj_right, _key_thumbnails_boundary_adj_right_def).toInt();
        m_fixedMaxLogicalThumbSize = settings.value(_key_thumbnails_fixed_thumb_size, _key_thumbnails_fixed_thumb_size_def).toBool();
        m_displayOrderHints = settings.value(_key_thumbnails_display_order_hints, _key_thumbnails_display_order_hints_def).toBool();

        m_drawDeskewOrientFix = settings.value(_key_deskew_orient_fix_enabled, _key_deskew_orient_fix_enabled_def).toBool();

        m_dewarpAutoVertHalfCorrection = settings.value(_key_dewarp_auto_vert_half_correction, _key_dewarp_auto_vert_half_correction_def).toBool();
        m_dewarpAutoDeskewAfterDewarp = settings.value(_key_dewarp_auto_deskew_after_dewarp, _key_dewarp_auto_deskew_after_dewarp_def).toBool();

        m_simulateSelectionModifierHintEnabled = settings.value(_key_thumbnails_simulate_key_press_hint, _key_thumbnails_simulate_key_press_hint_def).toBool();
    }    

    static void updateHotkeys()
    {
        m_hotKeyManager.load();
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
    static int m_highlightColorAdjustment;

    static bool m_thumbsListOrderAllowed;
    static int m_thumbsMinSpacing;
    static int m_thumbsBoundaryAdjTop;
    static int m_thumbsBoundaryAdjBottom;
    static int m_thumbsBoundaryAdjLeft;
    static int m_thumbsBoundaryAdjRight;
    static bool m_fixedMaxLogicalThumbSize;
    static bool m_displayOrderHints;

    static bool m_drawDeskewOrientFix;

    static bool m_dewarpAutoVertHalfCorrection;
    static bool m_dewarpAutoDeskewAfterDewarp;
    static bool m_simulateSelectionModifier;
    static bool m_simulateSelectionModifierHintEnabled;
    static bool m_inversePageOrder;
};

#endif // GLOBALSTATICSETTINGS_H
