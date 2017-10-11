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

#ifndef STATUSBARPROVIDER_H_
#define STATUSBARPROVIDER_H_

#include <QStatusBar>
#include <Dpi.h>
#include <Dpm.h>
#include <QSizeF>
#include <QEvent>
#include <QApplication>
#include <assert.h>

enum StatusLabelPhysSizeDisplayMode {
    Pixels = 0,
    Inch,
    MM,
    SM
};

class StatusBarProvider
{
public:

    static void registerStatusBar(QStatusBar* _statusBar)
    {
        m_statusBar = _statusBar;
        assert(m_statusBar);
    }

    static void changeFilterIdx(int idx);
    static void setOutputFilterIdx(int idx)
    {
        m_outputFilterIdx = idx;
        m_pageSize = QSizeF();
        notify();
    }
    static void setPagePhysSize(const QSizeF _pageSize, const Dpi _originalDpi);
    static void setSettingsDPi(const Dpi _settingsDpi)
    {
        m_settingsDpi = _settingsDpi;
        notify();
    }

    static QSizeF getPageSize() { return m_pageSize; }
    static Dpi getOriginalDpi() { return m_originalDpi; }
    static Dpi getSettingsDpi() { return m_settingsDpi; notify(); }

    static void toggleStatusLabelPhysSizeDisplayMode()
    {
        statusLabelPhysSizeDisplayMode = (statusLabelPhysSizeDisplayMode != SM) ?
                    (StatusLabelPhysSizeDisplayMode) ((int)statusLabelPhysSizeDisplayMode+1) : Pixels;
    }

public:
    static QEvent::Type StatusBarEventType;
    static StatusLabelPhysSizeDisplayMode statusLabelPhysSizeDisplayMode;
private:
    static void notify() { QApplication::postEvent(m_statusBar, new QEvent(StatusBarEventType)); }

private:
    static QStatusBar* m_statusBar;
    static QSizeF m_pageSize;
    static Dpi m_originalDpi;
    static Dpi m_settingsDpi;
    static int m_filterIdx;
    static int m_outputFilterIdx;

};

#endif
