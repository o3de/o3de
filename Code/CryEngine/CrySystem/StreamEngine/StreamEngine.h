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

// Description : Streaming Engine


#ifndef CRYINCLUDE_CRYSYSTEM_STREAMENGINE_STREAMENGINE_H
#define CRYINCLUDE_CRYSYSTEM_STREAMENGINE_STREAMENGINE_H
#pragma once


#include "IStreamEngine.h"
#include "ISystem.h"
#include "TimeValue.h"

#include <CryThread.h>
#include "StreamIOThread.h"
#include "StreamReadStream.h"

#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/std/chrono/clocks.h>
#include <AzCore/std/containers/queue.h>

enum EIOThread
{
    eIOThread_HDD = 0,
    eIOThread_Optical = 1,
    eIOThread_InMemory = 2,
    eIOThread_Last = 3,
};

//////////////////////////////////////////////////////////////////////////
class CStreamEngine
    : public IStreamEngine
    , public ISystemEventListener
    , public AzFramework::InputChannelEventListener
{
public:
    CStreamEngine();
    ~CStreamEngine();

    void Shutdown();
    // This is called to cancel all pending requests, without sending callbacks.
    void CancelAll();


    //Helper added to aid in migration from Cry's CStreamEngine to AZ::IO::Streamer
    static AZ::IO::IStreamerTypes::Priority CryStreamPriorityToAZStreamPriority(EStreamTaskPriority cryPriority);
    static AZStd::chrono::milliseconds AZDeadlineFromReadParams(const StreamReadParams& params);

    //////////////////////////////////////////////////////////////////////////
    // IStreamEngine interface
    //////////////////////////////////////////////////////////////////////////
    IReadStreamPtr StartRead (const EStreamTaskType tSource, const char* szFile, IStreamCallback* pCallback, const StreamReadParams* pParams = NULL);
    size_t StartBatchRead(IReadStreamPtr* pStreamsOut, const StreamReadBatchParams* pReqs, size_t numReqs, AZStd::function<void()>* preRequestCallback = nullptr);
    void BeginReadGroup();
    void EndReadGroup();

    bool IsStreamDataOnHDD() const { return m_bStreamDataOnHDD; }
    void SetStreamDataOnHDD(bool bFlag) { m_bStreamDataOnHDD = bFlag; }

    void Update();
    void UpdateAndWait(bool bAbortAll = false);
    void Update(uint32 nUpdateTypesBitmask);

    void GetMemoryStatistics(ICrySizer* pSizer);

#if defined(STREAMENGINE_ENABLE_STATS)
    SStreamEngineStatistics& GetStreamingStatistics();
    void ClearStatistics();

    void GetBandwidthStats(EStreamTaskType type, float* bandwidth);
#endif

    void GetStreamingOpenStatistics(SStreamEngineOpenStats& openStatsOut);

    const char* GetStreamTaskTypeName(EStreamTaskType type);

    SStreamJobEngineState GetJobEngineState();
    SStreamEngineTempMemStats& GetTempMemStats() { return m_tempMem; }

    // Will pause or unpause streaming of specified by mask data types
    void PauseStreaming(bool bPause, uint32 nPauseTypesBitmask);
    // Pause/resumes any IO active from the streaming engine
    void PauseIO(bool bPause);

    uint32 GetPauseMask() const { return m_nPausedDataTypesMask; }

#if defined(STREAMENGINE_ENABLE_LISTENER)
    void SetListener(IStreamEngineListener* pListener);
    IStreamEngineListener* GetListener();
#endif

    //////////////////////////////////////////////////////////////////////////

    // updates the job priority of an IO job into the IOQueue while maintaining order in the queue
    void UpdateJobPriority(IReadStreamPtr pJobStream);

    void ReportAsyncFileRequestComplete(CAsyncIOFileRequest_AutoPtr pFileRequest);
    void AbortJob(CReadStream* pStream);


    // Dispatches synchrnous callbacks, free temporary memory hold for callbacks.
    void MainThread_FinalizeIOJobs();
    void MainThread_FinalizeIOJobs(uint32 type);

    void* TempAlloc(size_t nSize, const char* szDbgSource, bool bFallBackToMalloc = true, bool bUrgent = false, uint32 align = 0);
    void TempFree(void* p, size_t nSize);

    uint32 GetCurrentTempMemorySize() const { return m_tempMem.m_nTempAllocatedMemory; }
    void FlagTempMemOutOfBudget()
    {
#ifdef STREAMENGINE_ENABLE_STATS
        m_bTempMemOutOfBudget = true;
#endif
    }

    //////////////////////////////////////////////////////////////////////////
    // AzFramework::InputChannelEventListener
    //////////////////////////////////////////////////////////////////////////
    bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
    //////////////////////////////////////////////////////////////////////////

    bool StartFileRequest(CAsyncIOFileRequest* pFileRequest);
    void SignalToStartWork(EIOThread e, bool bForce);

private:
    void StartThreads();
    void StopThreads();

    void ResumePausedStreams_PauseLocked();

#if defined(STREAMENGINE_ENABLE_STATS)
    // add job to current statistics
    void UpdateStatistics(CReadStream* pReadStream);
    void DrawStatistics();
#endif

    void QueueRequestCompleteJob(class AZRequestReadStream* stream, AZ::IO::SizeType numBytesRead, void* buffer,
        AZ::IO::IStreamerTypes::RequestStatus requestState);

    //////////////////////////////////////////////////////////////////////////
    // ISystemEventListener
    //////////////////////////////////////////////////////////////////////////
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    //////////////////////////////////////////////////////////////////////////

private:

    //////////////////////////////////////////////////////////////////////////
    CryMT::set<CReadStream_AutoPtr> m_streams;
    CryMT::vector<CReadStream_AutoPtr> m_finishedStreams;
    std::vector<CReadStream_AutoPtr> m_tempFinishedStreams;

    CryCriticalSection m_pendingRequestCompletionsLock;
    AZStd::queue<AZ::Job*> m_pendingRequestCompletions;

    // 2 IO threads.
    _smart_ptr<CStreamingIOThread> m_pThreadIO[eIOThread_Last];
    std::vector<_smart_ptr<CStreamingWorkerThread> > m_asyncCallbackThreads;
    std::vector<SStreamRequestQueue*> m_asyncCallbackQueues;

    CryCriticalSection m_pausedLock;
    std::vector<CReadStream_AutoPtr> m_pausedStreams;
    volatile uint32 m_nPausedDataTypesMask;

    bool m_bStreamDataOnHDD;
    bool m_bUseOpticalDriveThread;

    //////////////////////////////////////////////////////////////////////////
    // Streaming statistics.
    //////////////////////////////////////////////////////////////////////////

#ifdef STREAMENGINE_ENABLE_LISTENER
    IStreamEngineListener* m_pListener;
#endif

#ifdef STREAMENGINE_ENABLE_STATS
    SStreamEngineStatistics m_Statistics;
    SStreamEngineDecompressStats m_decompressStats;
    CTimeValue m_TimeOfLastReset;
    CTimeValue m_TimeOfLastUpdate;

    CryCriticalSection m_csStats;
    std::vector<CAsyncIOFileRequest_AutoPtr> m_statsRequestList;

    struct SExtensionInfo
    {
        SExtensionInfo()
            : m_fTotalReadTime(0.0f)
            , m_nTotalRequests(0)
            , m_nTotalReadSize(0)
            , m_nTotalRequestSize(0)
        {
        }
        float m_fTotalReadTime;
        size_t m_nTotalRequests;
        uint64 m_nTotalReadSize;
        uint64 m_nTotalRequestSize;
    };
    typedef std::map<string, SExtensionInfo> TExtensionInfoMap;
    TExtensionInfoMap m_PerExtensionInfo;

    //////////////////////////////////////////////////////////////////////////
    // Used to calculate unzip/verify bandwidth for statistics.
    uint32 m_nUnzipBandwidth;
    uint32 m_nUnzipBandwidthAverage;
    uint32 m_nVerifyBandwidth;
    uint32 m_nVerifyBandwidthAverage;
    CTimeValue m_nLastBandwidthUpdateTime;

    bool m_bStreamingStatsPaused;
    bool m_bInputCallback;
    bool m_bTempMemOutOfBudget;
    //////////////////////////////////////////////////////////////////////////
#endif

    SStreamEngineOpenStats m_OpenStatistics;

    bool m_bShutDown;

    volatile int m_nBatchMode;

    // Memory currently allocated by streaming engine for temporary storage.
    SStreamEngineTempMemStats m_tempMem;
};

#endif // CRYINCLUDE_CRYSYSTEM_STREAMENGINE_STREAMENGINE_H
