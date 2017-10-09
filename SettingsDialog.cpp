/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "SettingsDialog.h"
#include "SettingsDialog.moc"
#include "OpenGLSupport.h"
#include "config.h"
#include <QVariant>
#include <QDir>
#include <MainWindow.h>
#include <QDebug>
#include <QResource>
#include <boost/foreach.hpp>
#include "filters/output/DespeckleLevel.h"
#include "filters/output/Params.h"

SettingsDialog::SettingsDialog(QWidget* parent)
    :	QDialog(parent), m_accepted(false)
{
    backupSettings();

    ui.setupUi(this);
    ui.lineEdit->setFiltering(true);
    connect(ui.lineEdit, SIGNAL(filterChanged(QString)), this, SLOT(filterChanged(QString)));

    ui.treeWidget->addAction(ui.actionExpand_all);
    ui.treeWidget->addAction(ui.actionCollapse_all);
    populateTreeWidget(ui.treeWidget);
    restoreSettingsTreeState(ui.treeWidget);


#ifndef ENABLE_OPENGL
    //    ui.use3DAcceleration->setVisible(false);
    //    ui.use3DAcceleration->setChecked(false);
    //    ui.use3DAcceleration->setEnabled(false);
    //    ui.use3DAcceleration->setToolTip(tr("Compiled without OpenGL support."));
#else
    //    if (!OpenGLSupport::supported()) {
    //        ui.use3DAcceleration->setChecked(false);
    //        ui.use3DAcceleration->setEnabled(false);
    //        ui.use3DAcceleration->setToolTip(tr("Your hardware / driver don't provide the necessary features."));
    //    } else {
    //        ui.use3DAcceleration->setChecked(
    //                    settings.value("settings/use_3d_acceleration", false).toBool()
    //                    );
    //    }
#endif

    connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    // pageGeneral is displayed by default
    initLanguageList(((MainWindow*)parent)->getLanguage());

    ui.startBatchProcessingDlgAllPages->setChecked(
                !m_settings.value("batch_dialog/start_from_current_page", true).toBool());
    ui.showStartBatchProcessingDlg->setChecked(
                !m_settings.value("batch_dialog/remember_choice", false).toBool());
}

void SettingsDialog::initLanguageList(QString cur_lang)
{
    ui.language->clear();
    ui.language->addItem("English", "en");;

    const QStringList language_file_filter("scantailor_*.qm");
    QStringList fileNames = QDir().entryList(language_file_filter);

    if (fileNames.isEmpty()) {
        fileNames = QDir(QApplication::applicationDirPath()).entryList(language_file_filter);
        if (fileNames.isEmpty()) {
            fileNames = QDir(QString::fromUtf8(TRANSLATIONS_DIR_ABS)).entryList(language_file_filter);
            if (fileNames.isEmpty()) {
                fileNames = QDir(QString::fromUtf8(TRANSLATIONS_DIR_REL)).entryList(language_file_filter);
            }
        }
    }

    fileNames.sort();

    for (int i = 0; i < fileNames.size(); ++i) {
        // get locale extracted by filename
        QString locale;
        locale = fileNames[i];
        locale.truncate(locale.lastIndexOf('.'));
        locale.remove(0, locale.indexOf('_') + 1);

        QString lang = QLocale::languageToString(QLocale(locale).language());
        ui.language->addItem(lang, locale);
        if (locale == cur_lang)
            ui.language->setCurrentIndex(i+1);
    }

    ui.language->setEnabled(ui.language->count() > 0);
}

void SettingsDialog::backupSettings()
{
    m_oldSettings.clear();
    QStringList sl = m_settings.allKeys();

    BOOST_FOREACH(QString key, sl) {
        m_oldSettings[key] = m_settings.value(key);
    }
}

void SettingsDialog::restoreSettings()
{
    m_settings.clear();

    for (QSettings::SettingsMap::const_iterator it = m_oldSettings.constBegin(); it != m_oldSettings.constEnd(); ++it) {
        m_settings.setValue(it.key(), *it);
    }
}

SettingsDialog::~SettingsDialog()
{
    if (!m_accepted) {
        restoreSettings();
        storeSettingsTreeState(ui.treeWidget);
    } else {
        storeSettingsTreeState(ui.treeWidget);
        emit settingsChanged();
    }
}

void
SettingsDialog::commitChanges()
{
    m_accepted = true;
    accept();
}


void
SettingsDialog::on_treeWidget_itemActivated(QTreeWidgetItem *item, int column)
{

}

void
SettingsDialog::on_treeWidget_itemSelectionChanged()
{
    QList<QTreeWidgetItem *> sItems= ui.treeWidget->selectedItems();
    if (sItems.isEmpty()) {
        ui.stackedWidget->setCurrentWidget(ui.pageEmpty);
    } else {
        ui.stackedWidget->setCurrentWidget(sItems[0]->data(0, Qt::UserRole).value<QWidget*>());
    }
}

void
SettingsDialog::setupItem(QTreeWidgetItem *item, QWidget* w, QString s, bool default_val)
{
    item->setData(0, Qt::UserRole, QVariant::fromValue(w));
    item->setData(0, Qt::UserRole+1, s);
    if (!s.isEmpty()) {
        const bool b = m_settings.value(s, default_val).toBool();
        item->setCheckState(1, b?Qt::Checked:Qt::Unchecked);
        w->setEnabled(b);
    }
}

void
disable_subtree(QTreeWidgetItem *item)
{
    item->setDisabled(true);
    for (int i = 0; i < item->childCount(); i++) {
        disable_subtree(item->child(i));
    }
}

void check_nested_disabled(QTreeWidgetItem *item)
{
    QString key = item->data(0, Qt::UserRole+1).toString();
    bool disable_children = (!key.isEmpty() && item->checkState(1) != Qt::Checked);

     for (int i = 0; i < item->childCount(); i++) {
         if (disable_children) {
             disable_subtree(item->child(i));
         } else {
             check_nested_disabled(item->child(i));
         }
     }
}

void
SettingsDialog::populateTreeWidget(QTreeWidget* treeWidget)
{
    const QStringList settingsTreeTitles = (QStringList()
                                            << tr("General")                                            
                                            <<        tr("Docking")
                                            <<        tr("Auto-save project")
                                            <<        tr("Tiff compression")
                                            <<        tr("Debug mode"))
                                            << tr("Fix Orientation")
                                            << tr("Split pages")
                                            <<        tr("Apply cut")
                                            << tr("Deskew")
                                            <<        tr("Mark deviant pages")
                                            << tr("Select Content")
                                            <<        tr("Fine Tune page corners")
                                            <<        tr("Borders Panel")
                                            <<        tr("Mark deviant pages")
                                            << tr("Margins")
                                            <<        tr("Original alignment")
                                            <<                tr("Auto margins")
                                            <<        tr("Mark deviant pages")
                                            << tr("Output")
                                            <<        tr("Black & White mode")
                                            <<        tr("Color/Grayscale mode")
                                            <<        tr("Mixed mode")
                                            <<               tr("Picture shape detection")
                                            <<                      tr("Quadro")
                                            <<        tr("Fill zones")
                                            <<        tr("Dewarping")
                                            <<        tr("Despeckling");
    const QResource tree_metadata(":/SettingsTreeData.tsv");
    QStringList tree_data = QString::fromUtf8((char const*)tree_metadata.data(), tree_metadata.size()).split('\n');

    QTreeWidgetItem* last_top_item = NULL;
    QTreeWidgetItem* parent_item = NULL;
    int idx = 0;

    treeWidget->blockSignals(true);

    BOOST_FOREACH(QString name, settingsTreeTitles) {

        QStringList metadata = tree_data[idx++].split('\t', QString::KeepEmptyParts);
        Q_ASSERT(!metadata.isEmpty());

        int level = 0;
        while(metadata[level++].isEmpty());

        parent_item = last_top_item; // level = 1
        for (int i = 0; i< level-2; i++) {
            Q_ASSERT(parent_item != NULL);
            parent_item = parent_item->child(parent_item->childCount()-1);
        }

        if (level == 1) {
            last_top_item = new QTreeWidgetItem(treeWidget, QStringList(name));
            parent_item = last_top_item;
        } else {
            parent_item = new QTreeWidgetItem(parent_item, QStringList(name));
        }

        QString id = metadata[level-1];
        Q_ASSERT(!id.isEmpty());

        QWidget* page = ui.stackedWidget->findChild<QWidget*>(id);
        if (page == 0) {
            page = ui.pageEmpty;
        }

        QString reg_key;
        if (metadata.size() > level) {
            reg_key = metadata[level];
        }

        bool state = true;
        if (metadata.size() > level+1) {
            state = metadata[level+1].toInt() > 0;
        }

        setupItem(parent_item, page, reg_key, state);

    }


    treeWidget->blockSignals(false);

    treeWidget->setCurrentItem(treeWidget->topLevelItem(0));

    for(int i = 0; i < treeWidget->topLevelItemCount(); i++) {
        check_nested_disabled(treeWidget->topLevelItem(i));
    }
}

bool filterItem(QTreeWidgetItem * i, const QString & filter)
{
    bool visible = false;
    for (int idx = 0; idx < i->childCount(); idx++)
    {
        if (filterItem(i->child(idx), filter)) {
            visible = true;
        }
    }

    if (!visible) {
        visible = i->text(0).contains(filter, Qt::CaseInsensitive);
    }

    i->setHidden(!visible);
    return visible;
}

void SettingsDialog::filterChanged(const QString & filter)
{
    ui.treeWidget->blockSignals(true);
    for(int i = 0; i < ui.treeWidget->topLevelItemCount(); i++) {
        filterItem(ui.treeWidget->topLevelItem(i), filter);
    }
    ui.treeWidget->expandAll();
    ui.treeWidget->blockSignals(false);
}

void conditionalExpand(QTreeWidgetItem* item, int& idx, const QStringList& states)
{
    item->setExpanded( (idx < states.count())? !states[idx++].isEmpty():false );
    for(int i = 0; i < item->childCount(); i++) {
        conditionalExpand(item->child(i), idx, states);
    }
}

void SettingsDialog::restoreSettingsTreeState(QTreeWidget* treeWidget)
{
    QStringList tree_expand_state = m_settings.value("main_window/settings_tree_state","").toString().split(',',QString::KeepEmptyParts);
    int idx = 0;
    for (int i = 0; i < treeWidget->topLevelItemCount(); i++) {
        conditionalExpand(treeWidget->topLevelItem(i), idx, tree_expand_state);
    }
}

void saveExpandState(QTreeWidgetItem* item, QStringList& states)
{
    states << (item->isExpanded()?"1":"");
    for(int i = 0; i < item->childCount(); i++) {
        saveExpandState(item->child(i), states);
    }
}

void SettingsDialog::storeSettingsTreeState(QTreeWidget* treeWidget)
{
    QStringList tree_expand_state;
    for (int i = 0; i < treeWidget->topLevelItemCount(); i++) {
        saveExpandState(treeWidget->topLevelItem(i), tree_expand_state);
    }
    m_settings.setValue("main_window/settings_tree_state",tree_expand_state.join(','));
}



void SettingsDialog::on_actionExpand_all_triggered()
{
    ui.treeWidget->expandAll();
}

void SettingsDialog::on_actionCollapse_all_triggered()
{
    ui.treeWidget->collapseAll();
}

void SettingsDialog::on_treeWidget_itemChanged(QTreeWidgetItem *item, int column)
{
    if (column == 1) {
        QString key = item->data(0, Qt::UserRole+1).toString();
        bool b = item->checkState(1) == Qt::Checked;
        m_settings.setValue(key, b);
        item->data(0, Qt::UserRole).value<QWidget*>()->setEnabled(b);
        for (int i = 0; i < item->childCount(); i++) {
            QTreeWidgetItem *c = item->child(i);

            if (c->data(1, Qt::CheckStateRole).isValid()) {
                if (!b && c->checkState(1) != Qt::Unchecked) {
                    c->setCheckState(1, Qt::Unchecked);
                    ui.treeWidget->expandItem(c);
                }
            }

            c->setDisabled(!b);
        }
        if (item->childCount() > 0) {
            ui.treeWidget->expandItem(item);
        }

        if (key == "picture_shape_detection/enabled" ||
                key == "picture_shape_detection/smaller_rect") {
            setupPictureShapeComboBox();
        }
    }

}

void SettingsDialog::on_language_currentIndexChanged(int index)
{
    m_settings.setValue("main_window/language", ui.language->itemData(index).toString());
}

void SettingsDialog::on_cbApplyCutDefault_clicked(bool checked)
{
    m_settings.setValue("apply_cut/default", checked);
}

void SettingsDialog::loadTiffList()
{
    bool filtered_only = !m_settings.value("tiff_compression/show_all", false).toBool();
    ui.cbTiffFilter->blockSignals(true);
    ui.cbTiffFilter->setChecked(filtered_only);
    ui.cbTiffFilter->blockSignals(false);

    const QResource tiff_data(":/TiffCompressionMethods.tsv");
    QStringList tiff_list = QString::fromUtf8((char const*)tiff_data.data(), tiff_data.size()).split('\n');
    if (filtered_only) {
        tiff_list = tiff_list.filter(QRegularExpression(".*\t0$"));
    }


    QString old_val = m_settings.value("tiff_compression/method", "LZW").toString();

    ui.cbTiffCompression->clear();

    ui.cbTiffCompression->blockSignals(true);
    BOOST_FOREACH(QString const& s, tiff_list) {
        if (!s.trimmed().isEmpty()) {
            const QStringList& sl = s.split('\t');
            ui.cbTiffCompression->addItem(sl[0], sl[2]);
        }
    }
    ui.cbTiffCompression->blockSignals(false);


    int idx = ui.cbTiffCompression->findText(old_val);
    if (idx != -1 || ui.cbTiffCompression->count() > 0) {
        if (idx < 0) {
            idx = 0;
        }
        ui.cbTiffCompression->setCurrentIndex(idx);
//        on_cbTiffCompression_currentIndexChanged(idx); // not triggered in one case
    }
}

void SettingsDialog::on_stackedWidget_currentChanged(int /*arg1*/)
{
    const QWidget* currentPage = ui.stackedWidget->currentWidget();
    if (currentPage == ui.pageApplyCut) {
        ui.cbApplyCutDefault->setChecked(m_settings.value("apply_cut/default", false).toBool());
    } else if (currentPage == ui.pageTiffCompression) {
        loadTiffList();
        ui.useHorizontalPredictor->setChecked(m_settings.value("tiff_compression/use_horizontal_predictor", false).toBool());
    } else if (currentPage == ui.pageAutoSaveProject) {
        ui.sbSavePeriod->setValue(abs(m_settings.value("auto-save_project/time_period_min", 5).toInt()));
    } else if (currentPage == ui.pageOutput) {
        ui.dpiDefaultXValue->setValue(m_settings.value("output/default_dpi_x", 600).toUInt());
        ui.dpiDefaultYValue->setValue(m_settings.value("output/default_dpi_y", 600).toUInt());

        ui.ThresholdMinValue->setValue( m_settings.value("output/binrization_threshold_control_min", -50).toInt() );
        ui.ThresholdMaxValue->setValue( m_settings.value("output/binrization_threshold_control_max", 50).toInt() );
        ui.ThresholdDefaultsValue->setValue( m_settings.value("output/binrization_threshold_control_default", 0).toInt());
        ui.ThresholdDefaultsValue->setMinimum(ui.ThresholdMinValue->value());
        ui.ThresholdDefaultsValue->setMaximum(ui.ThresholdMaxValue->value());
        connect( ui.ThresholdMinValue, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onThresholdValueChanged);
        connect( ui.ThresholdMaxValue, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SettingsDialog::onThresholdValueChanged);
    } else if (currentPage == ui.pageDespeckling) {
        QComboBox* cb = ui.despecklingDefaultsValue;
        cb->blockSignals(true);
        cb->clear();
        cb->addItem(tr("None"),  output::DESPECKLE_OFF);
        cb->addItem(tr("Cuatious"),  output::DESPECKLE_CAUTIOUS);
        cb->addItem(tr("Normal"),  output::DESPECKLE_NORMAL);
        cb->addItem(tr("Aggresive"),  output::DESPECKLE_AGGRESSIVE);
        int def = m_settings.value("despeckling/default_level", output::DESPECKLE_CAUTIOUS).toInt();

        int idx = cb->findData(def);
        cb->setCurrentIndex(idx!=-1?idx:0);

        cb->blockSignals(false);
    } else if (currentPage == ui.pagePictureShapeDetection) {
        setupPictureShapeComboBox();
    } else if (currentPage == ui.pageGeneral) {
        ui.startBatchProcessingDlgAllPages->setChecked(
                    !m_settings.value("batch_dialog/start_from_current_page", true).toBool());
        ui.showStartBatchProcessingDlg->setChecked(
                    !m_settings.value("batch_dialog/remember_choice", false).toBool());
    }

}

void SettingsDialog::setupPictureShapeComboBox()
{
    QComboBox* cb = ui.picturesShapeDefaultsValue;
    cb->blockSignals(true);
    cb->clear();
    cb->addItem(tr("Free"),  output::FREE_SHAPE);
    if (m_settings.value("picture_shape_detection/enabled", true).toBool()) {
        cb->addItem(tr("Rectangular"),  output::RECTANGULAR_SHAPE);
    }
    if (m_settings.value("picture_shape_detection/smaller_rect", true).toBool()) {
        cb->addItem(tr("Quadro"),  output::QUADRO_SHAPE);
    }

    int def = m_settings.value("picture_shape_detection/default", output::FREE_SHAPE).toInt();


    int idx = cb->findData(def);
    idx = idx!=-1?idx:0;
    cb->setCurrentIndex(idx);

    cb->blockSignals(false);
    on_picturesShapeDefaultsValue_currentIndexChanged(idx);
}


void SettingsDialog::on_cbTiffCompression_currentIndexChanged(int index)
{
    ui.lblTiffDetails->setText(ui.cbTiffCompression->itemData(index).toString());
    m_settings.setValue("tiff_compression/method", ui.cbTiffCompression->currentText());
}

void SettingsDialog::on_cbTiffFilter_clicked(bool checked)
{
    m_settings.setValue("tiff_compression/show_all", !checked);
    loadTiffList();
}

void SettingsDialog::on_sbSavePeriod_valueChanged(int arg1)
{
    m_settings.setValue("auto-save_project/time_period_min", arg1);
}

void SettingsDialog::onThresholdValueChanged(int)
{
    int min = ui.ThresholdMinValue->value();
    int max = ui.ThresholdMaxValue->value();

    if (min > max) { //swap values
        max = min;
        min = ui.ThresholdMaxValue->value();
    }

    if (min == max) {
        if (max < ui.ThresholdMaxValue->maximum()) {
            max++;
        } else
        if (min > ui.ThresholdMaxValue->minimum()) {
            min--;
        }
    }

    min = std::max(ui.ThresholdMinValue->minimum(), min);
    max = std::min(ui.ThresholdMaxValue->maximum(), max);

    if (min != ui.ThresholdMinValue->value()) {
        ui.ThresholdMinValue->setValue(min);
    }

    if (max != ui.ThresholdMaxValue->value()) {
        ui.ThresholdMaxValue->setValue(max);
    }

    m_settings.setValue("output/binrization_threshold_control_min", min);
    m_settings.setValue("output/binrization_threshold_control_max", max);

    ui.ThresholdDefaultsValue->setMinimum(min);
    ui.ThresholdDefaultsValue->setMaximum(max);
}

void SettingsDialog::on_despecklingDefaultsValue_currentIndexChanged(int index)
{
    int val = ui.despecklingDefaultsValue->itemData(index).toInt();
    m_settings.setValue("despeckling/default_level", val);
}

void SettingsDialog::on_picturesShapeDefaultsValue_currentIndexChanged(int index)
{
    int val = ui.picturesShapeDefaultsValue->itemData(index).toInt();
    m_settings.setValue("picture_shape_detection/default", val);
}

void SettingsDialog::on_startBatchProcessingDlgAllPages_toggled(bool checked)
{
    m_settings.setValue("batch_dialog/start_from_current_page", !checked);
}

void SettingsDialog::on_showStartBatchProcessingDlg_clicked(bool checked)
{
    m_settings.setValue("batch_dialog/remember_choice", !checked);
}

void SettingsDialog::on_ThresholdDefaultsValue_valueChanged(int arg1)
{
    int val = arg1;
    val = std::max(val, ui.ThresholdMinValue->value());
    val = std::min(val, ui.ThresholdMaxValue->value());
    if (val != arg1) {
        ui.ThresholdDefaultsValue->blockSignals(true);
        ui.ThresholdDefaultsValue->setValue(val);
        ui.ThresholdDefaultsValue->blockSignals(false);
    }

    m_settings.setValue("output/binrization_threshold_control_default", val);
}

void SettingsDialog::on_dpiDefaultYValue_valueChanged(int arg1)
{
    m_settings.setValue("output/default_dpi_y", arg1);
}

void SettingsDialog::on_dpiDefaultXValue_valueChanged(int arg1)
{
    m_settings.setValue("output/default_dpi_x", arg1);
}

void SettingsDialog::on_useHorizontalPredictor_clicked(bool checked)
{
    m_settings.setValue("tiff_compression/use_horizontal_predictor", checked);
}
