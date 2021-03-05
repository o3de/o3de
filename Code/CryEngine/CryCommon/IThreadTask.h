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

#include "BitFiddling.h"

#ifndef CRYINCLUDE_CRYCOMMON_ITHREADTASK_H
#define CRYINCLUDE_CRYCOMMON_ITHREADTASK_H
#pragma once

#include <smartptr.h>

// forward declarations
struct SThreadTaskInfo;

enum EThreadTaskFlags
{
    THREAD_TASK_BLOCKING = BIT(0),              // Blocking tasks will be allocated on their own thread.
    THREAD_TASK_ASSIGN_TO_POOL = BIT(1),    // Task can be assigned to any thread in the group of threads
};

class IThreadTask_Thread
{
public:
    // <interfuscator:shuffle>
    virtual ~IThreadTask_Thread() {};
    virtual void AddTask(SThreadTaskInfo* pTaskInfo) = 0;
    virtual void RemoveTask(SThreadTaskInfo* pTaskInfo) = 0;
    virtual void RemoveAllTasks() = 0;
    virtual void SingleUpdate() = 0;
    // </interfuscator:shuffle>
};

typedef int ThreadPoolHandle;

struct SThreadTaskParams
{
    uint32 nFlags;             // Task flags. @see ETaskFlags
    union
    {
        int                             nPreferedThread;    // Preferred Thread index (0,1,2,3...)
        ThreadPoolHandle    nThreadsGroupId;    // Id of group of threads(useful only if THREAD_TASK_ASSIGN_TO_POOL is set)
    };
    int16                               nPriorityOff;               // If THREAD_TASK_BLOCKING, this will adjust the priority of the thread
    int16                               nStackSizeKB;               // If THREAD_TASK_BLOCKING, this will adjust the stack size of the thread
    const char* name;       // Name for this task (thread for the blocking task will be named using this string)

    SThreadTaskParams()
        : nFlags(0)
        , nPreferedThread(-1)
        , nPriorityOff(0)
        , name("")
        , nStackSizeKB(SIMPLE_THREAD_STACK_SIZE_KB) {}
};

//////////////////////////////////////////////////////////////////////////
// Tasks must implement this interface.
//////////////////////////////////////////////////////////////////////////
struct IThreadTask
{
    // <interfuscator:shuffle>
    // The function to be called on every update for non bocking tasks.
    // Or will be called only once for the blocking threads.
    virtual void OnUpdate() = 0;

    // Called to indicate that this task must quit.
    // Warning! can be called from different thread then OnUpdate call.
    virtual void Stop() = 0;

    // Returns task info
    virtual struct SThreadTaskInfo* GetTaskInfo() = 0;

    virtual ~IThreadTask() {}
    // </interfuscator:shuffle>
};

struct SThreadTaskInfo
    : public CMultiThreadRefCount
{
    IThreadTask_Thread* m_pThread;
    IThreadTask* m_pTask;
    SThreadTaskParams       m_params;

    SThreadTaskInfo()
        : m_pThread(NULL)
        , m_pTask(NULL) { m_params.nFlags = 0; m_params.nPreferedThread = -1; }
};

// Might be changed to uint64 etc in the future
typedef uint32 ThreadPoolAffinityMask;
#define INVALID_AFFINITY 0

//////////////////////////////////////////////////////////////////////////
// Description of thread pool to create
//////////////////////////////////////////////////////////////////////////
struct ThreadPoolDesc
{
    ThreadPoolAffinityMask  AffinityMask;       // number of bits means number of threads. affinity overlapping is prohibited
    string                                  sPoolName;
    int32                                       nThreadPriority;
    int32                                       nThreadStackSizeKB;

    ThreadPoolDesc()
        : AffinityMask(INVALID_AFFINITY)
        , sPoolName("UnnamedPool")
        , nThreadPriority(-1)
        , nThreadStackSizeKB(-1) { }

    ILINE bool CreateThread(ThreadPoolAffinityMask affinityMask)
    {
        if (this->AffinityMask & affinityMask)
        {
            return false;
        }

        this->AffinityMask |= affinityMask;
        return true;
    }

    ILINE uint32 GetThreadCount() const
    {
        return CountBits(AffinityMask);
    }
};

//////////////////////////////////////////////////////////////////////////
// Task manager.
//////////////////////////////////////////////////////////////////////////
struct IThreadTaskManager
{
    // <interfuscator:shuffle>
    virtual ~IThreadTaskManager(){}
    // Register new task to the manager.
    virtual void RegisterTask(IThreadTask* pTask, const SThreadTaskParams& options) = 0;
    virtual void UnregisterTask(IThreadTask* pTask) = 0;

    // Limit number of threads to this amount.
    virtual void SetMaxThreadCount(int nMaxThreads) = 0;

    // Create a pool of threads
    virtual ThreadPoolHandle CreateThreadsPool(const ThreadPoolDesc& desc) = 0;
    virtual const bool DestroyThreadsPool(const ThreadPoolHandle& handle) = 0;
    virtual const bool GetThreadsPoolDesc(const ThreadPoolHandle handle, ThreadPoolDesc* pDesc) const = 0;
    virtual const bool SetThreadsPoolAffinity(const ThreadPoolHandle handle, const ThreadPoolAffinityMask AffinityMask) = 0;

    virtual void SetThreadName(threadID dwThreadId, const char* sThreadName) = 0;
    virtual const char* GetThreadName(threadID dwThreadId) = 0;

    // Return thread handle by thread name
    virtual threadID GetThreadByName(const char* sThreadName) = 0;

    // if bMark=true the calling thread will dump its stack during crashes
    virtual void MarkThisThreadForDebugging(const char* name, bool bDump) = 0;
    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_ITHREADTASK_H
