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

// Description : Streaming Engine implementation


#include "CrySystem_precompiled.h"
#include "StreamEngine.h"

//CReadStream is 99% unused since the introduction of AZRequestReadStream.
//In the near future CReadStream will disappear altogether and StreamReadStream cpp/h
// won't be needed anymore.
#include "StreamReadStream.h"

#include "AZRequestReadStream.h"
#include "Pak/CryPakUtils.h"

#include "../System.h"

#include <AzCore/IO/IStreamer.h>
#include <AzCore/std/sort.h>

#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

#define MAX_HEAVY_ASSETS 20

#if defined(STREAMENGINE_ENABLE_STATS)
SStreamEngineStatistics* g_pStreamingStatistics = 0;
#endif

SStreamEngineOpenStats* g_pStreamingOpenStatistics = 0;

extern CMTSafeHeap* g_pPakHeap;

//////////////////////////////////////////////////////////////////////////
CStreamEngine::CStreamEngine()
    : AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityDebug())
{
    m_nBatchMode = 0;
    m_bShutDown = false;
    m_bUseOpticalDriveThread = g_cvars.sys_streaming_use_optical_drive_thread != 0;
    m_nPausedDataTypesMask = 0;
    m_bStreamDataOnHDD = gEnv->pCryPak->IsInstalledToHDD();

#ifdef STREAMENGINE_ENABLE_STATS
    g_pStreamingStatistics = &m_Statistics;
    m_Statistics.nPendingReadBytes = 0;

    m_Statistics.nCurrentAsyncCount = 0;
    m_Statistics.nCurrentDecompressCount = 0;
    m_Statistics.nCurrentFinishedCount = 0;

    memset(&m_decompressStats, 0, sizeof(m_decompressStats));

    m_nUnzipBandwidth = 0;
    m_nUnzipBandwidthAverage = 0;
    m_bStreamingStatsPaused = false;
    m_bInputCallback = false;
    m_bTempMemOutOfBudget = false;

    ClearStatistics();
#endif

    memset(&m_OpenStatistics, 0, sizeof(m_OpenStatistics));
    g_pStreamingOpenStatistics = &m_OpenStatistics;

#ifdef STREAMENGINE_ENABLE_LISTENER
    m_pListener = NULL;
#endif

    StartThreads();

    // register system listener
    GetISystem()->GetISystemEventDispatcher()->RegisterListener(this);
}

//////////////////////////////////////////////////////////////////////////
// MT: Main thread only
CStreamEngine::~CStreamEngine()
{
#ifdef STREAMENGINE_ENABLE_STATS
    g_pStreamingStatistics = 0;
    if (m_bInputCallback)
    {
        AzFramework::InputChannelEventListener::Disconnect();
    }
#endif
    g_pStreamingOpenStatistics = NULL;
    Shutdown();
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::BeginReadGroup()
{
    CryInterlockedIncrement(&m_nBatchMode);
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::EndReadGroup()
{
    CryInterlockedDecrement(&m_nBatchMode);

    for (int i = 0; i < eIOThread_Last; i++)
    {
        if (m_pThreadIO[i])
        {
            // New requests accomulated untill all Start stream requests are submitted and can be properly sorted.
            m_pThreadIO[i]->SignalStartWork(false);
        }
    }
}

AZ::IO::IStreamerTypes::Priority CStreamEngine::CryStreamPriorityToAZStreamPriority(EStreamTaskPriority cryPriority)
{
    switch (cryPriority)
    {
    case estpUrgent:
        return AZ::IO::IStreamerTypes::s_priorityHighest;
    // estpPreempted = 1, //For internal use only
    case estpAboveNormal:
        return AZ::IO::IStreamerTypes::s_priorityHigh;
    case estpNormal:
        return AZ::IO::IStreamerTypes::s_priorityMedium;
    case estpBelowNormal:
        return AZ::IO::IStreamerTypes::s_priorityLow;
    case estpIdle:
        [[fallthrough]];
    default:
        return AZ::IO::IStreamerTypes::s_priorityLowest;
    }
}

AZStd::chrono::milliseconds CStreamEngine::AZDeadlineFromReadParams(const StreamReadParams& params)
{

    if (params.nLoadTime == 0)
    {
        // File should be loaded right away.
        return AZStd::chrono::milliseconds(0);
    }
    else
    {
        return AZStd::chrono::milliseconds(AZStd::max(params.nLoadTime, params.nMaxLoadTime));
    }
}

//////////////////////////////////////////////////////////////////////////
// Starts asynchronous read from the specified file
// It is expected that the callbacks are called from Main Thread only when
// the async data loading is finished.
IReadStreamPtr CStreamEngine::StartRead (const EStreamTaskType tSource, const char* szFilePath, IStreamCallback* pCallback, const StreamReadParams* pParams)
{
    using namespace AZ::IO;

    if (!szFilePath)
    {
        CryFatalError("Use of the stream engine without a file is deprecated! Use the job system.");
        return NULL;
    }

    if (gEnv->IsDedicated())
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_WARNING, "Attempting to use the stream engine on a dedicated server!  Don't do that!");
        return NULL;
    }

    if (!m_bShutDown)
    {
        AZRequestReadStream* pStream = AZRequestReadStream::Allocate(tSource, szFilePath, pCallback, pParams);
        if (!pStream)
        {
            CryFatalError("Failed to create Request Stream for %s", szFilePath);
            return nullptr;
        }

        size_t offset = pParams ? pParams->nOffset : 0;
        AZStd::chrono::microseconds deadline = pParams
            ? AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZDeadlineFromReadParams(*pParams))
            : AZStd::chrono::microseconds(0);
        AZ::IO::IStreamerTypes::Priority priority = pParams
            ? CryStreamPriorityToAZStreamPriority(pParams->ePriority)
            : AZ::IO::IStreamerTypes::s_priorityHighest;

        // Add a ref to stream before binding to the callback. Callback will release the reference when it's invoked.
        pStream->AddRef();

        auto callback = [this, pStream](FileRequestHandle request)
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzRenderDetailed);
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            void* buffer = nullptr;
            AZ::u64 bytesRead = 0;
            [[maybe_unused]] bool result = streamer->GetReadRequestResult(request, buffer, bytesRead);
            AZ_Assert(result, "Cry Stream Engine requested a callback on reading, but couldn't retrieve result.");
            QueueRequestCompleteJob(pStream, bytesRead, buffer, streamer->GetRequestStatus(request));
            // Release reference that was taken above in order to hold onto stream while job is queued
            pStream->Release();
        };

        // Register stream and start file request.
        IReadStreamPtr result = static_cast<IReadStreamPtr>(pStream);

        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
        FileRequestPtr azRequest = streamer->Read(szFilePath, pStream->GetFileReadBuffer(), pStream->GetFileSize(),
            pStream->GetFileSize(), deadline, priority, offset);
        streamer->SetRequestCompleteCallback(azRequest, callback);
        pStream->SetFileRequest(azRequest);
        streamer->QueueRequest(azRequest);

        return result;
    }

    return NULL;
}

//It is NOT necessary to schedule the callbacks on the main thread.
//Regular async calls is OK.
size_t CStreamEngine::StartBatchRead(IReadStreamPtr* pStreamsOut, const StreamReadBatchParams* pReqs, size_t numReqs, AZStd::function<void()>* preRequestCallback)
{
    using namespace AZ::IO;

    FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);

    size_t nValidStreams = 0;

    if (!m_bShutDown)
    {
        enum
        {
            MaxStreamsPerBatch = 32
        };

        size_t nReqIdx = 0;

        // we have requests to evaluate, call the callback before enqueing the requests
        if (numReqs > 0 && preRequestCallback != nullptr)
        {
            (*preRequestCallback)();
        }

        if (numReqs > 0)
        {
            numReqs = AZStd::min(numReqs, aznumeric_cast<size_t>(MaxStreamsPerBatch));
            AZStd::vector<FileRequestPtr> batch;

            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            streamer->CreateRequestBatch(batch, numReqs);

            while (numReqs > 0)
            {
                const StreamReadBatchParams& args = pReqs[nReqIdx];

                if (!args.szFile)
                {
                    CryFatalError("Use of the stream engine without a file is deprecated! Use the job system.");
                }

                AZRequestReadStream* pStream;

                {
                    FRAME_PROFILER_FAST("CStreamEngine::StartBatchRead_AllocReadStream", gEnv->pSystem, PROFILE_SYSTEM, gEnv->bProfilerEnabled);
                    pStream = AZRequestReadStream::Allocate(args.tSource, args.szFile, args.pCallback, &args.params);
                }

                if (pStream)
                {
                    FileRequestPtr& request = batch[nValidStreams];
                    pStreamsOut[nValidStreams++] = pStream;

                    // Add a ref to stream before binding to the callback. Callback will release the reference when it's invoked.
                    pStream->AddRef();
                    auto callback = [this, pStream](FileRequestHandle request)
                    {
                        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzRenderDetailed);
                        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                        void* buffer = nullptr;
                        AZ::u64 bytesRead = 0;
                        [[maybe_unused]] bool result = streamer->GetReadRequestResult(request, buffer, bytesRead);
                        AZ_Assert(result, "Cry Stream Engine requested a callback on reading, but couldn't retrieve result.");
                        QueueRequestCompleteJob(pStream, bytesRead, buffer, streamer->GetRequestStatus(request));
                        // Release reference that was taken above in order to hold onto stream while job is queued
                        pStream->Release();
                    };

                    streamer->Read(request, args.szFile, pStream->GetFileReadBuffer(), pStream->GetFileSize(),
                        pStream->GetFileSize(), AZDeadlineFromReadParams(args.params),
                        CryStreamPriorityToAZStreamPriority(args.params.ePriority), args.params.nOffset);
                    streamer->SetRequestCompleteCallback(request, callback);
                    pStream->SetFileRequest(request);
                }
                else
                {
                    CryFatalError("Failed to create Request Stream for %s at mip number %d", args.szFile, (int)nReqIdx);
                }

                --numReqs;
                ++nReqIdx;
            }

            streamer->QueueRequestBatch(AZStd::move(batch));
        }
    }

    return nValidStreams;
}

void CStreamEngine::QueueRequestCompleteJob(AZRequestReadStream* stream, AZ::IO::SizeType numBytesRead, void* buffer,
    AZ::IO::IStreamerTypes::RequestStatus requestState)
{
    // Some graphics APIs don't support multiple threads instancing resources such as textures. To work around this limitation
    // the jobs that complete a streaming request are queued and a previous request will kick off the next one. This will cause
    // only one job that finishes a streaming request to ever be active without causing mutexes to cause stalls in the job system.

    // Add a ref to stream before binding to the callback. Callback will release the reference when it's invoked.
    stream->AddRef();
    auto jobFunction = [this, stream, numBytesRead, buffer, requestState]()
    {
        stream->OnRequestComplete(numBytesRead, buffer, requestState);
        // Release reference that was taken above in order to hold onto stream while job is queued
        stream->Release();

        CryAutoLock<CryCriticalSection> lock(m_pendingRequestCompletionsLock);
        AZ_Assert(!m_pendingRequestCompletions.empty(), 
            "CStreamEngine::QueueRequestCompleteJob expects at least one job in the queue as this is this is the job ran from the callback.")
        // The top request is always the one that's running, so pop that one of the queue and start any other pending jobs.
        m_pendingRequestCompletions.pop();
        if (!m_pendingRequestCompletions.empty())
        {
            m_pendingRequestCompletions.front()->Start();
        }
    };

    AZ::Job* job = AZ::CreateJobFunction(jobFunction, true, AZ::JobContext::GetGlobalContext());

    CryAutoLock<CryCriticalSection> lock(m_pendingRequestCompletionsLock);
    if (m_pendingRequestCompletions.empty())
    {
        m_pendingRequestCompletions.push(job);
        job->Start();
    }
    else
    {
        m_pendingRequestCompletions.push(job);
    }
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::ResumePausedStreams_PauseLocked()
{
    for (size_t i = 0; i < (size_t)m_pausedStreams.size(); )
    {
        CReadStream* pStream = (CReadStream*)(IReadStream*)m_pausedStreams[i];
        int nStreamMask = 1 << (uint32)pStream->m_Type;
        if (0 == (nStreamMask & m_nPausedDataTypesMask))
        {
            if (pStream->GetError() == 0) // If was not aborted
            {
                // This stream must be resumed
                m_streams.insert(pStream);
                CAsyncIOFileRequest* pFileRequest = pStream->CreateFileRequest();
                if (!StartFileRequest(pFileRequest))
                {
                    pFileRequest->Release();
                }
            }
            m_pausedStreams.erase(m_pausedStreams.begin() + i);
        }
        else
        {
            i++;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CStreamEngine::StartFileRequest(CAsyncIOFileRequest* pFileRequest)
{
    bool bStartImmidietly = m_nBatchMode == 0;

    EStreamSourceMediaType eMediaType = pFileRequest->GetMediaType();
    bool bQueued = false;

    CStreamingIOThread* pIO = m_pThreadIO[0];

    for (size_t i = 1; i < eIOThread_Last; ++i)
    {
        CStreamingIOThread* pAltIO = m_pThreadIO[i];

        if (pAltIO && pAltIO->GetMediaType() == eMediaType)
        {
            pIO = pAltIO;
            break;
        }
    }

    if (pIO)
    {
#ifdef STREAMENGINE_ENABLE_LISTENER
        if (m_pListener)
        {
            m_pListener->OnStreamEnqueue(pFileRequest, pFileRequest->m_strFileName.c_str(), pFileRequest->m_pReadStream->GetCallerType(), pFileRequest->m_pReadStream->GetParams());
        }
#endif

        pIO->AddRequest(pFileRequest, bStartImmidietly);
        bQueued = true;
    }

    if (!bQueued)
    {
        assert(0); // No IO thread.
        return false;
    }

#ifdef STREAMENGINE_ENABLE_STATS
    m_Statistics.typeInfo[pFileRequest->m_eType].nTotalStreamingRequestCount++;

    if (g_cvars.sys_streaming_debug == 3)
    {
        const char* const sFileFilter = g_cvars.sys_streaming_debug_filter_file_name->GetString();

        if (!pFileRequest->m_strFileName.empty() && !m_bStreamingStatsPaused)
        {
            if (!sFileFilter || !sFileFilter[0] || strstr(pFileRequest->m_strFileName.c_str(), sFileFilter))
            {
                CryAutoCriticalSection lock(m_csStats);
                m_statsRequestList.insert(m_statsRequestList.begin(), pFileRequest);
            }
        }
    }
#endif

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::SignalToStartWork(EIOThread e, bool bForce)
{
    if ((e >= 0) && (e < eIOThread_Last))
    {
        if (m_pThreadIO[e])
        {
            m_pThreadIO[e]->SignalStartWork(bForce);
        }
    }
}

//////////////////////////////////////////////////////////////////////////

#ifdef STREAMENGINE_ENABLE_STATS
void UpdateIOThreadStats(
    SStreamEngineStatistics::SMediaTypeInfo* pNotInMemoryInfo,
    SStreamEngineStatistics::SMediaTypeInfo* pInMemoryInfo,
    CStreamingIOThread* pIOThread,
    float fSecSinceLastReset)
{
    if (pNotInMemoryInfo == 0 || pIOThread == 0)
    {
        return;
    }

    // not in memory reading
    pNotInMemoryInfo->fActiveDuringLastSecond = pIOThread->m_NotInMemoryStats.m_fReadingDuringLastSecond;
    pNotInMemoryInfo->fAverageActiveTime =
        pIOThread->m_NotInMemoryStats.m_TotalReadTime.GetSeconds() / fSecSinceLastReset * 100;

    pNotInMemoryInfo->nBytesRead = pIOThread->m_NotInMemoryStats.m_nReadBytesInLastSecond;
    pNotInMemoryInfo->nRequestCount = pIOThread->m_NotInMemoryStats.m_nRequestCountInLastSecond;
    pNotInMemoryInfo->nTotalBytesRead = pIOThread->m_NotInMemoryStats.m_nTotalReadBytes;
    pNotInMemoryInfo->nTotalRequestCount = pIOThread->m_NotInMemoryStats.m_nTotalRequestCount;

    pNotInMemoryInfo->nSeekOffsetLastSecond = pIOThread->m_NotInMemoryStats.m_nReadOffsetInLastSecond;
    if (pIOThread->m_NotInMemoryStats.m_nTotalRequestCount > 0)
    {
        pNotInMemoryInfo->nAverageSeekOffset = pIOThread->m_NotInMemoryStats.m_nTotalReadOffset /
            pIOThread->m_NotInMemoryStats.m_nTotalRequestCount;
    }
    else
    {
        pNotInMemoryInfo->nAverageSeekOffset = 0;
    }

    pNotInMemoryInfo->nCurrentReadBandwidth = pIOThread->m_NotInMemoryStats.m_nCurrentReadBandwith;
    pNotInMemoryInfo->nSessionReadBandwidth = (uint32)(pNotInMemoryInfo->nTotalBytesRead / fSecSinceLastReset);

    pNotInMemoryInfo->nActualReadBandwidth = pIOThread->m_NotInMemoryStats.m_nActualReadBandwith;
    float fTotalReadTime = pIOThread->m_NotInMemoryStats.m_TotalReadTime.GetSeconds();
    if (fTotalReadTime > 0.0f)
    {
        pNotInMemoryInfo->nAverageActualReadBandwidth = (uint32)(pNotInMemoryInfo->nTotalBytesRead / fTotalReadTime);
    }

    // in memory reading
    if (pInMemoryInfo)
    {
        pInMemoryInfo->nBytesRead = pIOThread->m_InMemoryStats.m_nReadBytesInLastSecond;
        pInMemoryInfo->nRequestCount = pIOThread->m_InMemoryStats.m_nRequestCountInLastSecond;
        pInMemoryInfo->nTotalBytesRead = pIOThread->m_InMemoryStats.m_nTotalReadBytes;
        pInMemoryInfo->nTotalRequestCount = pIOThread->m_InMemoryStats.m_nTotalRequestCount;
    }
}
#endif

void CStreamEngine::Update(uint32 nUpdateTypesBitmask)
{
    FUNCTION_PROFILER_LEGACYONLY(GetISystem(), PROFILE_SYSTEM);
    AZ_TRACE_METHOD();
    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    // Dispatch completed callbacks.
    MainThread_FinalizeIOJobs(nUpdateTypesBitmask);
}

// Gets called regularly, to finalize those proxies whose jobs have
// already been executed (e.g. to call the callbacks)
//  - to be called from the main thread only
//  - starts new jobs in the single-threaded model
void CStreamEngine::Update()
{
    FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);
    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    // Dispatch completed callbacks.
    MainThread_FinalizeIOJobs();

#ifdef STREAMENGINE_ENABLE_STATS
    if (g_cvars.sys_streaming_resetstats)
    {
        ClearStatistics();
        g_cvars.sys_streaming_resetstats = 0;
    }

    CTimeValue t = gEnv->pTimer->GetAsyncTime();
    if ((t - m_nLastBandwidthUpdateTime).GetMilliSecondsAsInt64() > 1000)
    {
        // Repeat every second.
        m_nUnzipBandwidth = m_decompressStats.m_tempUnzipTime.GetValue() == 0 ? 0 : (uint32)(m_decompressStats.m_nTempBytesUnziped / m_decompressStats.m_tempUnzipTime.GetSeconds());
        m_nVerifyBandwidth = m_decompressStats.m_tempVerifyTime.GetValue() == 0 ? 0 : (uint32)(m_decompressStats.m_nTempBytesVerified / m_decompressStats.m_tempVerifyTime.GetSeconds());

        m_decompressStats.m_tempUnzipTime.SetValue(0);
        m_decompressStats.m_nTempBytesUnziped = 0;
        m_decompressStats.m_tempVerifyTime.SetValue(0);
        m_decompressStats.m_nTempBytesVerified = 0;

        m_nLastBandwidthUpdateTime = t;
    }
    if (m_decompressStats.m_totalUnzipTime.GetValue() != 0)
    {
        m_nUnzipBandwidthAverage = (uint32)(m_decompressStats.m_nTotalBytesUnziped / m_decompressStats.m_totalUnzipTime.GetSeconds());
    }
    if (m_decompressStats.m_totalVerifyTime.GetValue() != 0)
    {
        m_nVerifyBandwidthAverage = (uint32)(m_decompressStats.m_nTotalBytesVerified / m_decompressStats.m_totalVerifyTime.GetSeconds());
    }

    m_Statistics.nDecompressBandwidth = m_nUnzipBandwidth;
    m_Statistics.nVerifyBandwidth = m_nVerifyBandwidth;
    m_Statistics.nDecompressBandwidthAverage = m_nUnzipBandwidthAverage;
    m_Statistics.nVerifyBandwidthAverage = m_nVerifyBandwidthAverage;

    CTimeValue currentTime = gEnv->pTimer->GetAsyncTime();

    CTimeValue timeSinceLastReset = currentTime - m_TimeOfLastReset;
    float fSecSinceLastReset = timeSinceLastReset.GetSeconds();

    CTimeValue timeSinceLastUpdate = currentTime - m_TimeOfLastUpdate;

    // update the stats every second
    if (timeSinceLastUpdate.GetMilliSecondsAsInt64() > 1000)
    {
        UpdateIOThreadStats(&m_Statistics.hddInfo, &m_Statistics.memoryInfo, m_pThreadIO[eIOThread_HDD], fSecSinceLastReset);
        UpdateIOThreadStats(&m_Statistics.discInfo, 0, m_pThreadIO[eIOThread_Optical], fSecSinceLastReset);
        UpdateIOThreadStats(&m_Statistics.memoryInfo, 0, m_pThreadIO[eIOThread_InMemory], fSecSinceLastReset);

        SStreamEngineStatistics::SRequestTypeInfo totals;

        // update stats on all types
        for (int i = 0; i < eStreamTaskTypeCount; i++)
        {
            SStreamEngineStatistics::SRequestTypeInfo& info = m_Statistics.typeInfo[i];

            if (info.nTotalStreamingRequestCount)
            {
                info.fAverageCompletionTime = info.fTotalCompletionTime / info.nTotalStreamingRequestCount;
            }
            else
            {
                info.fAverageCompletionTime = 0;
            }
            info.nSessionReadBandwidth = (uint32)(info.nTotalReadBytes / fSecSinceLastReset);
            info.nCurrentReadBandwidth = (uint32)(info.nTmpReadBytes / timeSinceLastUpdate.GetSeconds());

            info.fAverageRequestCount = info.nTotalStreamingRequestCount / fSecSinceLastReset;

            totals.Merge(info);

            info.nTmpReadBytes = 0;
        }

        if (totals.nTotalStreamingRequestCount > 0)
        {
            m_Statistics.fAverageCompletionTime = totals.fTotalCompletionTime / totals.nTotalStreamingRequestCount;
        }

        m_Statistics.nTotalSessionReadBandwidth = (uint32)(totals.nTotalReadBytes / fSecSinceLastReset);
        m_Statistics.nTotalCurrentReadBandwidth = (uint32)(totals.nTmpReadBytes / timeSinceLastUpdate.GetSeconds());
        m_Statistics.fAverageRequestCount = totals.nTotalStreamingRequestCount / fSecSinceLastReset;

        m_Statistics.nTotalRequestCount = totals.nTotalRequestCount;
        m_Statistics.nTotalStreamingRequestCount = totals.nTotalStreamingRequestCount;
        m_Statistics.nTotalBytesRead = totals.nTotalReadBytes;

        // update this flag only once a second to be sure it's visible in display info
        m_Statistics.bTempMemOutOfBudget = m_bTempMemOutOfBudget;
        m_bTempMemOutOfBudget = false;

        m_TimeOfLastUpdate = currentTime;
    }

    int nTmpAllocated = m_tempMem.m_nTempAllocatedMemoryFrameMax;
    m_Statistics.nMaxTempMemory = max(m_Statistics.nMaxTempMemory, nTmpAllocated);
    m_Statistics.nTempMemory = nTmpAllocated;

    m_tempMem.m_nTempAllocatedMemoryFrameMax = m_tempMem.m_nTempAllocatedMemory;

    if (m_Statistics.vecHeavyAssets.size() > MAX_HEAVY_ASSETS)
    {
        AZStd::sort(m_Statistics.vecHeavyAssets.begin(), m_Statistics.vecHeavyAssets.end());
        m_Statistics.vecHeavyAssets.resize(MAX_HEAVY_ASSETS);
    }

    if (g_cvars.sys_streaming_debug)
    {
        DrawStatistics();

        if (!m_bInputCallback)
        {
            AzFramework::InputChannelEventListener::Connect();
            m_bInputCallback = true;
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
// Only waits at most the specified amount of time for some IO to complete
void CStreamEngine::UpdateAndWait(bool bAbortAll)
{
    // for stream->Wait sync
    LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

    if (bAbortAll)
    {
        for (int i = 0; i < eIOThread_Last; i++)
        {
            if (m_pThreadIO[i])
            {
                m_pThreadIO[i]->AbortAll(bAbortAll);
            }
        }
    }

    while (!m_finishedStreams.empty() || !m_streams.empty())
    {
        Update();
        // In case we still have cancelled or aborted streams in the queue,
        // we wake the io threads here to ensure they are removed correctly;
        for (uint32 i = 0; i < (uint32)eIOThread_Last; ++i)
        {
            SignalToStartWork((EIOThread)i, true);
        }
        CrySleep(10);
    }

    if (bAbortAll)
    {
        for (int i = 0; i < eIOThread_Last; i++)
        {
            if (m_pThreadIO[i])
            {
                m_pThreadIO[i]->AbortAll(false);
            }
        }
    }
}

// In the Multi-Threaded model (with the IO Worker thread)
// removes the proxies from the IO Queue as needed, and the proxies may call their callbacks
void CStreamEngine::MainThread_FinalizeIOJobs(uint32 type)
{
    static bool bNoReentrant = false;

    if (!bNoReentrant)
    {
        bNoReentrant = true;

        FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);
        LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

#ifdef STREAMENGINE_ENABLE_STATS
        m_Statistics.nMainStreamingThreadWait = CryGetTicks();
#endif

        int nCount = 0;

        CryMT::vector<CReadStream_AutoPtr> finishedStreams;
        // Dispatch completed callbacks.
        CReadStream_AutoPtr pStream(0);
        while (m_finishedStreams.try_pop_front(pStream))
        {
            if (pStream->m_Type & type)
            {
                pStream->MainThread_Finalize();

#ifdef STREAMENGINE_ENABLE_STATS
                // update statistics
                CryInterlockedDecrement(&m_Statistics.nCurrentFinishedCount);
                UpdateStatistics(pStream);
#endif

                m_streams.erase(pStream);

                nCount++;
                // perform time slicing if requested
                if (g_cvars.sys_streaming_max_finalize_per_frame > 0 &&
                    nCount > g_cvars.sys_streaming_max_finalize_per_frame)
                {
                    break;
                }
            }
            else
            {
                finishedStreams.push_back(pStream);
            }
        }

        bNoReentrant = false;

        while (finishedStreams.try_pop_front(pStream))
        {
            m_finishedStreams.push_back(pStream);
        }

#ifdef STREAMENGINE_ENABLE_STATS
        m_Statistics.nMainStreamingThreadWait = CryGetTicks() - m_Statistics.nMainStreamingThreadWait;
#endif
    }
}


// In the Multi-Threaded model (with the IO Worker thread)
// removes the proxies from the IO Queue as needed, and the proxies may call their callbacks
void CStreamEngine::MainThread_FinalizeIOJobs()
{
    static bool bNoReentrant = false;

    if (!bNoReentrant)
    {
        bNoReentrant = true;

        FUNCTION_PROFILER(GetISystem(), PROFILE_SYSTEM);
        LOADING_TIME_PROFILE_SECTION(gEnv->pSystem);

#ifdef STREAMENGINE_ENABLE_STATS
        m_Statistics.nMainStreamingThreadWait = CryGetTicks();
#endif

        int nCount = 0;

        //Optim: swap finished streams out into a non MT vector
        //avoid expensive push / pop operations.
        m_tempFinishedStreams.clear();
        m_finishedStreams.swap(m_tempFinishedStreams);

        int numFinishedStreams = m_tempFinishedStreams.size();

        // Dispatch completed callbacks.
        for (int i = 0; i < numFinishedStreams; i++)
        {
            CReadStream_AutoPtr pStream = m_tempFinishedStreams[i];

            //Check for a certain type of error that we need to handle in a TRC compliant way
            if (pStream->GetError() == ERROR_VERIFICATION_FAIL)
            {
#if !defined(_RELEASE)
                CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_COMMENT, "Stream error detected.");
#endif //!_RELEASE
            }

            pStream->MainThread_Finalize();

#ifdef STREAMENGINE_ENABLE_STATS
            // update statistics
            CryInterlockedDecrement(&m_Statistics.nCurrentFinishedCount);
            UpdateStatistics(pStream);
#endif

            m_streams.erase(pStream);

            nCount++;

            // AM: Optim, no longer support this behavior
            // perform time slicing if requested
            if (g_cvars.sys_streaming_max_finalize_per_frame > 0 &&
                nCount > g_cvars.sys_streaming_max_finalize_per_frame)
            {
                CryLogAlways("sys_streaming_max_finalize_per_frame is now deprecated");
                //break;
            }
        }

        m_tempFinishedStreams.clear();

        bNoReentrant = false;

#ifdef STREAMENGINE_ENABLE_STATS
        m_Statistics.nMainStreamingThreadWait = CryGetTicks() - m_Statistics.nMainStreamingThreadWait;
#endif
    }
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::UpdateJobPriority(IReadStreamPtr pJobStream)
{
    for (int i = 0; i < eIOThread_Last; i++)
    {
        if (m_pThreadIO[i])
        {
            m_pThreadIO[i]->NeedSorting();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::StopThreads()
{
    for (int i = 0; i < eIOThread_Last; i++)
    {
        m_pThreadIO[i] = 0;
    }

    m_asyncCallbackThreads.clear();
    m_tempMem.m_nWakeEvents = 0;
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::StartThreads()
{
    StopThreads();

    m_tempMem.m_nWakeEvents = 0;

    m_pThreadIO[eIOThread_HDD] = new CStreamingIOThread(this, eStreamSourceTypeHDD, "Streaming File IO HDD");//, 160);
    m_tempMem.m_wakeEvents[m_tempMem.m_nWakeEvents++] = &m_pThreadIO[eIOThread_HDD]->GetWakeEvent();

    if (!(gEnv->IsDedicated()))
    {
        if (m_bUseOpticalDriveThread)
        {
            m_pThreadIO[eIOThread_Optical] = new CStreamingIOThread(this, eStreamSourceTypeDisc, "Streaming File IO Optical");
            m_tempMem.m_wakeEvents[m_tempMem.m_nWakeEvents++] = &m_pThreadIO[eIOThread_Optical]->GetWakeEvent();
        }

        m_pThreadIO[eIOThread_InMemory] = new CStreamingIOThread(this, eStreamSourceTypeMemory, "Streaming File IO InMemory");
        m_tempMem.m_wakeEvents[m_tempMem.m_nWakeEvents++] = &m_pThreadIO[eIOThread_InMemory]->GetWakeEvent();
    }

    // Initialise fallback thread matrix, needed for rescheduling
    for (int i = 0; i < eIOThread_Last; ++i)
    {
        if (!m_pThreadIO[i])
        {
            continue;
        }

        for (int j = 0; j < eIOThread_Last; ++j)
        {
            if (i == j)
            {
                continue;
            }
            if (!m_pThreadIO[j])
            {
                continue;
            }

            m_pThreadIO[i]->RegisterFallbackIOThread(m_pThreadIO[j]->GetMediaType(), m_pThreadIO[j]);
        }
    }

    // More decompress threads can be added here.
    m_asyncCallbackQueues.push_back(new SStreamRequestQueue);
    m_asyncCallbackThreads.push_back(new CStreamingWorkerThread(this, "Streaming AsyncCallback", CStreamingWorkerThread::eWorkerAsyncCallback, m_asyncCallbackQueues.back()));

    //m_asyncCallbackThreads.push_back( new CStreamingWorkerThread(this,"Streaming AsyncCallback Pak 1",CStreamingWorkerThread::eWorkerAsyncCallback, m_asyncCallbackQueues[eStreamTaskTypePak]) );
}

//! Puts the memory statistics into the given sizer object
//! According to the specifications in interface ICrySizer
void CStreamEngine::GetMemoryStatistics(ICrySizer* pSizer)
{
    SIZER_COMPONENT_NAME(pSizer, "CRefStreamEngine");

    size_t nSize = sizeof(*this);

    pSizer->AddObject(this, nSize);
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::AbortJob(CReadStream* pStream)
{
    if (m_finishedStreams.try_remove((CReadStream*)pStream))
    {
#ifdef STREAMENGINE_ENABLE_STATS
        CryInterlockedDecrement(&m_Statistics.nCurrentFinishedCount);
#endif
    }

    {
        CryAutoLock<CryCriticalSection> pausedLock(m_pausedLock);
        if (!m_pausedStreams.empty())
        {
            std::vector<CReadStream_AutoPtr>::iterator it = std::find(m_pausedStreams.begin(), m_pausedStreams.end(), pStream);
            if (it != m_pausedStreams.end())
            {
                m_pausedStreams.erase(it);
            }
        }
    }

    m_streams.erase(pStream);
}

#if defined(STREAMENGINE_ENABLE_STATS)
SStreamEngineStatistics& CStreamEngine::GetStreamingStatistics()
{
    return m_Statistics;
}
#endif

#ifdef STREAMENGINE_ENABLE_STATS
void CStreamEngine::UpdateStatistics(CReadStream* pReadStream)
{
    uint32 nBytesRead = pReadStream->m_nBytesRead;

    SStreamEngineStatistics::SRequestTypeInfo& info = m_Statistics.typeInfo[pReadStream->m_Type];
    info.nTotalRequestCount++;

    // only add to stats if request was valid
    const string& name = pReadStream->GetName();
    if (name.length() > 0)
    {
        info.nTotalReadBytes += nBytesRead;
        info.nTmpReadBytes += nBytesRead;
        info.nTotalRequestDataSize += pReadStream->m_Params.nSize;

        CTimeValue completionTime = gEnv->pTimer->GetAsyncTime() - pReadStream->GetRequestTime();
        float fCompletionTime = completionTime.GetMilliSeconds();
        info.fTotalCompletionTime += fCompletionTime;

        size_t splitter = name.find_last_of(".");
        if (splitter != string::npos)
        {
            string extension = name.substr(splitter + 1);
            TExtensionInfoMap::iterator findRes = m_PerExtensionInfo.find(extension);
            if (findRes == m_PerExtensionInfo.end())
            {
                m_PerExtensionInfo[extension] = SExtensionInfo();
                findRes = m_PerExtensionInfo.find(extension);
            }

            SExtensionInfo& extensionInfo = findRes->second;
            extensionInfo.m_fTotalReadTime += pReadStream->m_ReadTime.GetMilliSeconds();
            extensionInfo.m_nTotalRequests++;
            extensionInfo.m_nTotalReadSize += nBytesRead;
            extensionInfo.m_nTotalRequestSize += pReadStream->m_Params.nSize;
        }
    }

    if (nBytesRead > 64 * 1024)
    {
        m_Statistics.vecHeavyAssets.push_back(SStreamEngineStatistics::SAsset(pReadStream->m_strFileName, nBytesRead));
    }
}
#endif

void CStreamEngine::Shutdown()
{
    m_bShutDown = true;

    // make sure we don't have queued paused streams during shutdown for the audio system
    // or we can suffer from deadlocks
    uint32 nPauseMask = GetPauseMask();
    uint32 nUnPauseMask = ~(nPauseMask & ~STREAM_TASK_TYPE_AUDIO_ALL);
    PauseStreaming(false, nUnPauseMask);
    PauseStreaming(true, nPauseMask);

    UpdateAndWait(true);
    CancelAll();

    StopThreads();

    m_streams.clear();
    m_finishedStreams.clear();

    // unregister system listener
    GetISystem()->GetISystemEventDispatcher()->RemoveListener(this);
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::CancelAll()
{
    for (int i = 0; i < eIOThread_Last; i++)
    {
        if (m_pThreadIO[i])
        {
            m_pThreadIO[i]->BeginReset();
        }
    }

    for (int i = 0; i < eIOThread_Last; i++)
    {
        if (m_pThreadIO[i])
        {
            m_pThreadIO[i]->EndReset();
        }
    }

    for (size_t i = 0; i < m_asyncCallbackThreads.size(); ++i)
    {
        m_asyncCallbackThreads[i]->BeginReset();
    }
    for (size_t i = 0; i < m_asyncCallbackThreads.size(); ++i)
    {
        m_asyncCallbackThreads[i]->EndReset();
    }

    // make sure we don't check for canceled tasks when destroying the m_finishedStreams container
    m_streams.clear();
    stl::free_container(m_finishedStreams);
    stl::free_container(m_tempFinishedStreams);
    {
        CryAutoLock<CryCriticalSection> lock(m_pausedLock);

        std::vector<CReadStream_AutoPtr> paused;
        paused.swap(m_pausedStreams);

        for (std::vector<CReadStream_AutoPtr>::iterator it = paused.begin(), itEnd = paused.end(); it != itEnd; ++it)
        {
            CReadStream* pStream = &**it;
            pStream->AbortShutdown();
        }
    }

    CReadStream::Flush();
    CAsyncIOFileRequest::Flush();
}


//////////////////////////////////////////////////////////////////////////
void CStreamEngine::ReportAsyncFileRequestComplete(CAsyncIOFileRequest_AutoPtr pFileRequest)
{
    if (!pFileRequest->IsCancelled())
    {
#ifdef STREAMENGINE_ENABLE_LISTENER
        if (m_pListener)
        {
            m_pListener->OnStreamBeginAsyncCallback(&*pFileRequest);
        }
#endif

        if (pFileRequest->m_pCallback)
        {
            pFileRequest->m_pCallback->OnAsyncFinished(pFileRequest);
        }
        if (pFileRequest->m_pReadStream)
        {
            CReadStream_AutoPtr pStream = (CReadStream*)(IReadStream*)pFileRequest->m_pReadStream;
            pStream->OnAsyncFileRequestComplete();
            m_finishedStreams.push_back(pStream);

#ifdef STREAMENGINE_ENABLE_STATS
            CryInterlockedIncrement(&m_Statistics.nCurrentFinishedCount);
#endif
        }

#ifdef STREAMENGINE_ENABLE_LISTENER
        if (m_pListener)
        {
            m_pListener->OnStreamEndAsyncCallback(&*pFileRequest);
        }
#endif

#ifdef STREAMENGINE_ENABLE_STATS
        if (g_cvars.sys_streaming_debug != 0)
        {
            if (g_cvars.sys_streaming_debug == 2 || g_cvars.sys_streaming_debug == 4)
            {
                const char* const sFileFilter = g_cvars.sys_streaming_debug_filter_file_name->GetString();

                if (!pFileRequest->m_strFileName.empty() && !m_bStreamingStatsPaused)
                {
                    if (!sFileFilter || !sFileFilter[0] || strstr(pFileRequest->m_strFileName.c_str(), sFileFilter))
                    {
                        CryAutoCriticalSection lock(m_csStats);
                        m_statsRequestList.insert(m_statsRequestList.begin(), pFileRequest);
                    }
                }
            }
        }
#endif
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CStreamEngine::GetStreamTaskTypeName(EStreamTaskType type)
{
    switch (type)
    {
    case eStreamTaskTypeMusic:
        return "Music";
    case eStreamTaskTypeAnimation:
        return "Animation";
    case eStreamTaskTypeGeometry:
        return "Geometry";
    case eStreamTaskTypeSound:
        return "Sound";
    case eStreamTaskTypeTexture:
        return "Texture";
    case eStreamTaskTypeShader:
        return "Shader";
    case eStreamTaskTypeTerrain:
        return "Terrain";
    case eStreamTaskTypeVideo:
        return "Video";
    case eStreamTaskTypeFlash:
        return "Flash";
    case eStreamTaskTypePak:
        return "Pak";
    case eStreamTaskTypeGeomCache:
        return "GeomCache";
    case eStreamTaskTypeMergedMesh:
        return "MergedMesh";
    }
    return "";
}

SStreamJobEngineState CStreamEngine::GetJobEngineState()
{
    m_tempMem.m_nTempMemoryBudget = g_cvars.sys_streaming_memory_budget * 1024;

    SStreamJobEngineState state;
    state.pReportQueues = &m_asyncCallbackQueues;
#ifdef STREAMENGINE_ENABLE_STATS
    state.pStats = &m_Statistics;
    state.pDecompressStats = &m_decompressStats;
#endif
    state.pHeap = g_pPakHeap;
    state.pTempMem = &m_tempMem;
    return state;
}

#ifdef STREAMENGINE_ENABLE_STATS
void CStreamEngine::GetBandwidthStats(EStreamTaskType type, float* bandwidth)
{
    *bandwidth = m_Statistics.typeInfo[type].nCurrentReadBandwidth / 1024.0f;
}
#endif

void CStreamEngine::GetStreamingOpenStatistics(SStreamEngineOpenStats& openStatsOut)
{
    openStatsOut = m_OpenStatistics;
}

#ifdef STREAMENGINE_ENABLE_LISTENER
void CStreamEngine::SetListener(IStreamEngineListener* pListener)
{
    m_pListener = pListener;
}
#endif

#ifdef STREAMENGINE_ENABLE_LISTENER
IStreamEngineListener* CStreamEngine::GetListener()
{
    return m_pListener;
}
#endif

//////////////////////////////////////////////////////////////////////////
void* CStreamEngine::TempAlloc(size_t nSize, const char* szDbgSource, bool bFallBackToMalloc, bool bUrgent, uint32 align)
{
    return m_tempMem.TempAlloc(g_pPakHeap, nSize, szDbgSource, bFallBackToMalloc, bUrgent, align);
}

void CStreamEngine::TempFree(void* p, size_t nSize)
{
    m_tempMem.TempFree(g_pPakHeap, p, nSize);
}

namespace
{
#ifdef STREAMENGINE_ENABLE_STATS
    void DrawText(const float, const float, ColorF, const char*, ...)
    {
        // ToDo: Remove whole file with SPEC-343, or update to draw with Atom? Likely the former as I think this whole system is dead.
    }
#endif

    void WriteToStreamingLog([[maybe_unused]] const char* str)
    {
#ifdef STREAMENGINE_ENABLE_STATS
        if (g_cvars.sys_streaming_debug == 4)
        {
            // ignore invalid file access when logging steaming data
            CDebugAllowFileAccess ignoreInvalidFileAccess;

            static string sFileName;
            static bool bFirstTime = true;
            if (bFirstTime)
            {
                char path[AZ::IO::IArchive::MaxPath];
                path[sizeof(path) - 1] = 0;
                gEnv->pCryPak->AdjustFileName("@usercache@\\TestResults\\StreamingLog.txt", path, AZ_ARRAY_SIZE(path), AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FLAGS_FOR_WRITING);
                sFileName = path;
            }
            AZ::IO::HandleType fileHandle = fxopen(sFileName, (bFirstTime) ? "wt" : "at");
            bFirstTime = false;
            if (fileHandle != AZ::IO::InvalidHandle)
            {
                AZ::IO::Print(fileHandle, "%s\n", str);
                gEnv->pFileIO->Close(fileHandle);
            }
        }
#endif
    }
}

#ifdef STREAMENGINE_ENABLE_STATS
//////////////////////////////////////////////////////////////////////////
void CStreamEngine::DrawStatistics()
{
    std::vector<CAsyncIOFileRequest_AutoPtr> tempRequests;

    if (g_cvars.sys_streaming_debug == 4)
    {
        float tx = 0;
        float ty = 30;
        float ystep = 12.0f;
        ColorF clText(1, 0, 0, 1);

        DrawText(tx, ty += ystep, clText, "Recording streaming stats to file ...");

        {
            CryAutoCriticalSection lock(m_csStats);
            tempRequests.swap(m_statsRequestList);
        }

        const char* const sFileFilter = g_cvars.sys_streaming_debug_filter_file_name->GetString();

        if (!tempRequests.empty())
        {
            for (int i = (int)tempRequests.size() - 1; i >= 0; i--)
            {
                CAsyncIOFileRequest* pFileRequest = tempRequests[i];

                if (g_cvars.sys_streaming_debug_filter > 0 && pFileRequest->m_eType != g_cvars.sys_streaming_debug_filter)
                {
                    continue;
                }
                if (g_cvars.sys_streaming_debug_filter == -1 && pFileRequest->m_eMediaType == eStreamSourceTypeMemory)
                {
                    continue;
                }
                if (g_cvars.sys_streaming_debug_filter_min_time && (pFileRequest->m_readTime.GetMilliSeconds() < (float)g_cvars.sys_streaming_debug_filter_min_time))
                {
                    continue;
                }
                if (sFileFilter && sFileFilter[0] && !strstr(pFileRequest->m_strFileName.c_str(), sFileFilter))
                {
                    continue;
                }

                const char* sFlags = (pFileRequest->m_eMediaType == eStreamSourceTypeHDD) ? "HDD" : ((pFileRequest->m_eMediaType == eStreamSourceTypeMemory) ? "mem" : "DVD");
                const char* sPriority = "";
                switch (pFileRequest->m_ePriority)
                {
                case estpUrgent:
                    sPriority = "     Urgent";
                    break;
                case estpNormal:
                    sPriority = "     Normal";
                    break;
                case estpIdle:
                    sPriority = "       Idle";
                    break;
                case estpPreempted:
                    sPriority = "  Preempted";
                    break;
                case estpBelowNormal:
                    sPriority = "BelowNormal";
                    break;
                case estpAboveNormal:
                    sPriority = "AboveNormal";
                    break;
                default:
                    sPriority = "    Unknown";
                    break;
                }

                string str;
                str.Format("[N%6d] [%+8d] [%8d] [%6.2f ms] (%5d|%5d) [%5.3fs] <%3d> <%s> <%s> <%s> %s:",
                    pFileRequest->m_nReadCounter,
                    pFileRequest->m_nReadHeadOffsetKB,
                    pFileRequest->m_nDiskOffset >> 10,
                    pFileRequest->m_readTime.GetMilliSeconds(),
                    pFileRequest->m_nSizeOnMedia / 1024,
                    ((pFileRequest->m_nRequestedSize) ? pFileRequest->m_nRequestedSize : pFileRequest->m_nFileSize) / 1024,
                    (pFileRequest->m_completionTime - pFileRequest->m_startTime).GetSeconds(),
                    pFileRequest->m_nTimeGroup,
                    sPriority,
                    sFlags,
                    pFileRequest->m_pakFile.c_str(),
                    pFileRequest->m_strFileName.c_str());

                WriteToStreamingLog(str.c_str());
            }
        }

        return;
    }


    {
        CryAutoCriticalSection lock(m_csStats);
        tempRequests = m_statsRequestList;

        size_t nMaxRequests = g_cvars.sys_streaming_debug_filter_min_time ? 1000 : 100;
        if (m_statsRequestList.size() > nMaxRequests)
        {
            m_statsRequestList.resize(nMaxRequests);
        }
    }

    std::vector<CAsyncIOFileRequest_AutoPtr>& requests = tempRequests;

    stack_string temp;
    float tx = 0;
    float ty = 30;
    float ystep = 12.0f;
    float xColumn = 80;
    ColorF clText(0, 1, 1, 1);

    SStreamEngineStatistics& stats = m_Statistics;
    SStreamEngineOpenStats openStats = m_OpenStatistics;

    const char* sMediaType = m_bStreamDataOnHDD ? "HDD" : "DVD";
    const char* sStatus = (m_bStreamingStatsPaused) ? "Paused" : "";
    DrawText(tx, ty += ystep, clText, "Streaming IO: %.2f|%.2fMB/s, ACT: %3dmsec, Unzip: %.2fMB/s, Verify: %.2fMB/s, Jobs:%5d (%4d) %s %s",
        (float)stats.nTotalCurrentReadBandwidth / (1024 * 1024), (float)stats.nTotalSessionReadBandwidth / (1024 * 1024),
        (uint32)stats.fAverageCompletionTime, (float)stats.nDecompressBandwidth / (1024 * 1024), (float)stats.nVerifyBandwidth / (1024 * 1024),
        (uint32)stats.nTotalStreamingRequestCount, (uint32)(stats.nTotalRequestCount - stats.nTotalStreamingRequestCount),
        sMediaType, sStatus);

    DrawText(tx, ty += ystep, clText, "\t Request: Active:%2d (%2.1fMB) Live:%2d Decompress:%2d Async:%2d Finished:%2d Temp Pool Max:%2.1fMB", openStats.nOpenRequestCount,
        (float)stats.nPendingReadBytes / (1024 * 1024), CAsyncIOFileRequest::s_nLiveRequests, stats.nCurrentDecompressCount, stats.nCurrentAsyncCount, stats.nCurrentFinishedCount,
        (float)stats.nMaxTempMemory / (1024 * 1024));

    ty += ystep;

    // HDD stats
    if (stats.hddInfo.nTotalRequestCount > 0)
    {
        DrawText(tx, ty += ystep, clText, "HDD : Request: %3d|%5d (%4d MB|%3d KB) - BW: %1.2f|%1.2f Mb/s (Eff: %2.1f|%2.1f Mb/s) \n",
            stats.hddInfo.nRequestCount, stats.hddInfo.nTotalRequestCount, (uint32)(stats.hddInfo.nTotalBytesRead / (1024 * 1024)),
            (uint32)(stats.hddInfo.nTotalBytesRead / (1024 * stats.hddInfo.nTotalRequestCount)),
            (float)stats.hddInfo.nCurrentReadBandwidth / (1024 * 1024), (float)stats.hddInfo.nSessionReadBandwidth / (1024 * 1024),
            (float)stats.hddInfo.nActualReadBandwidth / (1024 * 1024), (float)stats.hddInfo.nAverageActualReadBandwidth / (1024 * 1024));
        DrawText(tx, ty += ystep, clText, "\t  Seek: %1.2f GB - Active: %2.1f%%(%2.1f%%)",
            (float)stats.hddInfo.nAverageSeekOffset / (1024 * 1024),
            stats.hddInfo.fActiveDuringLastSecond, stats.hddInfo.fAverageActiveTime);
    }
    // Optical stats
    if (stats.discInfo.nTotalRequestCount > 0)
    {
        DrawText(tx, ty += ystep, clText, "Disc: Request: %3d|%5d (%4d MB|%3d KB) - BW: %1.2f|%1.2f Mb/s (Eff: %2.1f|%2.1f Mb/s) \n",
            stats.discInfo.nRequestCount, stats.discInfo.nTotalRequestCount, (uint32)(stats.discInfo.nTotalBytesRead / (1024 * 1024)),
            (uint32)(stats.discInfo.nTotalBytesRead / (1024 * stats.discInfo.nTotalRequestCount)),
            (float)stats.discInfo.nCurrentReadBandwidth / (1024 * 1024), (float)stats.discInfo.nSessionReadBandwidth / (1024 * 1024),
            (float)stats.discInfo.nActualReadBandwidth / (1024 * 1024), (float)stats.discInfo.nAverageActualReadBandwidth / (1024 * 1024));
        DrawText(tx, ty += ystep, clText, "\t  Seek: %1.2f GB - Active: %2.1f%%(%2.1f%%)",
            (float)stats.discInfo.nAverageSeekOffset / (1024 * 1024),
            stats.discInfo.fActiveDuringLastSecond, stats.discInfo.fAverageActiveTime);
    }
    DrawText(tx, ty += ystep, clText, "Mem : Request: %3d|%5d (%4d MB)",
        stats.memoryInfo.nRequestCount, stats.memoryInfo.nTotalRequestCount, (stats.memoryInfo.nTotalBytesRead / (1024 * 1024)));

    ty += ystep;

    for (int i = eStreamTaskTypeCount - 1; i >= 1; i--)
    {
        EStreamTaskType eTaskType = (EStreamTaskType)i;
        SStreamEngineStatistics::SRequestTypeInfo info = stats.typeInfo[eTaskType];

        if (g_cvars.sys_streaming_debug > 1 || info.nTotalRequestCount > 0)
        {
            DrawText(tx, ty += ystep, clText, "%9s: BSize:%3dKb Read:%4dMb BW:%1.2f|%1.2f Mb/s ACT:%5dms %2d(%2.1fMB)|%5d",
                gEnv->pSystem->GetStreamEngine()->GetStreamTaskTypeName(eTaskType),
                (uint32)(info.nTotalReadBytes / max((uint32)1, info.nTotalStreamingRequestCount) / 1024),
                (uint32)(info.nTotalReadBytes / (1024 * 1024)), (float)info.nCurrentReadBandwidth / (1024 * 1024),
                (float)info.nSessionReadBandwidth / (1024 * 1024), (uint32)info.fAverageCompletionTime,
                openStats.nOpenRequestCountByType[eTaskType], (float)info.nPendingReadBytes / (1024 * 1024), (uint32)info.nTotalStreamingRequestCount);
        }
    }

    if (g_cvars.sys_streaming_debug == 5)
    {
        ty += ystep;
        ty += ystep;

        DrawText(tx, ty += ystep, clText, "Name | Time(s) | Size(Kb) | Read(Mb) | ReqS(Mb) | Count");

        for (TExtensionInfoMap::iterator it = m_PerExtensionInfo.begin(); it != m_PerExtensionInfo.end(); ++it)
        {
            SExtensionInfo& extensionInfo = it->second;
            DrawText(tx, ty += ystep, clText, "%4s | %7.3f | %8d | %8.3f | %8.3f | %5d",
                it->first.c_str(), extensionInfo.m_fTotalReadTime / 1000, (uint32)(extensionInfo.m_nTotalReadSize / max((size_t)1, extensionInfo.m_nTotalRequests) / 1024),
                extensionInfo.m_nTotalReadSize / (1024.0f * 1024.0f), extensionInfo.m_nTotalRequestSize / (1024.0f * 1024.0f), extensionInfo.m_nTotalRequests);
        }
    }
    else if (g_cvars.sys_streaming_debug > 1)
    {
        ty += ystep;

        DrawText(tx, ty += ystep, clText, "[Offset KB]");
        DrawText(tx + xColumn, ty, clText, "[io  ms]\t(read | size) [t sec] [Grp]   <   Priority> <Disk>   Filename");

        ty += ystep;

        const char* const sFileFilter = g_cvars.sys_streaming_debug_filter_file_name->GetString();

        for (size_t i = 0, nRequests = requests.size(); i < nRequests; i++)
        {
            CAsyncIOFileRequest* pFileRequest = requests[i];

            if (g_cvars.sys_streaming_debug_filter > 0 && pFileRequest->m_eType != g_cvars.sys_streaming_debug_filter)
            {
                continue;
            }
            if (g_cvars.sys_streaming_debug_filter == -1 && pFileRequest->m_eMediaType == eStreamSourceTypeMemory)
            {
                continue;
            }
            if (g_cvars.sys_streaming_debug_filter_min_time && (pFileRequest->m_readTime.GetMilliSeconds() < (float)g_cvars.sys_streaming_debug_filter_min_time))
            {
                continue;
            }
            if (sFileFilter != 0 && sFileFilter[0] && !strstr(pFileRequest->m_strFileName.c_str(), sFileFilter))
            {
                continue;
            }

            {
                float fMillis = pFileRequest->m_readTime.GetMilliSeconds();
                const char* sFlags = "";
                switch (pFileRequest->m_eMediaType)
                {
                case eStreamSourceTypeHDD:
                    sFlags = "HDD";
                    break;
                case eStreamSourceTypeDisc:
                    sFlags = "DVD";
                    break;
                case eStreamSourceTypeMemory:
                    sFlags = "MEM";
                    break;
                }
                const char* sPriority = "";
                switch (pFileRequest->m_ePriority)
                {
                case estpUrgent:
                    sPriority = "     Urgent";
                    break;
                case estpNormal:
                    sPriority = "     Normal";
                    break;
                case estpIdle:
                    sPriority = "       Idle";
                    break;
                case estpPreempted:
                    sPriority = "  Preempted";
                    break;
                case estpBelowNormal:
                    sPriority = "BelowNormal";
                    break;
                case estpAboveNormal:
                    sPriority = "AboveNormal";
                    break;
                default:
                    sPriority = "    Unknown";
                    break;
                }
                uint32 nRequestedSize = (pFileRequest->m_nRequestedSize != 0) ? pFileRequest->m_nRequestedSize : pFileRequest->m_nFileSize;

                //////////////////////////////////////////////////////////////////////////
                ColorF colOffset;
                if (pFileRequest->m_nReadHeadOffsetKB >= 0)
                {
                    colOffset = ColorF (0, 1, 0, 1); // Green
                    if (pFileRequest->m_nReadHeadOffsetKB > 32)
                    {
                        colOffset = ColorF (0.5f, 1.f, 0, 1.f); // Cyan
                    }
                }
                else
                {
                    colOffset = ColorF (1, 0, 0, 1); // Red
                }
                if (pFileRequest->m_eMediaType != eStreamSourceTypeMemory)
                {
                    DrawText(tx, ty, colOffset, "[%+d]", pFileRequest->m_nReadHeadOffsetKB);
                }
                //////////////////////////////////////////////////////////////////////////

                DrawText(tx + xColumn, ty, clText, "[%6.2f]\t(%5d|%5d) [%5.2f] [%3d] <%s> <%s>\t%s",
                    fMillis, pFileRequest->m_nSizeOnMedia / 1024, nRequestedSize / 1024, (pFileRequest->m_completionTime - pFileRequest->m_startTime).GetSeconds(),
                    pFileRequest->m_nTimeGroup, sPriority, sFlags, pFileRequest->m_strFileName.c_str());

                ty += ystep;
            }
        }
    }
}
#endif //STREAMENGINE_ENABLE_STATS

#ifdef STREAMENGINE_ENABLE_STATS
void CStreamEngine::ClearStatistics()
{
    m_TimeOfLastReset = gEnv->pTimer->GetAsyncTime();
    m_TimeOfLastUpdate = m_TimeOfLastReset;

    m_Statistics.hddInfo.ResetStats();
    m_Statistics.discInfo.ResetStats();

    m_PerExtensionInfo.clear();

    m_Statistics.nDecompressBandwidth = 0;
    m_Statistics.nVerifyBandwidth = 0;
    m_Statistics.nDecompressBandwidthAverage = 0;
    m_Statistics.nVerifyBandwidthAverage = 0;

    m_Statistics.nTotalBytesRead = 0;
    m_Statistics.nTotalRequestCount = 0;
    m_Statistics.nTotalStreamingRequestCount = 0;

    m_Statistics.nMaxTempMemory = 0;

    m_Statistics.fAverageCompletionTime = 0;

    for (int i = 0; i < eStreamTaskTypeCount; i++)
    {
        m_Statistics.typeInfo[i].ResetStats();
    }
    m_Statistics.vecHeavyAssets.clear();

    for (int i = 0; i < eIOThread_Last; i++)
    {
        if (m_pThreadIO[i])
        {
            m_pThreadIO[i]->m_InMemoryStats.Reset();
            m_pThreadIO[i]->m_NotInMemoryStats.Reset();
        }
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
bool CStreamEngine::OnInputChannelEventFiltered([[maybe_unused]] const AzFramework::InputChannel& inputChannel)
{
#ifdef STREAMENGINE_ENABLE_STATS
    if (g_cvars.sys_streaming_debug)
    {
        if (inputChannel.GetInputChannelId() == AzFramework::InputDeviceKeyboard::Key::Function11)
        {
            m_bStreamingStatsPaused = true;
        }
        if (inputChannel.GetInputChannelId() == AzFramework::InputDeviceKeyboard::Key::Function12)
        {
            m_bStreamingStatsPaused = false;
        }
    }
#endif
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::OnSystemEvent(ESystemEvent event, [[maybe_unused]] UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
    {
        // unpause the streaming engine, when init phase is done
        PauseStreaming(false, -1);
        break;
    }
    case ESYSTEM_EVENT_LEVEL_LOAD_PREPARE:
#if defined(STREAMENGINE_ENABLE_STATS)
        ClearStatistics();
#endif

        WriteToStreamingLog("*LEVEL_LOAD_PREPARE");
        break;

    case ESYSTEM_EVENT_LEVEL_LOAD_START:
    {
        WriteToStreamingLog("*LEVEL_LOAD_START");
        break;
    }
    case ESYSTEM_EVENT_LEVEL_LOAD_END:
    {
        WriteToStreamingLog("*LEVEL_LOAD_END");
        break;
    }
    case ESYSTEM_EVENT_LEVEL_PRECACHE_START:
    {
        WriteToStreamingLog("*LEVEL_LOAD_PRECACHE_START");
        break;
    }
    case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
    {
        WriteToStreamingLog("*LEVEL_LOAD_PRECACHE_END");
        break;
    }
    case ESYSTEM_EVENT_LEVEL_UNLOAD:
    {
        UpdateAndWait(true);
        CancelAll();

#if defined(STREAMENGINE_ENABLE_STATS)
        ClearStatistics();
#endif
        break;
    }
    case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
    {
        UpdateAndWait(true);
        CancelAll();

#if defined(STREAMENGINE_ENABLE_STATS)
        ClearStatistics();
#endif
    }
    break;
    case ESYSTEM_EVENT_FAST_SHUTDOWN:
    {
        Shutdown();
        break;
    }
    }
}

//////////////////////////////////////////////////////////////////////////
void CStreamEngine::PauseStreaming(bool bPause, uint32 nPauseTypesBitmask)
{
    CryAutoLock<CryCriticalSection> pausedLock(m_pausedLock);
    if (bPause)
    {
        m_nPausedDataTypesMask |= nPauseTypesBitmask;
    }
    else
    {
        m_nPausedDataTypesMask &= ~nPauseTypesBitmask;
        ResumePausedStreams_PauseLocked();
    }
}
//////////////////////////////////////////////////////////////////////////
void CStreamEngine::PauseIO(bool bPause)
{
    for (int i = 0; i < eIOThread_Last; i++)
    {
        if (m_pThreadIO[i])
        {
            m_pThreadIO[i]->Pause(bPause);
        }
    }
}
//////////////////////////////////////////////////////////////////////////
