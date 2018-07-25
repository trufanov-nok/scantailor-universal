/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2018 Alexander Trufanov <trufanovan@gmail.com>

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

#ifndef PUBLISHING_OPTIONSWIDGET_H_
#define PUBLISHING_OPTIONSWIDGET_H_

#include "ui_PublishingOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "PageId.h"
#include "IntrusivePtr.h"
#include "PageSelectionAccessor.h"
#include "djvuencoder.h"
#include "DjVuPageGenerator.h"
#include <memory>

namespace publishing
{

class Settings;

class OptionsWidget :
    public FilterOptionsWidget, private Ui::PublishingOptionsWidget
{
	Q_OBJECT
public:
	OptionsWidget(IntrusivePtr<Settings> const& settings,
		PageSelectionAccessor const& page_selection_accessor);
	
	virtual ~OptionsWidget();
	
    void preUpdateUI(QString const& filename, const PageId &page_id);
	
    void postUpdateUI();

private:
    void setupQuickWidget(QQuickWidget* wgt);
    void setLinkToMainApp(QQuickWidget* wgt);
    void initEncodersSelector();
    bool initTiffConvertor();
    void checkDependencies(const AppDependency& dep);
    void checkDependencies(const AppDependencies& deps);
    bool allDependenciesFound(const QStringList& req_apps, QStringList *missing_apps = nullptr);
    void showMissingAppSelectors(QStringList req_apps, bool isEncoderDep);
    void setOutputDpi(uint dpi);
    void displayWidgets();


private slots:
    void on_statusChanged(const QQuickWidget::Status &arg1);
    void on_cbEncodersSelector_currentIndexChanged(int index);

    void on_dpiValue_linkActivated(const QString &link);

public slots:
    void resizeQuickWidget(QVariant arg);
    void requestParamUpdate(const QVariant requestor);

private:
	
	IntrusivePtr<Settings> m_ptrSettings;
    PageId m_pageId;
	PageSelectionAccessor m_pageSelectionAccessor;
    QString m_filename;
    AppDependencies m_dependencies;
    QVector<DjVuEncoder*> m_encoders;    
    QStringList m_convertorRequiredApps;
    QString m_encoderCmd;
    QString m_convertorCmd;
    std::unique_ptr<DjVuPageGenerator> m_pageGenerator;
};

} // namespace publishing

#endif
