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

// Description : Streaming Thread for IO


#ifndef CRYINCLUDE_CRYSYSTEM_STREAMENGINE_STREAMIOTHREAD_H
#define CRYINCLUDE_CRYSYSTEM_STREAMENGINE_STREAMIOTHREAD_H
#pragma once

#include <IStreamEngine.h>
#include "StreamAsyncFileRequest.h"


class CStreamEngine;

//////////////////////////////////////////////////////////////////////////
// Thread that performs IO operations.
//////////////////////////////////////////////////////////////////////////
class CStreamingIOThread
    : public CrySimpleThread<CStreamingIOThread>
    , public CMultiThreadRefCount
{
public:
    CStreamingIOThread(CStreamEngine* pStreamEngine, EStreamSourceMediaType mediaType, const char* name);
    ~CStreamingIOThread();

    void CancelAll();
    void AbortAll(bool bAbort);

    void BeginReset();
    void EndReset();

    void AddRequest(CAsyncIOFileRequest* pRequest, bool bStartImmidietly);
    int GetRequestCount() const { return m_fileRequestQueue.size(); };
    void SortRequests();
    void NeedSorting();
    void SignalStartWork(bool bForce);
    bool HasUrgentRequests();
    EStreamSourceMediaType GetMediaType() const { return m_eMediaType; }
    bool IsMisscheduled(EStreamSourceMediaType mt) const;

    void Pause(bool bPause);

    void RegisterFallbackIOThread(EStreamSourceMediaType mediaType, CStreamingIOThread* pIOThread);

    CStreamEngineWakeEvent& GetWakeEvent() { return m_awakeEvent; }

    //////////////////////////////////////////////////////////////////////////
    // CrySimpleThread
    //////////////////////////////////////////////////////////////////////////
    virtual void Run();
    virtual void Cancel();
    //////////////////////////////////////////////////////////////////////////

protected:

    void ProcessNewRequests();
    void ProcessReset();

public:

#ifdef STREAMENGINE_ENABLE_STATS
    struct SStats
    {
        SStats()
            : m_nTotalReadBytes(0)
            , m_nCurrentReadBandwith(0)
            , m_nReadBytesInLastSecond(0)
            , m_fReadingDuringLastSecond(.0f)
            , m_nTempBytesRead(0)
            , m_nActualReadBandwith(0)
            , m_nTempReadOffset(0)
            , m_nTotalReadOffset(0)
            , m_nReadOffsetInLastSecond(0)
            , m_nTempRequestCount(0)
            , m_nTotalRequestCount(0)
            , m_nRequestCountInLastSecond(0)
        {}

        void Update(const CTimeValue& deltaT);

        void Reset()
        {
            m_nTotalReadBytes = 0;
            m_nTotalReadOffset = 0;
            m_nTotalRequestCount = 0;
            m_TotalReadTime.SetValue(0);
        }

        float m_fReadingDuringLastSecond;
        CTimeValue m_TotalReadTime;
        uint64 m_nTotalReadBytes;
        uint64 m_nTotalReadOffset;
        uint32 m_nTotalRequestCount;
        uint32 m_nCurrentReadBandwith;          // Read bandwidth over one second
        uint32 m_nActualReadBandwith;               // Actual read bandwidth extrapolated over one second
        uint32 m_nReadBytesInLastSecond;
        uint32 m_nRequestCountInLastSecond;
        uint64 m_nReadOffsetInLastSecond;

        uint32 m_nTempRequestCount;
        uint64 m_nTempBytesRead;
        uint64 m_nTempReadOffset;
        CTimeValue m_TempReadTime;
    };

    SStats m_InMemoryStats;
    SStats m_NotInMemoryStats;
#endif

    int64 m_nLastReadDiskOffset;
    int m_nStreamingCPU;

private:
    CStreamEngine* m_pStreamEngine;
    std::vector<CAsyncIOFileRequest*> m_fileRequestQueue;
    std::vector<CAsyncIOFileRequest*> m_temporaryArray;
    CryMT::vector<CAsyncIOFileRequest*> m_newFileRequests;

    EStreamSourceMediaType m_eMediaType;
    uint32 m_nFallbackMTs;

    typedef std::pair<CStreamingIOThread*, EStreamSourceMediaType> TFallbackIOPair;
    typedef std::vector<TFallbackIOPair> TFallbackIOVec;
    typedef TFallbackIOVec::iterator TFallbackIOVecConstIt;
    TFallbackIOVec m_FallbackIOThreads;

    volatile bool m_bCancelThreadRequest;
    volatile bool m_bNeedSorting;
    volatile bool m_bNewRequests;
    volatile bool m_bPaused;
    volatile bool m_bNeedReset;
    volatile bool m_bAbortReads;

    volatile int m_iUrgentRequests;

    CStreamEngineWakeEvent m_awakeEvent;
    CryEvent m_resetDoneEvent;
    string m_name;
    uint32 m_nReadCounter;
};

//////////////////////////////////////////////////////////////////////////
// Thread that performs IO operations.
//////////////////////////////////////////////////////////////////////////
class CStreamingWorkerThread
    : public CrySimpleThread<CStreamingIOThread>
    , public CMultiThreadRefCount
{
public:
    enum EWorkerType
    {
        eWorkerAsyncCallback,
    };
    CStreamingWorkerThread(CStreamEngine* pStreamEngine, const char* name, EWorkerType type, SStreamRequestQueue* pQueue);
    ~CStreamingWorkerThread();

    void BeginReset();
    void EndReset();

    void CancelAll();

    //////////////////////////////////////////////////////////////////////////
    // CrySimpleThread
    //////////////////////////////////////////////////////////////////////////
    virtual void Run();
    virtual void Cancel();
    //////////////////////////////////////////////////////////////////////////

private:
    EWorkerType m_type;
    CStreamEngine* m_pStreamEngine;
    SStreamRequestQueue* m_pQueue;

    volatile bool m_bCancelThreadRequest;
    volatile bool m_bNeedsReset;

    CryEvent m_resetDoneEvent;
    string m_name;
};

#endif // CRYINCLUDE_CRYSYSTEM_STREAMENGINE_STREAMIOTHREAD_H
