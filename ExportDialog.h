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
//begin of modified by monday2000
//Export_Subscans
// This file was added by monday2000

#ifndef EXPORT_DIALOG_H_
#define EXPORT_DIALOG_H_

#include "ui_ExportDialog.h"
#include "exporting/ExportSettings.h"
#include <QDialog>
#include <QSettings>

namespace exporting {

class ExportDialog : public QDialog
{
    Q_OBJECT
public:

    ExportDialog(QWidget* parent, const QString& defaultOutDir);

    virtual ~ExportDialog();

    void setCount(int count);

    void reset(void);
    void setExportLabel(void);
    void setStartExport(void);

Q_SIGNALS:
    void ExportOutputSignal(ExportSettings settings);
    void ExportStopSignal();
    void SetStartExportSignal();

public Q_SLOTS:
    void startExport(void);
    void stepProgress();
    void exportCanceled();
    void exportCompleted();
    void exportError(QString const&);

private Q_SLOTS:
    void OnCheckDefaultOutputFolder(bool);
    void OnClickExport();
    void outExportDirBrowse();
    void outExportDirEdited(QString const&);
    //void tabChanged(int tab);
    void OnCheckGenerateBlankBackSubscans(bool);
    void OnCheckUseSepSuffixForPics(bool);
    void OnCheckKeepOriginalColorIllumForeSubscans(bool);

    void on_cbExportZones_stateChanged(int arg1);

    void on_cbExportForeground_stateChanged(int arg1);

    void on_cbExportBackground_stateChanged(int arg1);

    void on_cbExportMask_stateChanged(int arg1);

    void on_cbMultipageOutput_toggled(bool checked);

    void on_cbExportImage_stateChanged(int arg1);

    void on_cbExportAutomask_stateChanged(int arg1);

    void on_cbExportWithoutOutputStage_stateChanged(int arg1);

    void on_btnResetToDefault_clicked();

private:
    void saveExportMode(ExportMode val, bool on);
    void displayExportMode(ExportModes mode);

private:
    void setExportOutputDir(QString const& dir);
    Ui::ExportDialog ui;

    bool m_autoOutDir;
    int m_count;
    QString const m_defaultOutDir;
    QString m_prevOutDir;
    QSettings m_settings;
};

}
#endif
//end of modified by monday2000
