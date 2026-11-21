/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
    Backported from ScanTailor Advanced (src/core/WorkerThreadPool.cpp).

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

#include "WorkerThreadPool.h"

#include "ThreadPriority.h"
#include "OutOfMemoryHandler.h"
#include "settings/ini_keys.h"
#include <QCoreApplication>
#include <QThreadPool>
#include <QRunnable>
#include <QThread>
#include <QSettings>
#include <QEvent>
#include <QtGlobal>
#include <algorithm>
#include <new>

#if defined(Q_OS_LINUX)
#include <sys/resource.h>
#endif

class WorkerThreadPool::TaskResultEvent : public QEvent
{
public:
    TaskResultEvent(BackgroundTaskPtr const& task, FilterResultPtr const& result)
        :   QEvent(User),
            m_ptrTask(task),
            m_ptrResult(result)
    {
    }

    BackgroundTaskPtr const& task() const
    {
        return m_ptrTask;
    }

    FilterResultPtr const& result() const
    {
        return m_ptrResult;
    }
private:
    BackgroundTaskPtr m_ptrTask;
    FilterResultPtr m_ptrResult;
};

class WorkerThreadPool::Runnable : public QRunnable
{
public:
    Runnable(WorkerThreadPool& owner, BackgroundTaskPtr const& task)
        :   m_rOwner(owner),
            m_ptrTask(task)
    {
        setAutoDelete(true);
    }

    virtual void run()
    {
        FilterResultPtr result;
        if (!m_ptrTask->isCancelled()) {
            applyPriority();
            try {
                result = (*m_ptrTask)();
            } catch (std::bad_alloc const&) {
                OutOfMemoryHandler::instance().handleOutOfMemorySituation();
            }
        }

        // The main thread counts the tasks in progress, thus each task must send an event, also without a result.
        QCoreApplication::postEvent(&m_rOwner, new TaskResultEvent(m_ptrTask, result));
    }
private:
    void applyPriority()
    {
        ThreadPriority const prio(
            ThreadPriority::load(
                _key_batch_processing_priority, ThreadPriority::Normal
            )
        );
#if defined(Q_OS_LINUX)
        // On Linux, setpriority() changes only the current thread, and WorkerThread uses the same method.
        setpriority(PRIO_PROCESS, 0, prio.toPosixNiceLevel());
#else
        QThread::currentThread()->setPriority(prio.toQThreadPriority());
#endif
    }

    WorkerThreadPool& m_rOwner;
    BackgroundTaskPtr m_ptrTask;
};

WorkerThreadPool::WorkerThreadPool(QObject* parent)
    :   QObject(parent),
        m_pPool(new QThreadPool(this)),
        m_numPendingTasks(0)
{
    // A new thread starts with the normal priority, but on Linux an old thread cannot get a higher priority again.
    m_pPool->setExpiryTimeout(0);
    updateNumberOfThreads();
}

WorkerThreadPool::~WorkerThreadPool()
{
    // Each runnable keeps a reference to this object, so all runnables must stop before the destructor ends.
    shutdown();
}

void
WorkerThreadPool::shutdown()
{
    m_pPool->waitForDone();
}

bool
WorkerThreadPool::hasSpareCapacity()
{
    updateNumberOfThreads();
    return m_numPendingTasks < m_pPool->maxThreadCount();
}

bool
WorkerThreadPool::isIdle() const
{
    return m_numPendingTasks == 0;
}

void
WorkerThreadPool::submitTask(BackgroundTaskPtr const& task)
{
    updateNumberOfThreads();
    ++m_numPendingTasks;
    m_pPool->start(new Runnable(*this, task));
}

int
WorkerThreadPool::maxThreadCount()
{
    int max_threads = std::max(1, QThread::idealThreadCount());
    if (sizeof(void*) <= 4) {
        // A 32-bit process has too little address space for more pages in memory.
        max_threads = std::min(max_threads, 2);
    }
    return max_threads;
}

int
WorkerThreadPool::threadCountSetting()
{
    QSettings settings;
    int const num_threads = settings.value(
        _key_batch_processing_threads, _key_batch_processing_threads_def
    ).toInt();
    return std::max(1, std::min(num_threads, maxThreadCount()));
}

void
WorkerThreadPool::customEvent(QEvent* event)
{
    if (TaskResultEvent* evt = dynamic_cast<TaskResultEvent*>(event)) {
        --m_numPendingTasks;
        if (evt->result()) {
            emit taskResult(evt->task(), evt->result());
        }
        emit taskFinished();
    }
}

void
WorkerThreadPool::updateNumberOfThreads()
{
    m_pPool->setMaxThreadCount(threadCountSetting());
}
