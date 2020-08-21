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

class QStatusBarProviderEvent: public QEvent
{
public:

    enum NitificationTypeEnum {
        PhysSizeChanged = 1,
        MousePosChanged = 2
    };

    Q_DECLARE_FLAGS(NitificationType, NitificationTypeEnum)

    QStatusBarProviderEvent(QEvent::Type _t, const QStatusBarProviderEvent::NitificationTypeEnum _type): QEvent(_t), m_type(_type) { }
    void setFlag(const NitificationTypeEnum _type)
    {
        m_type |= _type;
    }
    bool testFlag(const NitificationTypeEnum _type)
    {
        return m_type.testFlag(_type);
    }
private:
    NitificationType m_type;
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
    static void setPagePhysSize(const QSizeF& _pageSize, const Dpi& _originalDpi);
    static void setSettingsDPi(const Dpi& _settingsDpi)
    {
        m_settingsDpi = _settingsDpi;
        notify();
    }

    static void setMousePos(const QPointF& pos)
    {
        m_mousePos = pos;
        notify(QStatusBarProviderEvent::MousePosChanged);
    }

    static QPointF getMousePos()
    {
        return m_mousePos;
    }
    static QSizeF getPageSize()
    {
        return m_pageSize;
    }
    static Dpi getOriginalDpi()
    {
        return m_originalDpi;
    }
    static Dpi getSettingsDpi()
    {
        return m_settingsDpi;
        notify();
    }

    static void toggleStatusLabelPhysSizeDisplayMode()
    {
        statusLabelPhysSizeDisplayMode = (statusLabelPhysSizeDisplayMode != SM) ?
                                         (StatusLabelPhysSizeDisplayMode)((int)statusLabelPhysSizeDisplayMode + 1) : Pixels;
    }

    static QString getStatusLabelPhysSizeDisplayModeSuffix(StatusLabelPhysSizeDisplayMode const mode)
    {
        switch (mode) {
        case Pixels:
            return QObject::tr("px");
        case Inch:
            return QObject::tr("in");
        case MM:
            return QObject::tr("mm");
        case SM:
            return QObject::tr("cm");
        default:
            return QString();
        }
    }

    static QString getStatusLabelPhysSizeDisplayModeSuffix()
    {
        return getStatusLabelPhysSizeDisplayModeSuffix(statusLabelPhysSizeDisplayMode);
    }

public:
    static QEvent::Type StatusBarEventType;
    static StatusLabelPhysSizeDisplayMode statusLabelPhysSizeDisplayMode;
private:
    static void notify(const QStatusBarProviderEvent::NitificationTypeEnum _type = QStatusBarProviderEvent::PhysSizeChanged)
    {
        if (m_statusBar) {
            QApplication::postEvent(m_statusBar, new QStatusBarProviderEvent(StatusBarEventType, _type));
        }
    }

private:
    static QStatusBar* m_statusBar;
    static QSizeF m_pageSize;
    static Dpi m_originalDpi;
    static Dpi m_settingsDpi;
    static int m_filterIdx;
    static int m_outputFilterIdx;
    static QPointF m_mousePos;

};

#endif
