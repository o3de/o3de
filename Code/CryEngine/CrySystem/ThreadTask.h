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

#ifndef CRYINCLUDE_CRYSYSTEM_THREADTASK_H
#define CRYINCLUDE_CRYSYSTEM_THREADTASK_H
#pragma once


#include <IThreadTask.h>
#include <CryThread.h>
#include <MultiThread_Containers.h>

#define MAIN_THREAD_INDEX 0

class CThreadTask_Thread;


void MarkThisThreadForDebugging(const char* name);
void UnmarkThisThreadFromDebugging();
void UpdateFPExceptionsMaskForThreads();


class CThreadTaskManager;
///
struct IThreadTaskRunnable
{
    virtual ~IThreadTaskRunnable(){}
    virtual void Run() = 0;
    virtual void Cancel() = 0;
};
//////////////////////////////////////////////////////////////////////////
class CThreadTask_Thread
    : public CryThread<IThreadTaskRunnable>
    , public IThreadTask_Thread
{
protected:
    void Init();
public:
    CThreadTask_Thread(CThreadTaskManager* pTaskMgr, const char* sName, int nThreadIndex, int nProcessor, int nThreadPriority, ThreadPoolHandle poolHandle = -1);
    ~CThreadTask_Thread();

    // see IThreadTaskRunnable, CryThread<>
    void Run() override;
    void Cancel() override;

    // see CryThread<>
    void Terminate() override;

    // IThreadTask_Thread
    void AddTask(SThreadTaskInfo* pTaskInfo) override;
    void RemoveTask(SThreadTaskInfo* pTaskInfo) override;
    void RemoveAllTasks() override;
    void SingleUpdate() override;

    void ChangeProcessor(int nProcessor);
public:
    CThreadTaskManager* m_pTaskManager;
    string m_sThreadName;
    int m_nThreadIndex;                 // -1 means the thread is blocking
    int m_nProcessor;
    int m_nThreadPriority;

    THREAD_HANDLE m_hThreadHandle;

    // Tasks running on this thread.
    typedef CryMT::CLocklessPointerQueue<SThreadTaskInfo, stl::STLGlobalAllocator<SThreadTaskInfo> > Tasks;
    Tasks tasks;

    // The task is being processing now
    SThreadTaskInfo*    m_pProcessingTask;

    CryEvent m_waitForTasks;

    // Set to true when thread must stop.
    volatile bool bStopThread;
    volatile bool bRunning;

    // handle of threads pool which this thread belongs to(if any)
    ThreadPoolHandle m_poolHandle;
};

//////////////////////////////////////////////////////////////////////////
class CThreadTaskManager
    : public IThreadTaskManager
{
private:
    typedef std::vector<CThreadTask_Thread*,  stl::STLGlobalAllocator<CThreadTask_Thread*> > Threads;
    // note: this struct is auxilary and NOT thread-safe
    // it is only for internal use inside the task manager
    struct CThreadsPool
    {
        ThreadPoolHandle    m_hHandle;
        Threads                     m_Threads;
        ThreadPoolDesc      m_pDescription;
        const bool SetAffinity(const ThreadPoolAffinityMask AffinityMask);
        const bool operator < (const CThreadsPool& p) const { return m_hHandle < p.m_hHandle; }
        const bool operator == (const CThreadsPool& p) const { return m_hHandle == p.m_hHandle; }
    };

    typedef std::vector<CThreadsPool> ThreadsPools;

public:
    CThreadTaskManager();
    ~CThreadTaskManager();

    void InitThreads();
    void CloseThreads();
    void StopAllThreads();

    //////////////////////////////////////////////////////////////////////////
    // IThreadTaskManager
    //////////////////////////////////////////////////////////////////////////
    virtual void RegisterTask(IThreadTask* pTask, const SThreadTaskParams& options);
    virtual void UnregisterTask(IThreadTask* pTask);
    virtual void SetMaxThreadCount(int nMaxThreads);
    virtual void SetThreadName(threadID dwThreadId, const char* sThreadName);
    virtual const char* GetThreadName(threadID dwThreadId);
    virtual threadID GetThreadByName(const char* sThreadName);

    // Thread pool framework
    virtual ThreadPoolHandle CreateThreadsPool(const ThreadPoolDesc& desc);
    virtual const bool DestroyThreadsPool(const ThreadPoolHandle& handle);
    virtual const bool GetThreadsPoolDesc(const ThreadPoolHandle handle, ThreadPoolDesc* pDesc) const;
    virtual const bool SetThreadsPoolAffinity(const ThreadPoolHandle handle, const ThreadPoolAffinityMask AffinityMask);

    virtual void MarkThisThreadForDebugging(const char* name, bool bDump);
    //////////////////////////////////////////////////////////////////////////

    // This is on update function of the main thread.
    void OnUpdate();

    void AddSystemThread(threadID nThreadId);
    void RemoveSystemThread(threadID nThreadId);

    // Balancing tasks in the pool between threads
    void BalanceThreadsPool(const ThreadPoolHandle& handle);
    void BalanceThreadInPool(CThreadTask_Thread* pFreeThread, Threads* pThreads = NULL);

private:
    void ScheduleTask(SThreadTaskInfo* pTaskInfo);
    void RescheduleTasks();
private:

    // User created threads pools
    mutable CryReadModifyLock m_threadsPoolsLock;
    ThreadsPools m_threadsPools;

    // Physical threads available to system.
    Threads m_threads;

    // Threads with single blocking task attached.
    Threads m_blockingThreads;

    typedef CryMT::CLocklessPointerQueue<SThreadTaskInfo> Tasks;

    Tasks   m_unassignedTasks;

    mutable CryCriticalSection m_threadNameLock;
    mutable CryCriticalSection m_threadRemove;
    typedef std::map<threadID, string> ThreadNames;
    ThreadNames m_threadNames;

    mutable CryCriticalSection m_systemThreadsLock;
    std::vector<threadID> m_systemThreads;

    // Max threads that can be executed at same time.
    int m_nMaxThreads;
};


#endif // CRYINCLUDE_CRYSYSTEM_THREADTASK_H
