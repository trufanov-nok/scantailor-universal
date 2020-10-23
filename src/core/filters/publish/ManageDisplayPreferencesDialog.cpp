#include "ManageDisplayPreferencesDialog.h"
#include "DisplayPreferences.h"
#include <QColorDialog>
#include <QRegularExpressionValidator>
#include "djview4/qdjvuwidget.h"
#include <QLineEdit>

namespace publish {

ManageDisplayPreferencesDialog::ManageDisplayPreferencesDialog(const DisplayPreferences &prefs, QWidget *parent):
    QDialog(parent)
{
    setupUi(this);

    connect(cbPrecomputeThumb, &QCheckBox::toggled, cbThumbSize,  &QSpinBox::setEnabled);
    connect(cbPrecomputeThumb, &QCheckBox::toggled, lblThumbDesc, &QLabel::setEnabled);
    connect(cbSuggestViewMode, &QCheckBox::toggled, cbViewMode,   &QComboBox::setEnabled);
    connect(cbSuggestViewMode, &QCheckBox::toggled, lblViewModeDesc, &QLabel::setEnabled);
    connect(cbSuggestBackground, &QCheckBox::toggled, btnSelectColor,   &QComboBox::setEnabled);
    connect(cbSuggestBackground, &QCheckBox::toggled, lblBackgroundDesc, &QLabel::setEnabled);
    connect(cbSuggestZoom, &QCheckBox::toggled, cbZoom,   &QComboBox::setEnabled);
    connect(cbSuggestZoom, &QCheckBox::toggled, lblZoomDesc, &QLabel::setEnabled);

    cbViewMode->addItem(tr("Color"), "color");
    cbViewMode->addItem(tr("Black & White"), "bw");
    cbViewMode->addItem(tr("Foreground"), "fore");
    cbViewMode->addItem(tr("Background"), "back");

    cbHorizontalAlignment->addItem(tr("Center"), "center");
    cbHorizontalAlignment->addItem(tr("Left"), "left");
    cbHorizontalAlignment->addItem(tr("Right"), "right");

    cbVerticalAlignment->addItem(tr("Center"), "center");
    cbVerticalAlignment->addItem(tr("Top"), "top");
    cbVerticalAlignment->addItem(tr("Bottom"), "bottom");

    cbZoom->addItem(tr("Fit Width","cbZoom"), "width");
    cbZoom->addItem(tr("Fit Page","cbZoom"), "page");
    cbZoom->addItem(tr("Stretch","cbZoom"), "stretch");
    cbZoom->addItem(tr("1:1","cbZoom"), "one2one");
    cbZoom->addItem(tr("300%","cbZoom"), "");
    cbZoom->addItem(tr("200%","cbZoom"), "");
    cbZoom->addItem(tr("150%","cbZoom"), "");
    cbZoom->addItem(tr("100%","cbZoom"), "");
    cbZoom->addItem(tr("75%","cbZoom"), "");
    cbZoom->addItem(tr("50%","cbZoom"), "");

    QRegularExpression rx("[1-9]\\d{0,2}%?");
    QValidator *validator = new QRegularExpressionValidator(rx, this);
    cbZoom->setValidator(validator);

    cbPrecomputeThumb->setChecked(prefs.thumbSize() > 0);
    cbSuggestViewMode->setChecked(!prefs.mode().isEmpty());
    m_background_clr = prefs.background();
    cbSuggestBackground->setChecked(!m_background_clr.isEmpty());
    gbSuggestAlignment->setChecked(!prefs.alignX().isEmpty());

    cbThumbSize->setValue(prefs.thumbSize());
    int idx = cbViewMode->findData(prefs.mode());
    if (idx != -1) {
        cbViewMode->setCurrentIndex(idx);
    }

    idx = cbHorizontalAlignment->findData(prefs.alignX());
    if (idx != -1) {
        cbHorizontalAlignment->setCurrentIndex(idx);
    }

    idx = cbVerticalAlignment->findData(prefs.alignY());
    if (idx != -1) {
        cbVerticalAlignment->setCurrentIndex(idx);
    }


    if (!m_background_clr.isEmpty()) {
        QColor clr(m_background_clr);
        if (!clr.isValid()) {
            clr.setRgb(0xFF, 0xFF, 0xFF);
        }
        const QString style("QToolButton {background: %1};");
        btnSelectColor->setStyleSheet(style.arg(clr.name(QColor::HexRgb)));
    }

    const QString& zoom = prefs.zoom();
    cbSuggestZoom->setChecked(!zoom.isEmpty());
    if (!zoom.isEmpty()) {
        if (zoom.startsWith("d")) {
            bool ok;
            int val = zoom.midRef(1).toInt(&ok);
            if (ok && val > 0 && val < 1000) {
                const QString item_text = QString::number(val)+"%";
                const int idx = cbZoom->findText(item_text);
                if (idx != -1) {
                    cbZoom->setCurrentIndex(idx);
                } else {
                    cbZoom->addItem(item_text);
                    cbZoom->setCurrentIndex(cbZoom->count()-1);
                }
            }
        } else {
            const int idx = cbZoom->findData(zoom);
            if (idx != -1) {
                cbZoom->setCurrentIndex(idx);
            }
        }
    }


}

DisplayPreferences
ManageDisplayPreferencesDialog::preferences()
{
    DisplayPreferences res;
    if (cbPrecomputeThumb->isChecked()) {
        res.setThumbSize(cbThumbSize->value());
    }
    if (cbSuggestViewMode->isChecked()) {
        res.setMode(cbViewMode->currentData().toString());
    }
    if (gbSuggestAlignment->isChecked()) {
        res.setAlignX(cbHorizontalAlignment->currentData().toString());
        res.setAlignY(cbVerticalAlignment->currentData().toString());
    }
    if (cbSuggestBackground->isChecked()) {
        res.setBackground(m_background_clr);
    }

    if (cbSuggestZoom->isChecked()) {
        QString val = cbZoom->currentData().toString();
        if (val.isEmpty()) {
            bool ok;
            const int zoom = cbZoom->currentText().
                    replace(QRegularExpression("\\s*%?$"),"").trimmed()
                    .toInt(&ok);
            if (ok) {
                if (zoom > 0 && zoom <1000) {
                    val = "d" + QString::number(zoom);
                }
            }
        }
        if (!val.isEmpty()) {
            res.setZoom(val);
        }
    }

    return res;
}

void ManageDisplayPreferencesDialog::on_btnSelectColor_clicked()
{
    QColor clr = QColorDialog::getColor(clr, this, tr("Color selection"));
    if (clr.isValid()) {
        m_background_clr = clr.name(QColor::HexRgb);
        QString s("QToolButton {background: %1};");
        btnSelectColor->setStyleSheet(s.arg(m_background_clr));
    }

}

}
