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

#include "hotkeysmanager.h"
#include "Utils.h"
#include <memory>
#include <assert.h>
#include <QMessageBox>
#include "CommandLine.h"

const uint _KeySchemeVer = 3;
const uint _KeySchemeLastCompatibleVer = _KeySchemeVer;

QHotKeys::QHotKeys()
{
    load();
}

void QHotKeys::resetToDefaults()
{
    m_data.clear();

    QVector<HotKeyInfo> data;
    data.append(HotKeyInfo(ProjectNew, QObject::tr("New project"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_N)));
    data.append(HotKeyInfo(ProjectOpen, QObject::tr("Open project"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_O)));
    data.append(HotKeyInfo(ProjectSave, QObject::tr("Save project"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_S)));
    data.append(HotKeyInfo(ProjectSaveAs, QObject::tr("Save project as"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_S)));
    data.append(HotKeyInfo(ProjectClose, QObject::tr("Close project"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_W)));
    data.append(HotKeyInfo(AppQuit, QObject::tr("Quit"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_Q)));

    HotKeyGroup group_menu("menu_actions", QObject::tr("Main menu"));
    group_menu.setHotKeys(data);

    m_data.append(group_menu);

    data.clear();
    data.append(HotKeyInfo(StageFixOrientation, QObject::tr("Fix Orientation"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_1)));
    data.append(HotKeyInfo(StageSplitPages, QObject::tr("Split Pages"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_2)));
    data.append(HotKeyInfo(StageDeskew, QObject::tr("Deskew"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_3)));
    data.append(HotKeyInfo(StageSelectContent, QObject::tr("Select Content"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_4)));
    data.append(HotKeyInfo(StageMargins, QObject::tr("Page Layout"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_5)));
    data.append(HotKeyInfo(StageOutput, QObject::tr("Output"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_6)));

    HotKeyGroup group_stages("stage_navigation", QObject::tr("Stages navigation"));
    group_stages.setHotKeys(data);

    m_data.append(group_stages);

    QList<HotKeySequence> list;

    data.clear();
    data.append(HotKeyInfo(PageFirst, QObject::tr("First page"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Home)));
    data.append(HotKeyInfo(PageLast, QObject::tr("Last page"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_End)));
    list.append(HotKeySequence(Qt::NoModifier, Qt::Key_PageUp));
    list.append(HotKeySequence(Qt::NoModifier, Qt::Key_Q));
    data.append(HotKeyInfo(PagePrev, QObject::tr("Previous page"), KeysAndModifiers, HotKey, list));
    list.clear();
    list.append(HotKeySequence(Qt::NoModifier, Qt::Key_PageDown));
    list.append(HotKeySequence(Qt::NoModifier, Qt::Key_W));
    data.append(HotKeyInfo(PageNext, QObject::tr("Next page"), KeysAndModifiers, HotKey, list));

    data.append(HotKeyInfo(PageFirstSelected, QObject::tr("First selected page"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::AltModifier, Qt::Key_Home)));
    data.append(HotKeyInfo(PageLastSelected, QObject::tr("Last selected page"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::AltModifier, Qt::Key_End)));
    data.append(HotKeyInfo(PagePrevSelected, QObject::tr("Previous selected page"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::AltModifier, Qt::Key_PageUp)));
    data.append(HotKeyInfo(PageNextSelected, QObject::tr("Next selected page"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::AltModifier, Qt::Key_PageDown)));

    QSettings _sett;
    int pg_jmp = _sett.value(_key_hot_keys_jump_forward_pg_num, _key_hot_keys_jump_forward_pg_num_def).toUInt();
    data.append(HotKeyInfo(PageJumpForward, QObject::tr("Jump %n pages forward", "plurals for \"page\" may be used", pg_jmp), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_PageDown)));
    pg_jmp = _sett.value(_key_hot_keys_jump_backward_pg_num, _key_hot_keys_jump_backward_pg_num_def).toUInt();
    data.append(HotKeyInfo(PageJumpBackward, QObject::tr("Jump %n pages backward", "plurals for \"page\" may be used", pg_jmp), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_PageUp)));

    data.append(HotKeyInfo(ThumbSizeChange, QObject::tr("Change size of thumbnails"), ModifierAllowed, MouseWheel,
                           HotKeySequence(Qt::AltModifier, Qt::Key_unknown)));

    HotKeyGroup group_pages("page_navigation", QObject::tr("Pages navigation"));
    group_pages.setHotKeys(data);

    m_data.append(group_pages);

    data.clear();
    data.append(HotKeyInfo(InsertEmptyPageBefore, QObject::tr("Insert empty page before"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_B)));
    data.append(HotKeyInfo(InsertEmptyPageAfter, QObject::tr("Insert empty page after"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_A)));

    HotKeyGroup group_pages_manipulation("page_manipulation", QObject::tr("Pages manipulation"));
    group_pages_manipulation.setHotKeys(data);

    m_data.append(group_pages_manipulation);

    data.clear();

    data.append(HotKeyInfo(DeskewChange, QObject::tr("Change angle"), ModifierAllowed, MouseWheel,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(DeskewChangePrec, QObject::tr("Change angle precisely"), ModifierAllowed, MouseWheel,
                           HotKeySequence(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_unknown)));
    HotKeyGroup group_deskew("deskew", QObject::tr("Deskew"));
    group_deskew.setHotKeys(data);

    m_data.append(group_deskew);

    data.clear();

    data.append(HotKeyInfo(ContentMove, QObject::tr("Move content zone"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ContentMoveAxes, QObject::tr("Move along axes"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ContentStretch, QObject::tr("Stretch or squeeze"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ContentInsert, QObject::tr("Create zone"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Insert)));
    data.append(HotKeyInfo(ContentDelete, QObject::tr("Delete zone"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Delete)));
    data.append(HotKeyInfo(ContentMoveUp, QObject::tr("Move zone up"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Up)));
    data.append(HotKeyInfo(ContentMoveDown, QObject::tr("Move zone down"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Down)));
    data.append(HotKeyInfo(ContentMoveLeft, QObject::tr("Move zone left"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Left)));
    data.append(HotKeyInfo(ContentMoveRight, QObject::tr("Move zone right"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Right)));

    HotKeyGroup group_content("content_detection", QObject::tr("Content selection"));
    group_content.setHotKeys(data);

    m_data.append(group_content);

    data.clear();

    data.append(HotKeyInfo(PageViewZoomIn, QObject::tr("Zoom in"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Plus)));
    data.append(HotKeyInfo(PageViewZoomOut, QObject::tr("Zoom out"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Minus)));
    data.append(HotKeyInfo(PageViewDisplayOriginal, QObject::tr("Display original image"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Space)));
    data.append(HotKeyInfo(PageViewMoveNoRestrictions, QObject::tr("Move without constraints"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier, Qt::Key_unknown)));
    HotKeyGroup group_page_view("page_view", QObject::tr("General page view"));
    group_page_view.setHotKeys(data);

    m_data.append(group_page_view);

    data.clear();

    data.append(HotKeyInfo(ZoneRectangle, QObject::tr("Create rectangle zone"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ZoneEllipse, QObject::tr("Create ellipse zone"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ZoneMove, QObject::tr("Move zone"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ZoneMoveVertically, QObject::tr("Move zone vertically"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier | Qt::MetaModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ZoneMoveHorizontally, QObject::tr("Move zone horizontally"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ZonePaste, QObject::tr("Paste from clipboard"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_V)));
    data.append(HotKeyInfo(ZoneClone, QObject::tr("Clone last modified zone"), ModifierAllowed, MouseDoubleCLick,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
    list.clear();
    list.append(HotKeySequence(Qt::NoModifier, Qt::Key_Delete));
    list.append(HotKeySequence(Qt::NoModifier, Qt::Key_D));

    data.append(HotKeyInfo(ZoneDelete, QObject::tr("Delete zone"), KeysAndModifiers, HotKey, list));
    data.append(HotKeyInfo(ZoneCancel, QObject::tr("Cancel move or creation"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Escape)));

    HotKeyGroup group_zones("zones_editor", QObject::tr("Zones edit"));
    group_zones.setHotKeys(data);

    m_data.append(group_zones);

    data.clear();

    data.append(HotKeyInfo(DewarpingMoveVertically, QObject::tr("Move vertically"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(DewarpingMoveHorizontally, QObject::tr("Move horizontally"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier, Qt::Key_unknown)));
    list.clear();
    list.append(HotKeySequence(Qt::NoModifier, Qt::Key_Delete));
    list.append(HotKeySequence(Qt::NoModifier, Qt::Key_D));
    data.append(HotKeyInfo(DewarpingDeletePoint, QObject::tr("Delete control point"), KeysAndModifiers, HotKey, list));

    HotKeyGroup group_dewarping("dewarping", QObject::tr("Distortion model (dewarping)"));
    group_dewarping.setHotKeys(data);

    m_data.append(group_dewarping);

    data.clear();

    data.append(HotKeyInfo(DespeckleMode0, QObject::tr("Switch off despeckling"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Z)));
    data.append(HotKeyInfo(DespeckleMode1, QObject::tr("Switch to cautious mode"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_X)));
    data.append(HotKeyInfo(DespeckleMode2, QObject::tr("Switch to normal mode"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_C)));
    data.append(HotKeyInfo(DespeckleMode3, QObject::tr("Switch to aggressive mode"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_V)));
    HotKeyGroup group_despeckling("despeckling", QObject::tr("Despeckling"));
    group_despeckling.setHotKeys(data);

    m_data.append(group_despeckling);
}

void QHotKeys::mergeHotkeys(const QVector<HotKeyGroup>& new_data)
{
    for (const HotKeyGroup& new_grp : new_data) {
        bool grp_found = false;
        for (HotKeyGroup& old_grp : m_data) {
            if (old_grp.id() == new_grp.id()) {
                grp_found = true;

                for (const HotKeyInfo& new_hk : new_grp.hotKeys()) {
                    int idx = -1;
                    for (int i = 0; i < old_grp.hotKeys().count(); i++) {
                        if (new_hk.id() == old_grp.hotKeys()[i].id()) {
                            idx = i;
                            break;
                        }
                    }
                    if (idx == -1) {
                        old_grp.hotKeys().append(new_hk);
                    } else {
                        HotKeyInfo info = new_hk;
                        info.setTitle(old_grp.hotKeys()[idx].title()); // keep current title translation
                        old_grp.hotKeys().replace(idx, info);
                    }
                }
                break;
            }
        }
        if (!grp_found) {
            m_data.append(new_grp);
        }
    }
}

bool QHotKeys::load(QSettings* _settings)
{
    std::unique_ptr<QSettings> ptr;
    if (!_settings) {
        ptr.reset(new QSettings());
        _settings = ptr.get();
    }

    QSettings& settings = *_settings;

    resetToDefaults();
    QVector<HotKeyGroup> loaded_data;

    if (settings.contains(_key_hot_keys_scheme_ver)) {
        uint scheme = settings.value(_key_hot_keys_scheme_ver, 0).toUInt();
        if (scheme < _KeySchemeLastCompatibleVer) {
            // settings contains outdated hotkeys scheme and it shoud be reseted.
            // this might be needed if new HotKayIds were added into existing group
            QString warning = QObject::tr("The hotkeys scheme in your settings file is incompatible with current application version. Hotkeys settings will be reseted.");
            if (CommandLine::get().isGui()) {
                QMessageBox::warning(nullptr, QObject::tr("Scan Tailor Universal"), warning);
            } else {
                std::cerr << warning.toStdString().c_str() << std::endl;
            }
            return false;
        }
        uint count = settings.value(_key_hot_keys_cnt, 0).toUInt();
        for (uint i = 0; i < count; i++) {
            QString group_id = settings.value(QString(_key_hot_keys_group_id).arg(i), "").toString();
            HotKeyGroup grp(group_id, "");
            grp.load(settings);
            loaded_data.append(grp);
        }
        mergeHotkeys(loaded_data);
        return true;
    }
    return false;
}

void QHotKeys::save(QSettings* _settings) const
{
    std::unique_ptr<QSettings> ptr;
    if (!_settings) {
        ptr.reset(new QSettings());
        _settings = ptr.get();
    }

    QSettings& settings = *_settings;

    settings.setValue(_key_hot_keys_scheme_ver, _KeySchemeVer);
    settings.setValue(_key_hot_keys_cnt, m_data.count());

    for (int i = 0; i < m_data.count(); i++) {
        const HotKeyGroup& grp = m_data[i];
        settings.setValue(QString(_key_hot_keys_group_id).arg(i), grp.id());
        grp.save(settings);
    }
}

const QString QHotKeys::modifiersToString(const Qt::KeyboardModifiers modifiers)
{
    QString res;
    if (modifiers.testFlag(Qt::ControlModifier)) {
        res += "Ctrl";
    }
    if (modifiers.testFlag(Qt::AltModifier)) {
        if (!res.isEmpty()) {
            res += "+";
        }
        res += "Alt";
    }
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        if (!res.isEmpty()) {
            res += "+";
        }
        res += "Shift";
    }
    if (modifiers.testFlag(Qt::MetaModifier)) {
        if (!res.isEmpty()) {
            res += "+";
        }
        res += "Meta";
    }
    return res;
}

const QString QHotKeys::keysToString(const QList<Qt::Key>& keys)
{
    QString res;
    for (const Qt::Key& key : keys) {
        if (!res.isEmpty()) {
            res += "+";
        }
        res += QKeySequence(key).toString();
    }
    return res;
}

const QString QHotKeys::hotkeysToString(const Qt::KeyboardModifiers modifiers, const QList<Qt::Key>& keys)
{
    QString mods_s = modifiersToString(modifiers);
    QString keys_s = keysToString(keys);
    if (!mods_s.isEmpty() && !keys_s.isEmpty()) {
        mods_s += "+";
    }
    return mods_s + keys_s;
}

const QString QHotKeys::toDisplayableText() const
{
    QString res = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
                  "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">"
                  "p, li { white-space: pre-wrap; }</style></head><body>"
                  "<table border=\"0\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px;"
                  " margin-right:0px;\"  cellspacing=\"0\" cellpadding=\"0\">";

    for (const HotKeyGroup& grp : qAsConst(m_data)) {
        res += "<tr><td><b>" + grp.title() + "</b></td></tr>";
        for (const HotKeyInfo& keys : grp.hotKeys()) {
            res += "<tr><td>" + keys.title() + "</td>";
            QString keys_seq;
            bool add_or = false;
            int seq_pos = 0;
            for (const HotKeySequence& seq : keys.sequences()) {
                QString modifier = modifiersToString(seq.m_modifierSequence);
                QString key_seq;
                for (const Qt::Key& key : qAsConst(seq.m_keySequence)) {
                    if (key != Qt::Key_unknown) {
                        if (!key_seq.isEmpty()) {
                            key_seq += "+";
                        }
                        key_seq += QKeySequence(key).toString();
                    }
                }

                if (!(keys.editorType() & KeysAllowed)) {
                    key_seq += keys.hotKeyTypeToString(keys.displayType());
                }

                if (!modifier.isEmpty() && !key_seq.isEmpty()) {
                    modifier += "+";
                }

                if (add_or) {
                    keys_seq += QObject::tr(" or ");
                } else {
                    add_or = true;
                }
                keys_seq += Utils::richTextForLink(modifier + key_seq, QString::number(keys.id()) + "_" + QString::number(seq_pos++));
            }
            res += "<td>" + keys_seq + "</td></tr>";
        }
    }

    return res + "</table></body></html>";
}

const HotKeyInfo* QHotKeys::get(const HotKeysId& id) const
{
    for (const HotKeyGroup& grp : qAsConst(m_data)) {
        for (const HotKeyInfo& info : grp.hotKeys()) {
            if (info.id() == id) {
                return &info;
            }
        }
    }
    return nullptr;
}

bool QHotKeys::replace(const HotKeysId& id, const HotKeyInfo& new_val)
{
    for (HotKeyGroup& grp : m_data) {
        for (int i = 0; i < grp.hotKeys().count(); i++) {
            const HotKeyInfo& info = grp.hotKeys()[i];
            if (info.id() == id) {
                grp.hotKeys().replace(i, new_val);
                return true;
            }
        }
    }
    return false;
}

bool HotKeyGroup::load(const QSettings& settings)
{
    const QString grp_key("hot_keys_" + m_groupName + "/");
    if (m_groupName.isEmpty() || !settings.contains(grp_key + "count")) {
        return false;
    }

    m_groupTitle = settings.value(grp_key + "title", m_groupTitle).toString();

    const uint hotkeys_cnt = settings.value(grp_key + "count", 0).toUInt();

    for (uint i = 0; i < hotkeys_cnt; i++) {
        const QString hotkey_key = QString("%1key_%2/").arg(grp_key).arg(i);

        const HotKeysId id = (HotKeysId) settings.value(hotkey_key + "id", HotKeysId::DefaultValueId).toUInt();
        assert(id != HotKeysId::DefaultValueId);

        QString title = settings.value(hotkey_key + "title", QString()).toString();
        assert(!title.isEmpty());

        const KeyType editor_type = (KeyType) settings.value(hotkey_key + "editor_type", KeyType::DefaultValueKT).toUInt();
        assert(editor_type != KeyType::DefaultValueKT);

        const HotKeyType display_type = (HotKeyType) settings.value(hotkey_key + "display_type", HotKeyType::DefaultValueHK).toUInt();
        assert(display_type != HotKeyType::DefaultValueHK);

        QList<HotKeySequence> sequences;
        const uint seq_cnt = settings.value(hotkey_key + "count", 0).toUInt();
        for (uint j = 0; j < seq_cnt; j++) {
            const QString seq_key = QString("%1sequence_%2/").arg(hotkey_key).arg(j);
            Qt::KeyboardModifiers modifiers = Qt::NoModifier;
            if (settings.contains(seq_key + "modifiers")) {
                modifiers = (Qt::KeyboardModifiers) settings.value(seq_key + "modifiers", Qt::KeyboardModifierMask).toUInt();
                assert(modifiers != Qt::KeyboardModifierMask);
            }
            const uint keys_cnt = settings.value(seq_key + "count", 0).toUInt();
            QList<Qt::Key> keys;
            for (uint k = 0; k < keys_cnt; k++) {
                const QString key_key = QString("%1key_%2/").arg(seq_key).arg(k);
                if (settings.contains(key_key + "key")) {
                    Qt::Key key = (Qt::Key) settings.value(key_key + "key", Qt::Key_unknown).toUInt();
                    assert(key != Qt::Key_unknown);
                    keys.append(key);
                }
            }
            sequences.append(HotKeySequence(modifiers, keys));
        }

        HotKeyInfo key_info(id, title, editor_type, display_type, sequences);
        m_hotKeys.append(key_info);
    }

    return true;
}

void HotKeyGroup::save(QSettings& settings) const
{
    const QString grp_key("hot_keys_" + m_groupName + "/");
    settings.setValue(grp_key + "title", m_groupTitle);
    settings.setValue(grp_key + "count", m_hotKeys.count());

    int i = 0;
    for (const HotKeyInfo& key_info : qAsConst(m_hotKeys)) {
        const QString hotkey_key = QString("%1key_%2/").arg(grp_key).arg(i++);

        settings.setValue(hotkey_key + "id", key_info.id());
        settings.setValue(hotkey_key + "title", key_info.title());
        settings.setValue(hotkey_key + "editor_type", key_info.editorType());
        settings.setValue(hotkey_key + "display_type", key_info.displayType());

        uint seq_cnt = key_info.sequences().count();
        settings.setValue(hotkey_key + "count", seq_cnt);

        int j = 0;
        for (const HotKeySequence& seq : key_info.sequences()) {
            const QString seq_key = QString("%1sequence_%2/").arg(hotkey_key).arg(j++);
            settings.setValue(seq_key + "modifiers", (int) seq.m_modifierSequence);

            const uint keys_cnt = seq.m_keySequence.count();
            settings.setValue(seq_key + "count", keys_cnt);

            int k = 0;
            for (const Qt::Key& key : qAsConst(seq.m_keySequence)) {
                if (key != Qt::Key_unknown) {
                    const QString key_key = QString("%1key_%2/").arg(seq_key).arg(k++);
                    settings.setValue(key_key + "key", key);
                }
            }

        }
    }
}

const QString HotKeyInfo::hotKeyTypeToString(const HotKeyType& val)
{
    switch (val) {
    case MouseClick: return QString(QObject::tr("click"));
    case MouseHold: return QString(QObject::tr("mouse"));
    case MouseWheel: return QString(QObject::tr("wheel"));
    case MouseDoubleCLick: return QString(QObject::tr("double click"));
    default:
        return QString();
    }
}
