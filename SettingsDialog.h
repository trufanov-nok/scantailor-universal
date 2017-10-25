/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef SETTINGS_DIALOG_H_
#define SETTINGS_DIALOG_H_

#include "ui_SettingsDialog.h"
#include "QSettings"
#include <QDialog>

class SettingsDialog : public QDialog
{
	Q_OBJECT
public:
	SettingsDialog(QWidget* parent = 0);
	
	virtual ~SettingsDialog();
signals:
    void settingsChanged();
private slots:
	void commitChanges();
    void filterChanged(const QString &);
    void on_treeWidget_itemActivated(QTreeWidgetItem *item, int column);
    void on_treeWidget_itemSelectionChanged();    
    void on_actionExpand_all_triggered();
    void on_actionCollapse_all_triggered();
    void on_treeWidget_itemChanged(QTreeWidgetItem *item, int column);
    void on_language_currentIndexChanged(int index);
    void on_cbApplyCutDefault_clicked(bool checked);
    void on_stackedWidget_currentChanged(int arg1);

    void on_cbTiffCompression_currentIndexChanged(int index);

    void on_cbTiffFilter_clicked(bool checked);

    void on_sbSavePeriod_valueChanged(int arg1);

    void onThresholdValueChanged(int);

    void on_despecklingDefaultsValue_currentIndexChanged(int index);

    void on_startBatchProcessingDlgAllPages_toggled(bool checked);

    void on_showStartBatchProcessingDlg_clicked(bool checked);

    void on_ThresholdDefaultsValue_valueChanged(int arg1);

    void on_dpiDefaultYValue_valueChanged(int arg1);

    void on_dpiDefaultXValue_valueChanged(int arg1);

    void on_useHorizontalPredictor_clicked(bool checked);

    void on_disableSmoothingBW_clicked(bool checked);

    void on_rectangularAreasSensitivityValue_valueChanged(int arg1);

    void on_originalPageDisplayOnKeyHold_clicked(bool checked);

private:
    void initLanguageList(QString cur_lang);
    void loadTiffList();
    void populateTreeWidget(QTreeWidget* treeWidget);
    void setupItem(QTreeWidgetItem *item, QWidget* w = nullptr, QString s = "", bool default_val = true);
    void backupSettings();
    void restoreSettings();
    void restoreSettingsTreeState(QTreeWidget* treeWidget);
    void storeSettingsTreeState(QTreeWidget* treeWidget);
    void setupPictureShapeComboBox();
private:
	Ui::SettingsDialog ui;
    QSettings m_settings;
    QSettings::SettingsMap m_oldSettings;
    bool m_accepted;
};

#endif
