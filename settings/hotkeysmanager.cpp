#include "hotkeysmanager.h"
#include "Utils.h"
#include <memory>
#include <assert.h>
#include <QMessageBox>
#include "CommandLine.h"

const uint _KeySchemeVer = 1;
const uint _KeySchemeLastCompatibleVer = _KeySchemeVer;

void QHotKeys::resetToDefaults()
{
    m_data.clear();

    QVector<HotKeyInfo> data;
    data.append(HotKeyInfo(ProjectNew, QObject::tr("New project\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_N)));
    data.append(HotKeyInfo(ProjectOpen, QObject::tr("Open project\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_O)));
    data.append(HotKeyInfo(ProjectSave, QObject::tr("Save project\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_S)));
    data.append(HotKeyInfo(ProjectSaveAs, QObject::tr("Save project as\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier|Qt::ShiftModifier, Qt::Key_S)));
    data.append(HotKeyInfo(ProjectClose, QObject::tr("Close project\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_W)));
    data.append(HotKeyInfo(AppQuit, QObject::tr("Quit\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_Q)));

    HotKeyGroup group_menu("menu_actions", QObject::tr("Main menu"));
    group_menu.setHotKeys(data);

    m_data.append(group_menu);

    data.clear();
    data.append(HotKeyInfo(StageFixOrientation, QObject::tr("Fix Orientation\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_1)));
    data.append(HotKeyInfo(StageSplitPages, QObject::tr("Split Pages\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_2)));
    data.append(HotKeyInfo(StageDeskew, QObject::tr("Deskew\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_3)));
    data.append(HotKeyInfo(StageSelectContent, QObject::tr("Select Content\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_4)));
    data.append(HotKeyInfo(StageMargins, QObject::tr("Margins\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_5)));
    data.append(HotKeyInfo(StageOutput, QObject::tr("Output\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_6)));

    HotKeyGroup group_stages("stage_navigation", QObject::tr("Satges navigation"));
    group_stages.setHotKeys(data);

    m_data.append(group_stages);

	QVector<HotKeySequence> vec;

    data.clear();
    data.append(HotKeyInfo(PageFirst, QObject::tr("First page\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Home)));
    data.append(HotKeyInfo(PageLast, QObject::tr("Last page\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_End)));
	vec.append(HotKeySequence(Qt::NoModifier, Qt::Key_PageUp));
	vec.append(HotKeySequence(Qt::NoModifier, Qt::Key_Q));
    data.append(HotKeyInfo(PagePrev, QObject::tr("Previous page\t"), KeysAndModifiers, HotKey, vec));
	vec.clear();
	vec.append(HotKeySequence(Qt::NoModifier, Qt::Key_PageDown));
	vec.append(HotKeySequence(Qt::NoModifier, Qt::Key_W));
    data.append(HotKeyInfo(PageNext, QObject::tr("Next page\t\t"), KeysAndModifiers, HotKey, vec));

    HotKeyGroup group_pages("page_navigation", QObject::tr("Pages navigation"));
    group_pages.setHotKeys(data);

    m_data.append(group_pages);

    data.clear();

    data.append(HotKeyInfo(DeskewChange, QObject::tr("Change angle\t"), ModifierAllowed, MouseWheel,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(DeskewChangePrec, QObject::tr("Change angle precisely\t"), ModifierAllowed, MouseWheel,
                           HotKeySequence(Qt::ControlModifier|Qt::ShiftModifier, Qt::Key_unknown)));
    HotKeyGroup group_deskew("deskew", QObject::tr("Deskew"));
    group_deskew.setHotKeys(data);

    m_data.append(group_deskew);

    data.clear();

    data.append(HotKeyInfo(ContentMove, QObject::tr("Move content zone\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ContentMoveAxes, QObject::tr("Move along axes\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ContentStretch, QObject::tr("Stretch or sqeeze\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier|Qt::ShiftModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ContentInsert, QObject::tr("Create zone\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Insert)));
    data.append(HotKeyInfo(ContentDelete, QObject::tr("Delete zone\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Delete)));
    data.append(HotKeyInfo(ContentMoveUp, QObject::tr("Move zone up\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Up)));
    data.append(HotKeyInfo(ContentMoveDown, QObject::tr("Move zone down\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Down)));
    data.append(HotKeyInfo(ContentMoveLeft, QObject::tr("Move zone left\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Left)));
    data.append(HotKeyInfo(ContentMoveRight, QObject::tr("Move zone right\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Right)));

    HotKeyGroup group_content("content_detection", QObject::tr("Content selection"));
    group_content.setHotKeys(data);

    m_data.append(group_content);

    data.clear();

    data.append(HotKeyInfo(PageViewZoomIn, QObject::tr("Zoom in\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Plus)));
    data.append(HotKeyInfo(PageViewZoomOut, QObject::tr("Zoom out\t\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Minus)));
    data.append(HotKeyInfo(PageViewDisplayOriginal, QObject::tr("Display original image\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Space)));
    data.append(HotKeyInfo(PageViewMoveNoRestrictions, QObject::tr("Move without constraints\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier, Qt::Key_unknown)));
    HotKeyGroup group_page_view("page_view", QObject::tr("General page view"));
    group_page_view.setHotKeys(data);

    m_data.append(group_page_view);

    data.clear();

    data.append(HotKeyInfo(ZoneRectangle, QObject::tr("Create rectangle zone\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ZoneMove, QObject::tr("Move zone\t\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ZoneMoveVertically, QObject::tr("Move zone vertically\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier|Qt::MetaModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ZoneMoveHorizontally, QObject::tr("Move zone horyzontally\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier|Qt::ShiftModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(ZonePaste, QObject::tr("Paste from clipboard\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_V)));
    data.append(HotKeyInfo(ZoneClone, QObject::tr("Clone last modified zone\t"), ModifierAllowed, MouseDoubleCLick,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
	vec.clear();
	vec.append(HotKeySequence(Qt::NoModifier, Qt::Key_Delete));
	vec.append(HotKeySequence(Qt::NoModifier, Qt::Key_D));


    data.append(HotKeyInfo(ZoneDelete, QObject::tr("Delete zone\t\t"), KeysAndModifiers, HotKey, vec));
    data.append(HotKeyInfo(ZoneCancel, QObject::tr("Cancel move or creation\t"), KeysAndModifiers, HotKey,
                           HotKeySequence(Qt::NoModifier, Qt::Key_Escape)));

    HotKeyGroup group_zones("zones_editor", QObject::tr("Zones edit"));
    group_zones.setHotKeys(data);

    m_data.append(group_zones);

    data.clear();

    data.append(HotKeyInfo(DewarpingMoveVertically, QObject::tr("Move vertically\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ControlModifier, Qt::Key_unknown)));
    data.append(HotKeyInfo(DewarpingMoveHorizontally, QObject::tr("Move horizontally\t"), ModifierAllowed, MouseHold,
                           HotKeySequence(Qt::ShiftModifier, Qt::Key_unknown)));
	vec.clear();
	vec.append(HotKeySequence(Qt::NoModifier, Qt::Key_Delete));
	vec.append(HotKeySequence(Qt::NoModifier, Qt::Key_D));
    data.append(HotKeyInfo(DewarpingDeletePoint, QObject::tr("Delete control point\t"), KeysAndModifiers, HotKey, vec));

    HotKeyGroup group_dewarping("dewarping", QObject::tr("Distortion model (dewarping)"));
    group_dewarping.setHotKeys(data);

    m_data.append(group_dewarping);
}

bool QHotKeys::load(QSettings *_settings)
{
    std::unique_ptr<QSettings> ptr;
    if (!_settings) {
        ptr.reset(new QSettings());
        _settings = ptr.get();
    }

    QSettings& settings = *_settings;

    m_data.clear();

    if (settings.contains("hot_keys/scheme_ver")) {
        uint scheme = settings.value("hot_keys/scheme_ver", 0).toUInt();
        if (scheme < _KeySchemeLastCompatibleVer) {
            // settings contains outdated hotkeys scheme and it shoud be resetted.
            // this might be needed if new HotKayIds were added into existing group
            QString warning = QObject::tr("The hotkeys scheme in your settigs file is incompatible with current application version. Hotkeys settings will be resetted.");
            if (CommandLine::get().isGui()) {
                QMessageBox::warning(nullptr, QObject::tr("Scan Tailor Universal"), warning);
            } else {
                std::cerr << warning.toStdString().c_str() << std::endl;
            }
            return false;
        }
        uint count = settings.value("hot_keys/count", 0).toUInt();
        for (uint i = 0; i < count; i++) {
            QString group_id = settings.value(QString("hot_keys/group_%1").arg(i), "").toString();
            HotKeyGroup grp(group_id, "");
            grp.load(settings);
            m_data.append(grp);
        }
        return true;
    }
    return false;
}

void QHotKeys::save(QSettings *_settings) const
{
    std::unique_ptr<QSettings> ptr;
    if (!_settings) {
        ptr.reset(new QSettings());
        _settings = ptr.get();
    }

    QSettings& settings = *_settings;

    settings.setValue("hot_keys/scheme_ver", _KeySchemeVer);
    settings.setValue("hot_keys/count", m_data.count());

    for (int i = 0; i < m_data.count(); i++) {
        const HotKeyGroup& grp = m_data[i];
        settings.setValue(QString("hot_keys/group_%1").arg(i), grp.id());
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

const QString QHotKeys::keysToString(const QVector<Qt::Key>& keys)
{
   QString res;
   for (const Qt::Key& key: keys) {
       if (!res.isEmpty()) {
           res += "+";
       }
       res += QKeySequence(key).toString();
   }
   return res;
}

const QString QHotKeys::hotkeysToString(const Qt::KeyboardModifiers modifiers, const QVector<Qt::Key>& keys)
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
    QString res = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\"> \
            <html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\"> \
            p, li { white-space: pre-wrap; margin-top:1px; margin-bottom:1px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;} \
            </style></head><body>";

    for (HotKeyGroup grp: m_data) {
        res += "<p><b>" + grp.title() + "</b></p>";
        for (HotKeyInfo keys: grp.hotKeys()) {
            res += "<p>"+ keys.title();
            QString keys_seq;
            bool add_or = false;
            int seq_pos = 0;
            for (HotKeySequence seq: keys.sequences()) {
                QString modifier = modifiersToString(seq.m_modifierSequence);
                QString key_seq;
                for (Qt::Key key: seq.m_keySequence) {
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
            res += keys_seq + "</p>";
        }
    }

    return res + "</body></html>";
}

const HotKeyInfo* QHotKeys::get( const HotKeysId& id) const
{
    for (const HotKeyGroup& grp: m_data) {
        for (const HotKeyInfo& info: grp.hotKeys()) {
            if (info.id() == id) {
                return &info;
            }
        }
    }
    return nullptr;
}

bool QHotKeys::replace(const HotKeysId& id, const HotKeyInfo& new_val)
{
    for (HotKeyGroup& grp: m_data) {
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


bool HotKeyGroup::load(const QSettings &settings)
{
    const QString grp_key("hot_keys_"+m_groupName+"/");
    if (m_groupName.isEmpty() || !settings.contains(grp_key+"count")) {
        return false;
    }

    m_groupTitle = settings.value(grp_key+"title", m_groupTitle).toString();

    const uint hotkeys_cnt = settings.value(grp_key+"count", 0).toUInt();

    for (uint i = 0; i < hotkeys_cnt; i++) {
        const QString hotkey_key = QString("%1key_%2/").arg(grp_key).arg(i);

        const HotKeysId id = (HotKeysId) settings.value(hotkey_key+"id", HotKeysId::DefaultValueId).toUInt();
        assert(id != HotKeysId::DefaultValueId);

        QString title = settings.value(hotkey_key+"title", QString()).toString();
        assert(!title.isEmpty());

        const KeyType editor_type = (KeyType) settings.value(hotkey_key+"editor_type", KeyType::DefaultValueKT).toUInt();
        assert(editor_type != KeyType::DefaultValueKT);

        const HotKeyType display_type = (HotKeyType) settings.value(hotkey_key+"display_type", HotKeyType::DefaultValueHK).toUInt();
        assert(display_type != HotKeyType::DefaultValueHK);

        QVector<HotKeySequence> sequences;
        const uint seq_cnt = settings.value(hotkey_key+"count", 0).toUInt();
        for (uint j = 0; j < seq_cnt; j++) {
            const QString seq_key = QString("%1sequence_%2/").arg(hotkey_key).arg(j);
            Qt::KeyboardModifiers modifiers = Qt::NoModifier;
            if (settings.contains(seq_key+"modifiers")) {
                modifiers = (Qt::KeyboardModifiers) settings.value(seq_key+"modifiers", Qt::KeyboardModifierMask).toUInt();
                assert(modifiers != Qt::KeyboardModifierMask);
            }
            const uint keys_cnt = settings.value(seq_key+"count", 0).toUInt();
            QVector<Qt::Key> keys;
            for (uint k = 0; k < keys_cnt; k++) {
                const QString key_key = QString("%1key_%2/").arg(seq_key).arg(k);
                if (settings.contains(key_key+"key")) {
                    Qt::Key key = (Qt::Key) settings.value(key_key+"key", Qt::Key_unknown).toUInt();
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

void HotKeyGroup::save(QSettings &settings) const
{
    const QString grp_key("hot_keys_"+m_groupName+"/");
    settings.setValue(grp_key+"title", m_groupTitle);
    settings.setValue(grp_key+"count", m_hotKeys.count());

    int i = 0;
    for (const HotKeyInfo key_info: m_hotKeys) {
        const QString hotkey_key = QString("%1key_%2/").arg(grp_key).arg(i++);

        settings.setValue(hotkey_key+"id", key_info.id());
        settings.setValue(hotkey_key+"title", key_info.title());
        settings.setValue(hotkey_key+"editor_type", key_info.editorType());
        settings.setValue(hotkey_key+"display_type", key_info.displayType());

        uint seq_cnt = key_info.sequences().count();
        settings.setValue(hotkey_key+"count", seq_cnt);

        int j = 0;
        for (const HotKeySequence seq: key_info.sequences()) {
            const QString seq_key = QString("%1sequence_%2/").arg(hotkey_key).arg(j++);
            settings.setValue(seq_key+"modifiers", (int) seq.m_modifierSequence);

            const uint keys_cnt = seq.m_keySequence.count();
            settings.setValue(seq_key+"count", keys_cnt);

            int k = 0;
            for (Qt::Key key: seq.m_keySequence) {
                if (key != Qt::Key_unknown) {
                    const QString key_key = QString("%1key_%2/").arg(seq_key).arg(k++);
                    settings.setValue(key_key+"key", key);
                }
            }

        }
    }
}

const QString HotKeyInfo::hotKeyTypeToString(const HotKeyType &val)
{
    switch (val) {
    case MouseClick: return QString(QObject::tr("Click"));
    case MouseHold: return QString(QObject::tr("Mouse hold"));
    case MouseWheel: return QString(QObject::tr("Wheel"));
    case MouseDoubleCLick: return QString(QObject::tr("Double click"));
    default:
        return QString();
    }
}
