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
#include "QMLLoader.h"
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
                  PageSelectionAccessor const& page_selection_accessor,
                  DjVuPageGenerator& m_pageGenerator);

    virtual ~OptionsWidget();

    void preUpdateUI(const PageId &page_id);

    void postUpdateUI();

    QMLLoader* getQMLLoader() { return &m_QMLLoader; }

    Q_PROPERTY(int width READ width NOTIFY widthChanged)

private:
    void setupQuickWidget(QQuickWidget* &wgt, QQmlComponent* component, QObject* instance);
    void initEncodersSelector();
    bool initTiffConverter();
    bool showMissingAppSelectors(QStringList req_apps, bool isEncoderDep);
    void displayWidgets();


private slots:
    void on_statusChanged(const QQuickWidget::Status &arg1);
    void on_cbEncodersSelector_currentIndexChanged(int index);

    void on_dpiValue_linkActivated(const QString &link);

public slots:
    void resizeQuickWidget(QVariant arg);
    void requestParamUpdate(const QVariant requestor = QVariant());
signals:
    void widthChanged();

private:

    IntrusivePtr<Settings> m_ptrSettings;
    PageId m_pageId;
    PageSelectionAccessor m_pageSelectionAccessor;
    QMLLoader m_QMLLoader;
    DjVuPageGenerator& m_pageGenerator;
    bool m_QMLinitialized;
};

} // namespace publishing

#endif
