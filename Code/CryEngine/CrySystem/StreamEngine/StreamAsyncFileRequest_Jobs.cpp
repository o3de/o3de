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

#include "CrySystem_precompiled.h"
#include <AzCore/Debug/Profiler.h>

#include <CryPath.h>
#include "StreamAsyncFileRequest.h"

#include "MTSafeAllocator.h"

namespace AZ::IO::ZipDir::ZipDirStructuresInternal
{
    extern void ZlibInflateElementPartial_Impl(
        int* pReturnCode, z_stream* pZStream, ZipDir::UncompressLookahead* pLookahead,
        uint8_t* pOutput, size_t nOutputLen, bool bOutputWriteOnly,
        const uint8_t* pInput, size_t nInputLen, size_t* pTotalOut);
}

#ifdef STREAMENGINE_ENABLE_LISTENER
#include "IStreamEngine.h"
class NotifyListener
{
public:
    NotifyListener(IStreamEngineListener* pL, CAsyncIOFileRequest* pReq)
        : m_pL(pL)
        , m_pReq(pReq)
        , m_bInProgress(false) {}
    virtual ~NotifyListener() {}
protected:
    IStreamEngineListener* m_pL;
    CAsyncIOFileRequest* m_pReq;
    bool m_bInProgress;
};
class NotifyListenerInflate
    : NotifyListener
{
public:
    NotifyListenerInflate(IStreamEngineListener* pL, CAsyncIOFileRequest* pReq)
        : NotifyListener(pL, pReq)
    {
        if (m_pL)
        {
            m_pL->OnStreamBeginInflate(m_pReq);
            m_bInProgress = true;
        }
    }
    ~NotifyListenerInflate()
    {
        End();
    }
    void End()
    {
        if (m_bInProgress)
        {
            m_pL->OnStreamEndInflate(m_pReq);
            m_bInProgress = false;
        }
    }
};
#endif

#if defined(STREAMENGINE_ENABLE_STATS)
#define STREAMENGINE_ENABLE_TIMING
#endif

//#define STREAM_DECOMPRESS_TRACE(...) OutputDebugString(AZStd::string::format(__VA_ARGS__).c_str());
#define STREAM_DECOMPRESS_TRACE(...)

void SStreamJobQueue::Flush(SStreamEngineTempMemStats& tms)
{
    extern CMTSafeHeap* g_pPakHeap;

    for (int c = m_nQueueLen, i = m_nPop % MaxJobs; c; --c, i = (i + 1) % MaxJobs)
    {
        Job& j = m_jobs[i];
        if (j.pSrcHdr && CryInterlockedDecrement(&j.pSrcHdr->nRefs) == 0)
        {
            tms.TempFree(g_pPakHeap, j.pSrc, j.pSrcHdr->nSize);
        }
        j.pSrc = NULL;
    }
}

int SStreamJobQueue::Push(void* pSrc, SStreamPageHdr* pSrcHdr, uint32 nOffs, uint32 nBytes, bool bLast)
{
    m_sema.Acquire();

    int nSlot = (m_nPush++) % MaxJobs;

    Job& j = m_jobs[nSlot];
    j.pSrc = pSrc;
    j.pSrcHdr = pSrcHdr;
    j.nOffs = nOffs;
    j.nBytes = nBytes;
    j.bLast = (uint32)bLast;

    bool bStartNext = CryInterlockedIncrement(&m_nQueueLen) == 1;
    return bStartNext ? nSlot : -1;
}

int SStreamJobQueue::Pop()
{
    int nSlot = (++m_nPop) % MaxJobs;
    bool bStartNext = CryInterlockedDecrement(&m_nQueueLen) > 0;

    m_sema.Release();

    return bStartNext ? nSlot : -1;
}

void CAsyncIOFileRequest::AddRef()
{
    int nRef = CryInterlockedIncrement(&m_nRefCount);
    STREAM_DECOMPRESS_TRACE("[StreamDecompress],AddRef,0x%x,%s,0x%p,%i\n", CryGetCurrentThreadId(), m_strFileName.c_str(), this, nRef);
}

int CAsyncIOFileRequest::Release()
{
    int nRef = CryInterlockedDecrement(&m_nRefCount);

    STREAM_DECOMPRESS_TRACE("[StreamDecompress],Release,0x%x,%s,0x%p,%i\n", CryGetCurrentThreadId(), m_strFileName.c_str(), this, nRef);

#ifndef _RELEASE
    if (nRef < 0)
    {
        __debugbreak();
    }
#endif

    if (nRef == 0)
    {
        Finalize();

        CryInterlockedPushEntrySList(s_freeRequests, m_nextFree);
    }

    return nRef;
}

void CAsyncIOFileRequest::DecompressBlockEntry(SStreamJobEngineState engineState, int nJob)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::System);

    STREAM_DECOMPRESS_TRACE("[StreamDecompress],DecompressBlockEntry,0x%x,%s,0x%p,%i\n", CryGetCurrentThreadId(), m_strFileName.c_str(), this, nJob);

    CAsyncIOFileRequest_TransferPtr pSelf(this);

    SStreamJobQueue::Job& job = m_pDecompQueue->m_jobs[nJob];

    void* pSrc = job.pSrc;
    SStreamPageHdr* const pSrcHdr = job.pSrcHdr;
    const uint32 nOffs = job.nOffs;
    const uint32 nBytes = job.nBytes;
    const bool bLast = job.bLast;
    const bool bFailed = HasFailed();

    if (!bFailed)
    {
#if defined(STREAMENGINE_ENABLE_TIMING)
        LARGE_INTEGER liStart;
        QueryPerformanceCounter(&liStart);
#endif

        //printf("Inflate: %s Avail in: %d, Avail Out: %d, Next In: 0x%p, Next Out: 0x%p\n", m_strFileName.c_str(), m_pZlibStream->avail_in, m_pZlibStream->avail_out, m_pZlibStream->next_in, m_pZlibStream->next_out);

#ifdef STREAMENGINE_ENABLE_LISTENER
        NotifyListenerInflate inflateListener(gEnv->pSystem->GetStreamEngine()->GetListener(), this);
#endif

        size_t nBytesDecomped = m_nBytesDecompressed;

        STREAM_DECOMPRESS_TRACE ("[StreamDecompress],ZlibInflateElementPartial_Impl,0x%x,%s,0x%p,%i,0x%p,%i,%i\n",
            CryGetCurrentThreadId(),
            m_strFileName.c_str(),
            (uint8_t*)m_pReadMemoryBuffer + nBytesDecomped,
            m_nFileSize - nBytesDecomped,
            (uint8_t*)pSrc + nOffs,
            nBytes,
            nBytesDecomped);

        int readStatus = Z_OK;

        {
            CryOptionalAutoLock<CryCriticalSection> decompLock(m_externalBufferLockDecompress, m_pExternalMemoryBuffer != NULL);

            AZ::IO::ZipDir::ZipDirStructuresInternal::ZlibInflateElementPartial_Impl(
                &readStatus,
                m_pZlibStream,
                m_pLookahead,
                (uint8_t*)m_pReadMemoryBuffer + nBytesDecomped,
                m_nFileSize - nBytesDecomped,
                m_bWriteOnlyExternal,
                (uint8_t*)pSrc + nOffs,
                nBytes,
                &nBytesDecomped
                );
        }

        m_nBytesDecompressed = nBytesDecomped;

        //inform listen, so aysnc callback does not overlap
#ifdef STREAMENGINE_ENABLE_LISTENER
        inflateListener.End();
#endif

        if (readStatus == Z_OK || readStatus == Z_STREAM_END)
        {
#if defined(STREAMENGINE_ENABLE_TIMING)
            LARGE_INTEGER liEnd, liFreq;
            QueryPerformanceCounter(&liEnd);
            QueryPerformanceFrequency(&liFreq);

            m_unzipTime += CTimeValue((int64)((liEnd.QuadPart - liStart.QuadPart) * CTimeValue::TIMEVALUE_PRECISION / liFreq.QuadPart));
#endif
        }
        else
        {
#ifndef _RELEASE
            AZ_Assert(false, "Decomp Error: %s : %s\n", m_strFileName.c_str(), m_pZlibStream ? m_pZlibStream->msg : "m_pZlibStream == NULL, no message available");
#endif
            Failed(ERROR_DECOMPRESSION_FAIL);
        }
    }

    if (pSrcHdr)
    {
        if (CryInterlockedDecrement(&pSrcHdr->nRefs) == 0)
        {
            engineState.pTempMem->TempFree(engineState.pHeap, pSrc, pSrcHdr->nSize);
        }
    }

    job.pSrc = NULL;

    int nPopSlot = m_pDecompQueue->Pop();

    // job is no longer valid

    if (HasFailed() || bLast)
    {
        JobFinalize_Decompress(pSelf, engineState);
    }
    else if (nPopSlot >= 0)
    {
        // Chain start the next job, we're responsible for it.
        STREAM_DECOMPRESS_TRACE("[StreamDecompress],Chaining,0x%x,%s,0x%p,%i\n", CryGetCurrentThreadId(), m_strFileName.c_str(), this, nPopSlot);
        JobStart_Decompress(pSelf, engineState, nPopSlot);
    }

#if defined(STREAMENGINE_ENABLE_STATS)
    CryInterlockedDecrement(&engineState.pStats->nCurrentDecompressCount);
#endif
}

//////////////////////////////////////////////////////////////////////////

uint32 CAsyncIOFileRequest::PushDecompressPage(const SStreamJobEngineState& engineState, void* pSrc, SStreamPageHdr* pSrcHdr, uint32 nBytes, bool bLast)
{
    uint32 nError = 0;

    for (uint32 nBlockPos = 0; !nError && (nBlockPos < nBytes); nBlockPos += STREAMING_BLOCK_SIZE)
    {
        bool bLastBlock = (nBlockPos + STREAMING_BLOCK_SIZE) >= nBytes;
        uint32 nBlockSize = min(nBytes - nBlockPos, (uint32)STREAMING_BLOCK_SIZE);

        nError = PushDecompressBlock(engineState, pSrc, pSrcHdr, nBlockPos, nBlockSize, bLast && bLastBlock);
    }

    return nError;
}

uint32 CAsyncIOFileRequest::PushDecompressBlock(const SStreamJobEngineState& engineState, void* pSrc, SStreamPageHdr* pSrcHdr, uint32 nOffs, uint32 nBytes, bool bLast)
{
    uint32 nError = m_nError;

    if (!nError)
    {
        if (pSrcHdr)
        {
            CryInterlockedIncrement(&pSrcHdr->nRefs);
        }

        int nPushJob = m_pDecompQueue->Push(pSrc, pSrcHdr, nOffs, nBytes, bLast);
        if (nPushJob >= 0)
        {
            STREAM_DECOMPRESS_TRACE("[StreamDecompress],PushDecompressBlock,0x%x,%s,0x%p,%i\n", CryGetCurrentThreadId(), m_strFileName.c_str(), this, nPushJob);

            AddRef();
            CAsyncIOFileRequest_TransferPtr pSelf(this);
            JobStart_Decompress(pSelf, engineState, nPushJob);
        }
    }

    return nError;
}

void CAsyncIOFileRequest::JobStart_Decompress(CAsyncIOFileRequest_TransferPtr& pSelf, const SStreamJobEngineState& engineState, int nJob)
{
    STREAM_DECOMPRESS_TRACE("[StreamDecompress],QueueDecompressBlockAppend,0x%x,%s,0x%p,%i\n", CryGetCurrentThreadId(), pSelf->m_strFileName.c_str(), &pSelf, nJob);

#if defined(STREAMENGINE_ENABLE_STATS)
    CryInterlockedIncrement(&engineState.pStats->nCurrentDecompressCount);
#endif

    CAsyncIOFileRequest* request = pSelf.Relinquish();
    if (!request->m_decompJobExecutor)
    {
        request->m_decompJobExecutor = AZStd::make_unique<AZ::LegacyJobExecutor>();
    }
    request->m_decompJobExecutor->StartJob([request, engineState, nJob]()
    {
        request->DecompressBlockEntry(engineState, nJob);
    }); // Legacy JobManager priority: eStreamPriority
}

//////////////////////////////////////////////////////////////////////////
void CAsyncIOFileRequest::JobFinalize_Read(CAsyncIOFileRequest_TransferPtr& pSelf, const SStreamJobEngineState& engineState)
{
    if (!pSelf->m_bCompressedBuffer || pSelf->HasFailed())
    {
        JobFinalize_Transfer(pSelf, engineState);
    }
}

void CAsyncIOFileRequest::JobFinalize_Decompress(CAsyncIOFileRequest_TransferPtr& pSelf, const SStreamJobEngineState& engineState)
{
    STREAM_DECOMPRESS_TRACE("[StreamDecompress],FinalizeDecompress,0x%x,%s,0x%p,0x%p,0x%p,0x%p\n", CryGetCurrentThreadId(), pSelf->m_strFileName.c_str(), &pSelf, &engineState, engineState.pStats, engineState.pDecompressStats);

    CAsyncIOFileRequest* pReq = &*pSelf;

    if (!pReq->HasFailed())
    {
        // Handle reads of subsections of a compressed file, by copying the section to the output
        uint8_t* pDst = (uint8_t*)pReq->m_pOutputMemoryBuffer;
        uint8_t* pSrc = (uint8_t*)pReq->m_pReadMemoryBuffer + pReq->m_nRequestedOffset;

        if (pDst != pSrc)
        {
            memmove(pReq->m_pOutputMemoryBuffer, pSrc, pReq->m_nRequestedSize);
        }

        pReq->JobFinalize_Validate(engineState);
    }

    pReq->JobFinalize_Buffer(engineState);

#if defined(STREAMENGINE_ENABLE_STATS) && defined(STREAMENGINE_ENABLE_TIMING)
    if (pReq->m_unzipTime.GetValue() != 0)
    {
        engineState.pDecompressStats->m_nTotalBytesUnziped += pReq->m_nFileSize;
        engineState.pDecompressStats->m_totalUnzipTime += pReq->m_unzipTime;

        engineState.pDecompressStats->m_nTempBytesUnziped += pReq->m_nFileSize;
        engineState.pDecompressStats->m_tempUnzipTime += pReq->m_unzipTime;
    }
#endif

    JobFinalize_Transfer(pSelf, engineState);
}

void CAsyncIOFileRequest::JobFinalize_Buffer(const SStreamJobEngineState& engineState)
{
    if (CryInterlockedDecrement(&m_nMemoryBufferUsers) == 0)
    {
        z_stream_s* pZlib = m_pZlibStream;

        if (pZlib)
        {
            //if the stream was cancelled in flight, inform zlib to free internal allocs
            if (pZlib->state)
            {
                inflateEnd(pZlib);
            }

            m_pZlibStream = NULL;
        }

        if (m_pMemoryBuffer)
        {
            engineState.pTempMem->TempFree(engineState.pHeap, m_pMemoryBuffer, m_nMemoryBufferSize);

            m_pMemoryBuffer = NULL;
            m_nMemoryBufferSize = 0;
        }
    }
}

void CAsyncIOFileRequest::JobFinalize_Validate([[maybe_unused]] const SStreamJobEngineState& engineState)
{
#if defined(SKIP_CHECKSUM_FROM_OPTICAL_MEDIA)
    if (m_eMediaType != eStreamSourceTypeDisc)
#endif  //SKIP_CHECKSUM_FROM_OPTICAL_MEDIA
    {
        CryOptionalAutoLock<CryCriticalSection> readLock(m_externalBufferLockRead, m_pExternalMemoryBuffer != NULL);
        if (!HasFailed())
        {
            if (m_crc32FromHeader != 0 && m_nPageReadStart == 0 && m_nRequestedSize == m_nFileSize)  //Compute the CRC32 if appropriate.
            {
#if defined(STREAMENGINE_ENABLE_TIMING)
                LARGE_INTEGER liStart, liEnd, liFreq;
                QueryPerformanceCounter(&liStart);
#endif  //STREAMENGINE_ENABLE_TIMING

                uint32 nCRC32 = crc32(0, (uint8_t*)m_pReadMemoryBuffer + m_nPageReadStart, m_nRequestedSize);

#if defined(STREAMENGINE_ENABLE_TIMING)
                QueryPerformanceCounter(&liEnd);
                QueryPerformanceFrequency(&liFreq);

                m_verifyTime += CTimeValue((int64)((liEnd.QuadPart - liStart.QuadPart) * CTimeValue::TIMEVALUE_PRECISION / liFreq.QuadPart));

                engineState.pDecompressStats->m_nTotalBytesVerified += m_nFileSize;
                engineState.pDecompressStats->m_totalVerifyTime += m_verifyTime;
                engineState.pDecompressStats->m_nTempBytesVerified += m_nFileSize;
                engineState.pDecompressStats->m_tempVerifyTime += m_verifyTime;
#endif  //STREAMENGINE_ENABLE_TIMING

                if (m_crc32FromHeader != nCRC32)
                {
                    //The contents of this file don't match what the header expects
#if !defined(_RELEASE)
                    CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR_DBGBRK, "Streaming Engine Failed to verify a file (%s). Computed CRC32 %d does not match stored CRC32 %d", m_strFileName.c_str(), nCRC32, m_crc32FromHeader);
#endif //!_RELEASE
                    Failed(ERROR_VERIFICATION_FAIL);
                }
            }
        }
    }
}

void CAsyncIOFileRequest::JobFinalize_Transfer(CAsyncIOFileRequest_TransferPtr& pSelf, const SStreamJobEngineState& engineState)
{
    STREAM_DECOMPRESS_TRACE("[StreamDecompress],FinalizeTransform,0x%x,%s,0x%p,0x%p,0x%p,0x%p\n", CryGetCurrentThreadId(), pSelf->m_strFileName.c_str(), &pSelf, &engineState, engineState.pStats, engineState.pDecompressStats);

    if (CryInterlockedCompareExchange(&pSelf->m_nFinalised, 1, 0) == 0)
    {
#if defined(STREAMENGINE_ENABLE_STATS)
        CryInterlockedIncrement(&engineState.pStats->nCurrentAsyncCount);
#endif

#if defined(STREAMENGINE_ENABLE_TIMING)
        pSelf->m_completionTime = gEnv->pTimer->GetAsyncTime();
#endif

        int nCallbackThreads = engineState.pReportQueues->size();

        EStreamTaskType eType = pSelf->m_eType;
        if (nCallbackThreads > 1 && eType == eStreamTaskTypeGeometry)
        {
            // If we have more then 1 call back threads, use this one for geometry only.
            (*engineState.pReportQueues)[1]->TransferRequest(pSelf);
        }
        else if (nCallbackThreads > 2 && eType == eStreamTaskTypeTexture)
        {
            // If we have more then 1 call back threads, use this one for textures only.
            (*engineState.pReportQueues)[2]->TransferRequest(pSelf);
        }
        else if (nCallbackThreads > 3 && eType == eStreamTaskTypeMergedMesh)
        {
            // If we have more then 3 call back threads, use this one for merged meshes only.
            (*engineState.pReportQueues)[3]->TransferRequest(pSelf);
        }
        else if (nCallbackThreads > 0)
        {
            (*engineState.pReportQueues)[0]->TransferRequest(pSelf);
        }
        else
        {
            __debugbreak();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void SStreamRequestQueue::TransferRequest(CAsyncIOFileRequest_TransferPtr& pRequest)
{
    {
        CryAutoLock<CryCriticalSection> l(m_lock);

        m_requests.push_back(pRequest.Relinquish());
    }

    m_awakeEvent.Set();
}

//////////////////////////////////////////////////////////////////////////
void SStreamEngineTempMemStats::TempFree(CMTSafeHeap* pHeap, const void* p, size_t nSize)
{
#if MTSAFE_USE_GENERAL_HEAP
    bool bInGenHeap = pHeap->IsInGeneralHeap(p);
#else
    bool bInGenHeap = false;
#endif
    pHeap->FreeTemporary(const_cast<void*>(p));
    ReportTempMemAlloc(0, bInGenHeap ? 0 : nSize, true);
}

void SStreamEngineTempMemStats::ReportTempMemAlloc(uint32 nSizeAlloc, uint32 nSizeFree, bool bTriggerWake)
{
    int nAdd = (int)nSizeAlloc - (int)nSizeFree;
    int const nOldSize = CryInterlockedExchangeAdd(&m_nTempAllocatedMemory, nAdd);
    int const nNewSize = nOldSize + nAdd;

    LONG nNewMax = 0;
    LONG nOldMax = 0;
    do
    {
        nOldMax = m_nTempAllocatedMemoryFrameMax;
        nNewMax = (LONG)max((int)nNewSize, (int)nOldMax);
    }
    while (CryInterlockedCompareExchange(&m_nTempAllocatedMemoryFrameMax, nNewMax, nOldMax) != nOldMax);

    if (bTriggerWake)
    {
        for (int i = 0, c = m_nWakeEvents; i != c; ++i)
        {
            m_wakeEvents[i]->Set();
        }
    }
}
