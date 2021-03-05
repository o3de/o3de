/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_BACKGROUNDTASKMANAGER_H
#define CRYINCLUDE_EDITOR_BACKGROUNDTASKMANAGER_H
#pragma once

#include "Include/IBackgroundTaskManager.h"
#include "CryListenerSet.h"

#include <QThread>

namespace BackgroundTaskManager
{
    typedef int TTaskID;

    struct STaskHandle
    {
        ETaskPriority priority;
        ETaskThreadMask threadMask;
        TTaskID id;
        IBackgroundTask* pTask;

        bool operator<(const STaskHandle& rhs) const
        {
            if (priority < rhs.priority)
            {
                return true;
            }
            if (priority > rhs.priority)
            {
                return false;
            }
            return id < rhs.id;
        }
    };

    struct SCompletedTask
    {
        ETaskResult state;
        TTaskID id;
        ETaskThreadMask threadMask;
        IBackgroundTask* pTask;
    };

    struct SScheduledTask
    {
        unsigned int time;
        STaskHandle handle;
    };

    class CTaskManager
        : public IBackgroundTaskManager
        , public IEditorNotifyListener
    {
    public:
        CTaskManager();
        ~CTaskManager();

        // IBackgroundTaskManager interface implementation
        virtual void AddTask(IBackgroundTask* pTask, ETaskPriority priority, ETaskThreadMask threadMask) override;
        virtual void ScheduleTask(IBackgroundTask* pTask, ETaskPriority priority, int delayMilliseconds, ETaskThreadMask threadMask) override;
        void AddListener(IBackgroundTaskManagerListener* pListener, const char* name) override;
        void RemoveListener(IBackgroundTaskManagerListener* pListener) override;

    private:
        // IEditorNotifyListener interface implementation
        virtual void OnEditorNotifyEvent(EEditorNotifyEvent ev) override;

        void Start(const uint32 threadCount = kDefaultThreadCount);
        void Stop();
        void StartScheduledTasks();
        void AddTask(const STaskHandle& outTask);
        void AddCompletedTask(const STaskHandle& outTask, ETaskResult resultState);
        void Update();

        inline bool IsStopped() const
        {
            return m_bStop;
        }

    private:
        // Internal queue (per thread mask)
        class CQueue
        {
        public:
            CQueue();

            // Add task to list
            void AddTask(const STaskHandle& taskHandle);

            // Pop task from list
            void PopTask(STaskHandle& outTaskHandle);

            // Release thread semaphore without adding a task
            void ReleaseSemaphore();

            // Remove all pending tasks
            void Clear();

        private:
            CrySemaphore m_semaphore;
            std::vector<STaskHandle> m_pendingTasks;
            CryMutex m_lock;
        };

        // Worker thread class implementation
        class CThread : public QThread
        {
        public:
            CThread(CTaskManager* pManager, CQueue* pQueue);
            ~CThread();

            void WaitForThread();

        private:
            void run() override;

        private:
            CTaskManager* m_pManager;
            CQueue* m_pQueue;
        };

    private:
        static const uint32 kMaxThreadCloseWaitTime = 10000; // ms
        static const uint32 kDefaultThreadCount = 4; // good enough for LiveCreate (main user right now), do not set to less than 2

        CQueue m_pendingTasks[ eTaskThreadMask_COUNT ];

        // Task scheduled for execution in the future
        std::vector<SScheduledTask> m_scheduledTasks;

        // Completed tasks (waiting for the "finalize" call)
        std::vector<SCompletedTask> m_completedTasks;

        volatile TTaskID m_nextTaskID;

        typedef std::vector<CThread*> TWorkerThreads;
        TWorkerThreads m_pThreads;

        CryMutex m_tasksLock;
        bool m_bStop;

        typedef CListenerSet<IBackgroundTaskManagerListener*> TListeners;
        TListeners m_listeners;
    };
}

//-----------------------------------------------------------------------------

#endif // CRYINCLUDE_EDITOR_BACKGROUNDTASKMANAGER_H
