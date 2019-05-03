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
#include <QDialog>
#include <QSettings>

class ExportDialog : public QDialog
{
	Q_OBJECT
public:

    enum ExportMode {
        None = 0,
        Foreground = 1,
        Background = 2,
        Mask = 4,
        Zones = 8,
        WholeImage = 16,
        AutoMask = 32,
        ImageWithoutOutputStage = 64
    };

    Q_DECLARE_FLAGS(ExportModes, ExportMode)

    enum PageGenTweak {
      NoTweaks = 0,
      KeepOriginalColorIllumForeSubscans = 1,
      IgnoreOutputProcessingStage = 2
    };

    Q_DECLARE_FLAGS(PageGenTweaks, PageGenTweak)

    struct Settings {
        ExportModes export_mode;
        bool default_out_dir;
        QString export_dir_path;
        bool export_to_multipage;
        bool generate_blank_back_subscans;
        bool use_sep_suffix_for_pics;
        PageGenTweaks page_gen_tweaks;
        bool export_selected_pages_only;
    };

    ExportDialog(QWidget* parent, const QString& defaultOutDir);

	virtual ~ExportDialog();

	void setCount(int count);
	void StepProgress();
	void reset(void);
	void setExportLabel(void);
	void setStartExport(void);	

signals:
    void ExportOutputSignal(Settings settings);
	void ExportStopSignal();
	void SetStartExportSignal();	

public slots:	
	void startExport(void);

private slots:	
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

private:
    void saveExportMode(ExportMode val, bool on);

private:
    void setExportOutputDir(QString const& dir);
	Ui::ExportDialog ui;

	bool m_autoOutDir;
    int m_count;
    QString const m_defaultOutDir;
    QString m_prevOutDir;
    QSettings m_settings;
};

#endif
//end of modified by monday2000
