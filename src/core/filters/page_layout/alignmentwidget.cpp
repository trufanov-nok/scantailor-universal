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

#include "alignmentwidget.h"
#include "ui_alignmentwidget.h"
#include "../Utils.h"
#include <QStandardItemModel>
#include <QPainter>

AlignmentWidget::AlignmentWidget(QWidget* parent, Alignment* alignment) :
    QWidget(parent), ui(new Ui::AlignmentWidget), m_alignment(alignment),
    m_useAutoMagnetAlignment(false), m_useOriginalProportionsAlignment(false),
    m_advancedAlignment(0)
{
    ui->setupUi(this);

    ui->panelAdvancedAlignment->setVisible(m_useAutoMagnetAlignment || m_useOriginalProportionsAlignment);

    markAlignmentButtons();

    Utils::mapSetValue(
        m_alignmentByButton, ui->alignTopLeftBtn,
        Alignment(Alignment::TOP, Alignment::LEFT)
    );
    Utils::mapSetValue(
        m_alignmentByButton, ui->alignTopBtn,
        Alignment(Alignment::TOP, Alignment::HCENTER)
    );
    Utils::mapSetValue(
        m_alignmentByButton, ui->alignTopRightBtn,
        Alignment(Alignment::TOP, Alignment::RIGHT)
    );
    Utils::mapSetValue(
        m_alignmentByButton, ui->alignLeftBtn,
        Alignment(Alignment::VCENTER, Alignment::LEFT)
    );
    Utils::mapSetValue(
        m_alignmentByButton, ui->alignCenterBtn,
        Alignment(Alignment::VCENTER, Alignment::HCENTER)
    );
    Utils::mapSetValue(
        m_alignmentByButton, ui->alignRightBtn,
        Alignment(Alignment::VCENTER, Alignment::RIGHT)
    );
    Utils::mapSetValue(
        m_alignmentByButton, ui->alignBottomLeftBtn,
        Alignment(Alignment::BOTTOM, Alignment::LEFT)
    );
    Utils::mapSetValue(
        m_alignmentByButton, ui->alignBottomBtn,
        Alignment(Alignment::BOTTOM, Alignment::HCENTER)
    );
    Utils::mapSetValue(
        m_alignmentByButton, ui->alignBottomRightBtn,
        Alignment(Alignment::BOTTOM, Alignment::RIGHT)
    );

    for (KeyVal const& kv : m_alignmentByButton) {
        connect(kv.first, &QToolButton::toggled, this, &AlignmentWidget::alignmentButtonClicked);
    }

    if (!m_alignment) {
        setEnabled(false);
    }
    setUseAutoMagnetAlignment(false);
    setUseOriginalProportionsAlignment(false);

    // make sure we cached all item texts
    ui->cbAutoMagnet->hidePopup();
    ui->cbOriginalProp->hidePopup();
}

AlignmentWidget::~AlignmentWidget()
{
    delete ui;
}

void AlignmentWidget::setAlignment(Alignment* alignment)
{
    m_alignment = alignment;
    m_advancedAlignment = 0;
    setEnabled(m_alignment != nullptr);
    if (m_alignment) {
        displayAlignment();
    }
}

void AlignmentWidget::setUseAutoMagnetAlignment(bool val)
{
    if (m_useAutoMagnetAlignment != val) {
        m_useAutoMagnetAlignment = val;
        ui->cbAutoMagnet->setCurrentIndex(0);
        ui->cbAutoMagnet->setVisible(m_useAutoMagnetAlignment);
        ui->panelAdvancedAlignment->setVisible(m_useAutoMagnetAlignment || m_useOriginalProportionsAlignment);
    }
}

void AlignmentWidget::setUseOriginalProportionsAlignment(bool val)
{
    if (m_useOriginalProportionsAlignment != val) {
        m_useOriginalProportionsAlignment = val;
        ui->cbOriginalProp->setCurrentIndex(0);
        ui->cbOriginalProp->setVisible(m_useOriginalProportionsAlignment);
        ui->panelAdvancedAlignment->setVisible(m_useAutoMagnetAlignment || m_useOriginalProportionsAlignment);
    }
}

const char* _icon_prp = "basic_icon_name";
const char* _disabled_by_prop = "disabled_by";

void
AlignmentWidget::markAlignmentButtons()
{

    ui->cbAutoMagnet->setItemData(0, 0);
    ui->cbAutoMagnet->setItemData(1, (int) Alignment::VAUTO | (int) Alignment::HAUTO);
    ui->cbAutoMagnet->setItemData(2, (int) Alignment::VAUTO);
    ui->cbAutoMagnet->setItemData(3, (int) Alignment::HAUTO);

    ui->cbOriginalProp->setItemData(0, 0);
    ui->cbOriginalProp->setItemData(1, (int) Alignment::VORIGINAL | (int) Alignment::HORIGINAL);
    ui->cbOriginalProp->setItemData(2, (int) Alignment::VORIGINAL);
    ui->cbOriginalProp->setItemData(3, (int) Alignment::HORIGINAL);

    ui->alignTopLeftBtn->setProperty(_icon_prp, QStringLiteral(":/icons/stock-gravity-north-west-24.png"));
    ui->alignTopBtn->setProperty(_icon_prp, QStringLiteral(":/icons/stock-gravity-north-24.png"));
    ui->alignTopRightBtn->setProperty(_icon_prp, QStringLiteral(":/icons/stock-gravity-north-east-24.png"));
    ui->alignLeftBtn->setProperty(_icon_prp, QStringLiteral(":/icons/stock-gravity-west-24.png"));
    ui->alignCenterBtn->setProperty(_icon_prp, QStringLiteral(":/icons/stock-gravity-center-24")); // should be suffixed
    ui->alignRightBtn->setProperty(_icon_prp, QStringLiteral(":/icons/stock-gravity-east-24.png"));
    ui->alignBottomLeftBtn->setProperty(_icon_prp, QStringLiteral(":/icons/stock-gravity-south-west-24.png"));
    ui->alignBottomBtn->setProperty(_icon_prp, QStringLiteral(":/icons/stock-gravity-south-24.png"));
    ui->alignBottomRightBtn->setProperty(_icon_prp, QStringLiteral(":/icons/stock-gravity-south-east-24.png"));

    const int full_set = Alignment::HAUTO | Alignment::VAUTO | Alignment::VORIGINAL | Alignment::HORIGINAL;
    const int vert_set = Alignment::VAUTO | Alignment::VORIGINAL;
    const int hor_set = Alignment::HAUTO | Alignment::HORIGINAL;

    ui->alignTopLeftBtn->setProperty(_disabled_by_prop, full_set);
    ui->alignTopBtn->setProperty(_disabled_by_prop, vert_set);
    ui->alignTopRightBtn->setProperty(_disabled_by_prop, full_set);
    ui->alignLeftBtn->setProperty(_disabled_by_prop, hor_set);
    ui->alignCenterBtn->setProperty(_disabled_by_prop, 0);
    ui->alignRightBtn->setProperty(_disabled_by_prop, hor_set);
    ui->alignBottomLeftBtn->setProperty(_disabled_by_prop, full_set);
    ui->alignBottomBtn->setProperty(_disabled_by_prop, vert_set);
    ui->alignBottomRightBtn->setProperty(_disabled_by_prop, full_set);
}

void
AlignmentWidget::resetAdvAlignmentComboBoxes(QComboBox* cb, const int composite_alignment)
{
    // items order matters
    cb->blockSignals(true);
    int idx = 0;
    for (int i = 1; i < cb->count(); i++) {
        const int val = cb->itemData(i).toInt();
        if ((val & composite_alignment) == val) {
            idx = i;
            break;
        }
    }
    cb->setCurrentIndex(idx);
    if (cb == ui->cbAutoMagnet) {
        on_cbAutoMagnet_currentIndexChanged(idx);
    } else if (cb == ui->cbOriginalProp) {
        on_cbOriginalProp_currentIndexChanged(idx);
    } else {
        assert(false);
    }
    cb->blockSignals(false);
}

Alignment applyAdvancedAlignment(Alignment alignment, int mask);

void AlignmentWidget::displayAlignment()
{
    if (!m_alignment) {
        return;
    }
    Alignment& alignment(*m_alignment);

    const bool need_alignment = !alignment.isNull();

    ui->alignWithOthersCB->blockSignals(true);
    ui->alignWithOthersCB->setChecked(need_alignment);
    ui->alignWithOthersCB->blockSignals(false);

    ui->panelAlignment->setEnabled(need_alignment);
    ui->panelAdvancedAlignment->setEnabled(need_alignment);

    const int composite_alignment = alignment.compositeAlignment();
    m_advancedAlignment = composite_alignment & Alignment::maskAdvanced;

    for (KeyVal const& kv : m_alignmentByButton) {
        const bool val = need_alignment &&
                         (applyAdvancedAlignment(kv.second, m_advancedAlignment) == alignment);
        kv.first->setChecked(val);
    }

    resetAdvAlignmentComboBoxes(ui->cbAutoMagnet, composite_alignment);
    resetAdvAlignmentComboBoxes(ui->cbOriginalProp, composite_alignment);
}

void
AlignmentWidget::alignmentButtonClicked(bool checked)
{
    if (!m_alignment || !checked) {
        return;
    }

    Alignment& alignment(*m_alignment);

    QToolButton* const button = dynamic_cast<QToolButton*>(sender());
    assert(button);

    AlignmentByButton::iterator const it(m_alignmentByButton.find(button));
    assert(it != m_alignmentByButton.end());

    const bool was_null = alignment.isNull();
    // overwrite button's alignment with selected Auto-Magnet / Original proportions options
    alignment = applyAdvancedAlignment(it->second, m_advancedAlignment);
    alignment.setNull(was_null); // we may emulate click when button disabled and alignment is null

    emit alignmentChanged();
}

void
AlignmentWidget::clickAlignmentButton()
{
    for (KeyVal const& it : m_alignmentByButton) {
        if (it.first->isChecked()) {
            emit it.first->toggled(true);
            break;
        }
    }
}

Alignment
applyAdvancedAlignment(Alignment alignment, int mask)
{
    int val = 0;
    if ((val = mask & Alignment::maskVertical) != 0) {
        alignment.setVertical((Alignment::Vertical) val);
    }

    if ((val = mask & Alignment::maskHorizontal) != 0) {
        alignment.setHorizontal((Alignment::Horizontal) val);
    }

    return alignment;
}

void setComboBoxItemEnabled(QComboBox* cb, const int selected_idx)
{
    // used in assumtion hat item lists are symmetric, rewrite in case of any change
    const int _cb_items_len = 4;
    const int _cb_items_mask[_cb_items_len][_cb_items_len] = { {1, 1, 1, 1}, {1, 0, 0, 0}, {1, 0, 0, 1}, {1, 0, 1, 0} };

    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(cb->model());
    for (int i = 0; i < _cb_items_len; i++) {
        QStandardItem* item = model->item(i);
        item->setFlags(_cb_items_mask[selected_idx][i] ?
                       item->flags() | Qt::ItemIsEnabled :
                       item->flags() & ~Qt::ItemIsEnabled);
    }
}

QIcon prepareIcon(const QString& filename, const QString& maskname)
{
    const QSize icon_size(24, 24);
    QPixmap icon(filename);
    if (icon.size() != icon_size) {
        icon = icon.scaled(icon_size, Qt::KeepAspectRatio);
    }
    if (!maskname.isEmpty()) {
        QPixmap mark(maskname);
        if (!mark.isNull()) {
            if (mark.size() != icon_size) {
                mark = mark.scaled(icon_size, Qt::KeepAspectRatio);
            }
            QPainter painter(&icon);
            painter.drawPixmap(0, 0, mark);
        }
    }
    return QIcon(icon);
}

QString getMaskIconSuffix(const bool has_vert, const bool has_hor)
{
    if (has_vert && !has_hor) {
        return "-ver.png";
    } else if (!has_vert && has_hor) {
        return "-hor.png";
    }
    return ".png";
}

void AlignmentWidget::updateAlignmentButtonsDisplay()
{
    if (!m_alignment) {
        return;
    }
    Alignment& alignment(*m_alignment);

    // update icons and buttons
    const int auto_idx = ui->cbAutoMagnet->currentIndex();
    const int orig_idx = ui->cbOriginalProp->currentIndex();
    const bool central_btn_enabled = !(auto_idx == 1 || orig_idx == 1
                                       || (auto_idx > 1 && orig_idx > 1));

    const int vert_part = m_advancedAlignment & Alignment::maskVertical;
    const int hor_part = m_advancedAlignment & Alignment::maskHorizontal;
    const int auto_set = Alignment::HAUTO | Alignment::VAUTO;
    const int orig_set = Alignment::HORIGINAL | Alignment::VORIGINAL;

    if (vert_part) {
        alignment.setVertical((Alignment::Vertical) vert_part);
    }
    if (hor_part) {
        alignment.setHorizontal((Alignment::Horizontal) hor_part);
    }

    const QString ext = ".png";
    QString suffix_auto = getMaskIconSuffix(vert_part & auto_set, hor_part & auto_set);
    QString suffix_orig = getMaskIconSuffix(vert_part & orig_set, hor_part & orig_set);

    QString mask_filename_auto;
    QString mask_filename_orig;
    if (m_advancedAlignment != 0) {
        mask_filename_auto = ":/icons/auto-magnet" + suffix_auto;
        mask_filename_orig = ":/icons/stock-center-24" + suffix_orig;
    }

    if (m_advancedAlignment != 0 && !ui->alignCenterBtn->isChecked()) { // one of indexes != 0
        ui->alignCenterBtn->setChecked(true);
    } else {
        clickAlignmentButton(); // emulate click for currently checked button
    }

    for (KeyVal const& kv : m_alignmentByButton) {
        QToolButton* btn = kv.first;
        QString icon_filename = btn->property(_icon_prp).toString();

        const int disabled_by = btn->property(_disabled_by_prop).toInt();
        bool disabled_by_auto =  disabled_by & m_advancedAlignment & auto_set;
        bool disabled_by_orig =  disabled_by & m_advancedAlignment & orig_set;
        bool disabled = disabled_by_auto | disabled_by_orig;

        if (btn == ui->alignCenterBtn) {
            disabled = !central_btn_enabled;
            if (disabled) {
                if (auto_idx == 1) {
                    disabled_by_auto = true;
                } else {
                    disabled_by_orig = true;
                }
            }
            icon_filename += ext;
        }

        if (disabled_by_auto) {
            btn->setIcon(prepareIcon(icon_filename, mask_filename_auto));
        } else if (disabled_by_orig) {
            btn->setIcon(prepareIcon(icon_filename, mask_filename_orig));
        } else {
            btn->setIcon(prepareIcon(icon_filename, ""));
        }

        btn->setEnabled(!disabled);

    }

}

void AlignmentWidget::on_alignWithOthersCB_toggled(bool checked)
{
    if (m_alignment) {
        m_alignment->setNull(!checked);
        displayAlignment();
        emit alignmentChanged();
    }
}

void AlignmentWidget::on_cbAutoMagnet_currentIndexChanged(int index)
{
    if (!m_alignment) {
        return;
    }

    // update other comboboxes
    setComboBoxItemEnabled(ui->cbOriginalProp, index);

    switch (index) {
    case 0: m_advancedAlignment &= ~(Alignment::VAUTO | Alignment::HAUTO); break; // save HORIGINAL/VORIGINAL
    case 1: m_advancedAlignment = Alignment::VAUTO | Alignment::HAUTO; break;
    case 2: m_advancedAlignment = Alignment::VAUTO | (m_advancedAlignment & Alignment::maskHorizontal & ~Alignment::HAUTO); break;
    case 3: m_advancedAlignment = (m_advancedAlignment & Alignment::maskVertical & ~Alignment::VAUTO) | Alignment::HAUTO; break;
    default:
        assert(false);
    }

    updateAlignmentButtonsDisplay();
}

void AlignmentWidget::on_cbOriginalProp_currentIndexChanged(int index)
{
    if (!m_alignment) {
        return;
    }

    // update other comboboxes
    setComboBoxItemEnabled(ui->cbAutoMagnet, index);

    switch (index) {
    case 0: m_advancedAlignment &= ~(Alignment::VORIGINAL | Alignment::HORIGINAL); break; // save HAUTO/VAUTO
    case 1: m_advancedAlignment = Alignment::VORIGINAL | Alignment::HORIGINAL; break;
    case 2: m_advancedAlignment = Alignment::VORIGINAL | (m_advancedAlignment & Alignment::maskHorizontal & ~Alignment::HORIGINAL); break;
    case 3: m_advancedAlignment = (m_advancedAlignment & Alignment::maskVertical & ~Alignment::VORIGINAL) | Alignment::HORIGINAL; break;
    default:
        assert(false);
    }

    updateAlignmentButtonsDisplay();
}

void AlignmentWidget::on_btnResetAdvAlignment_clicked()
{
    ui->cbAutoMagnet->setCurrentIndex(0);
    ui->cbOriginalProp->setCurrentIndex(0);
}

AlignmentComboBox::AlignmentComboBox(QWidget* parent): QComboBox(parent)
{
    m_menu.setStyle(style());
}

void AlignmentComboBox::showPopup()
{
    hidePopup();
    m_menu.popup(mapToGlobal(QPoint(0, 0)));
}

void AlignmentComboBox::hidePopup()
{
    updContextMenu();
    QComboBox::hidePopup();
}

void AlignmentComboBox::updContextMenu()
{
    QStandardItemModel* model = qobject_cast<QStandardItemModel*>(this->model());

    for (int i = 0; i < count(); ++i) {
        const QString item_text = itemText(i);
        if (!item_text.isEmpty()) {
            setItemText(i, m_empty);
            if (i < m_itemsText.count()) {
                QAction* action = m_menu.actions()[i];
                action->setText(item_text);
                action->setIcon(itemIcon(i));
            } else {
                QAction* action = m_menu.addAction(itemIcon(i), item_text);
                connect(action, &QAction::triggered, this, [ = ]() {
                    setCurrentIndex(i);
                });
            }
        }

        m_menu.actions()[i]->setEnabled(model->item(i)->flags() & Qt::ItemIsEnabled);
    }
}
