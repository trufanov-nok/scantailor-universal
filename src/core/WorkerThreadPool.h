/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
    Backported from ScanTailor Advanced (src/core/WorkerThreadPool.h).

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

#ifndef WORKERTHREADPOOL_H_
#define WORKERTHREADPOOL_H_

#include "NonCopyable.h"
#include "BackgroundTask.h"
#include "FilterResult.h"
#include <QObject>

class QThreadPool;

class WorkerThreadPool : public QObject
{
    Q_OBJECT
    DECLARE_NON_COPYABLE(WorkerThreadPool)
public:
    WorkerThreadPool(QObject* parent = 0);

    virtual ~WorkerThreadPool();

    void shutdown();

    bool hasSpareCapacity();

    bool isIdle() const;

    void submitTask(BackgroundTaskPtr const& task);

    static int maxThreadCount();

    static int threadCountSetting();
signals:
    void taskResult(BackgroundTaskPtr const& task, FilterResultPtr const& result);

    void taskFinished();
protected:
    virtual void customEvent(QEvent* event);
private:
    class TaskResultEvent;
    class Runnable;

    void updateNumberOfThreads();

    QThreadPool* m_pPool;
    int m_numPendingTasks;
};

#endif
