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

#if defined(ENABLE_LOADING_PROFILER)

#include "BootProfiler.h"
#include "ThreadInfo.h"
#include <stack>
#include <AzFramework/IO/FileOperations.h>


namespace
{
    StaticInstance<CBootProfiler, AZStd::no_destruct<CBootProfiler>> gProfilerInstance;
    enum
    {
        eMAX_THREADS_TO_PROFILE = 128,
        eNUM_RECORDS_PER_POOL = 2048, // so, eNUM_RECORDS_PER_POOL * sizeof(CBootProfilerRecord) == mem consumed by pool item
        // sizeof(CProfileBlockTimes)==152,
        // poolmem = 304Kb for 1 pool per thread
    };
}

int CBootProfiler::CV_sys_bp_frames = 0;
float CBootProfiler::CV_sys_bp_time_threshold = 0;

class CProfileBlockTimes
{
protected:
    LARGE_INTEGER m_startTimeStamp;
    LARGE_INTEGER m_stopTimeStamp;
    LARGE_INTEGER m_freq;
    CProfileBlockTimes()
    {
        memset(&m_startTimeStamp, 0, sizeof(m_startTimeStamp));
        memset(&m_stopTimeStamp, 0, sizeof(m_stopTimeStamp));
        memset(&m_freq, 0, sizeof(m_freq));
    }
};

class CBootProfilerRecord
{
public:
    const char* m_label;
    LARGE_INTEGER m_startTimeStamp;
    LARGE_INTEGER m_stopTimeStamp;
    LARGE_INTEGER m_freq;

    CBootProfilerRecord* m_pParent;
    typedef AZStd::vector<CBootProfilerRecord*> ChildVector;
    ChildVector m_Childs;

    CryFixedStringT<256> m_args;

    ILINE CBootProfilerRecord(const char* label, LARGE_INTEGER timestamp, LARGE_INTEGER freq, const char* args)
        : m_label(label)
        , m_startTimeStamp(timestamp)
        , m_freq(freq)
        , m_pParent(NULL)
    {
        memset(&m_stopTimeStamp, 0, sizeof(m_stopTimeStamp));
        if (args)
        {
            m_args = args;
        }
    }

    ILINE ~CBootProfilerRecord()
    {
        // childs are allocated via pool as well, the destructors of each child
        // is called explicitly, for the purpose of freeing memory occupied by
        // m_Child vector. Otherwise there will be a memory leak.
        ChildVector::iterator it = m_Childs.begin();
        while (it != m_Childs.end())
        {
            (*it)->~CBootProfilerRecord();
            ++it;
        }
    }

    void Print(AZ::IO::HandleType fileHandle, char* buf, size_t buf_size, size_t depth, LARGE_INTEGER stopTime, const char* threadName, const float timeThreshold)
    {
        if (m_stopTimeStamp.QuadPart == 0)
        {
            m_stopTimeStamp = stopTime;
        }

        const float time = (float)(m_stopTimeStamp.QuadPart - m_startTimeStamp.QuadPart) * 1000.f / (float)m_freq.QuadPart;

        if (timeThreshold > 0.0f && time < timeThreshold)
        {
            return;
        }

        string tabs; //tabs(depth++, '\t')
        tabs.insert(0, depth++, '\t');

        {
            string label = m_label;
            label.replace("&", "&amp;");
            label.replace("<", "&lt;");
            label.replace(">", "&gt;");
            label.replace("\"", "&quot;");
            label.replace("'", "&apos;");

            if (m_args.size() > 0)
            {
                m_args.replace("&", "&amp;");
                m_args.replace("<", "&lt;");
                m_args.replace(">", "&gt;");
                m_args.replace("\"", "&quot;");
                m_args.replace("'", "&apos;");
                m_args.replace("%", "&#37;");
            }

            sprintf_s(buf, buf_size, "%s<block name=\"%s\" totalTimeMS=\"%f\" startTime=\"%" PRIu64 "\" stopTime=\"%" PRIu64 "\" args=\"%s\"> \n",
                tabs.c_str(), label.c_str(), time, m_startTimeStamp.QuadPart, m_stopTimeStamp.QuadPart, m_args.c_str());
            AZ::IO::Print(fileHandle, buf);
        }

        const size_t childsSize = m_Childs.size();
        for (size_t i = 0; i < childsSize; ++i)
        {
            CBootProfilerRecord* record = m_Childs[i];
            assert(record);
            record->Print(fileHandle, buf, buf_size, depth, stopTime, threadName, timeThreshold);
        }

        sprintf_s(buf, buf_size, "%s</block>\n", tabs.c_str());
        AZ::IO::Print(fileHandle, buf);
    }
};

//////////////////////////////////////////////////////////////////////////

class CProfileInfo
{
    friend class CBootProfilerSession;
private:
    CBootProfilerRecord* m_pRoot;
    CBootProfilerRecord* m_pCurrent;
public:
    CProfileInfo()
        : m_pRoot(NULL)
        , m_pCurrent(NULL) {}
};


class CBootProfilerThreadsInterface
{
protected:
    CBootProfilerThreadsInterface()
    {
        memset(m_threadInfo, 0, sizeof(m_threadInfo));
        m_threadCounter = 0;
    }

    unsigned int GetThreadIndexByID(unsigned int threadID);
    const char* GetThreadNameByIndex(unsigned int threadIndex);

    int m_threadCounter;
private:
    unsigned int m_threadInfo[eMAX_THREADS_TO_PROFILE]; //threadIDs
};

//////////////////////////////////////////////////////////////////////////
ILINE unsigned int CBootProfilerThreadsInterface::GetThreadIndexByID(unsigned int threadID)
{
    for (int i = 0; i < eMAX_THREADS_TO_PROFILE; ++i)
    {
        if (m_threadInfo[i] == 0)
        {
            break;
        }
        if (m_threadInfo[i] == threadID)
        {
            return i;
        }
    }

    unsigned int counter = CryInterlockedIncrement(&m_threadCounter) - 1; //count to index
    m_threadInfo[counter] = threadID;

    return counter;
}

ILINE const char* CBootProfilerThreadsInterface::GetThreadNameByIndex(unsigned int threadIndex)
{
    assert(threadIndex < m_threadCounter);

    const char* threadName = CryThreadGetName(m_threadInfo[threadIndex]);
    return threadName;
}

class CRecordPool
{
public:
    CRecordPool()
        : m_baseAddr(NULL)
        , m_allocCounter(0)
        , m_next(NULL)
    {
        m_baseAddr = (CBootProfilerRecord*)CryModuleMemalign(eNUM_RECORDS_PER_POOL *    sizeof(CBootProfilerRecord), 16);
    }
    ~CRecordPool()
    {
        CryModuleMemalignFree(m_baseAddr);
        delete m_next;
    }

    ILINE CBootProfilerRecord* allocateRecord()
    {
        if (m_allocCounter < eNUM_RECORDS_PER_POOL)
        {
            CBootProfilerRecord* newRecord = m_baseAddr + m_allocCounter;
            ++m_allocCounter;
            return newRecord;
        }
        else
        {
            return NULL;
        }
    }

    ILINE void setNextPool(CRecordPool* pool) { m_next = pool; }

private:
    CBootProfilerRecord* m_baseAddr;
    uint32 m_allocCounter;

    CRecordPool* m_next;
};

class CBootProfilerSession
    : public CBootProfilerThreadsInterface
    , protected CProfileBlockTimes
{
public:
    CBootProfilerSession();
    ~CBootProfilerSession();

    void Start();
    void Stop();

    CBootProfilerRecord* StartBlock(const char* name, const char* args);
    void StopBlock(CBootProfilerRecord* record);

    void CollectResults(const char* filename, const float timeThreshold);

private:
    string m_name;

    CProfileInfo m_threadsProfileInfo[eMAX_THREADS_TO_PROFILE];
    CRecordPool* m_threadsRecordsPool[eMAX_THREADS_TO_PROFILE];     //head
    CRecordPool* m_threadsCurrentPools[eMAX_THREADS_TO_PROFILE];  //current
};


//////////////////////////////////////////////////////////////////////////

CBootProfilerSession::CBootProfilerSession()
{
    memset(m_threadsProfileInfo, 0, sizeof(m_threadsProfileInfo));

    memset(m_threadsRecordsPool, 0, sizeof(m_threadsRecordsPool));
    memset(m_threadsCurrentPools, 0, sizeof(m_threadsCurrentPools));
}

CBootProfilerSession::~CBootProfilerSession()
{
    for (unsigned int i = 0; i < m_threadCounter; ++i)
    {
        CProfileInfo& profile = m_threadsProfileInfo[i];

        // Since m_pRoot is allocated using memory pool (line 296),
        // its destructor is called explicitly to free the memory of
        // m_Childs and each of its child.

        if (profile.m_pRoot)
        {
            profile.m_pRoot->~CBootProfilerRecord();
        }
        delete m_threadsRecordsPool[i];
    }
}

void CBootProfilerSession::Start()
{
    LARGE_INTEGER time, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&time);
    m_startTimeStamp = time;
    m_freq = freq;
}

void CBootProfilerSession::Stop()
{
    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    m_stopTimeStamp = time;
}

CBootProfilerRecord* CBootProfilerSession::StartBlock(const char* name, const char* args)
{
    const unsigned int curThread = CryGetCurrentThreadId();
    const unsigned int threadIndex = GetThreadIndexByID(curThread);

    assert(threadIndex < eMAX_THREADS_TO_PROFILE);

    CProfileInfo& profile = m_threadsProfileInfo[threadIndex];

    CRecordPool* pool = m_threadsCurrentPools[threadIndex];

    if (!profile.m_pRoot)
    {
        if (!pool)
        {
            pool = new CRecordPool;
            m_threadsRecordsPool[threadIndex] = pool;
            m_threadsCurrentPools[threadIndex] = pool;
        }

        CBootProfilerRecord* rec = pool->allocateRecord();
        profile.m_pRoot = profile.m_pCurrent = new(rec)CBootProfilerRecord("root", m_startTimeStamp, m_freq, args);
    }

    assert(pool);

    LARGE_INTEGER time, freq;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&time);

    CBootProfilerRecord* pParent = profile.m_pCurrent;
    assert(pParent);
    assert(profile.m_pRoot);

    CBootProfilerRecord* rec = pool->allocateRecord();
    if (!rec)
    {
        //pool is full, create a new one
        pool = new CRecordPool;
        m_threadsCurrentPools[threadIndex]->setNextPool(pool);
        m_threadsCurrentPools[threadIndex] = pool;

        rec = pool->allocateRecord();
    }

    profile.m_pCurrent = new(rec)CBootProfilerRecord(name, time, freq, args);
    profile.m_pCurrent->m_pParent = pParent;
    pParent->m_Childs.push_back(profile.m_pCurrent);

    return profile.m_pCurrent;
}

void CBootProfilerSession::StopBlock(CBootProfilerRecord* record)
{
    if (record)
    {
        LARGE_INTEGER time;
        QueryPerformanceCounter(&time);
        record->m_stopTimeStamp = time;

        unsigned int curThread = CryGetCurrentThreadId();
        unsigned int threadIndex = GetThreadIndexByID(curThread);
        assert(threadIndex < eMAX_THREADS_TO_PROFILE);

        CProfileInfo& profile = m_threadsProfileInfo[threadIndex];
        profile.m_pCurrent = record->m_pParent;
    }
}

void CBootProfilerSession::CollectResults(const char* filename, const float timeThreshold)
{
    if (!gEnv || !gEnv->pCryPak)
    {
        AZ_Warning("BootProfiler", false, "CryPak not set - skipping CollectResults");
        return;
    }
    static const char* szTestResults = "@cache@\\TestResults";
    string filePath = string(szTestResults) + "\\" + "bp_" + filename + ".xml";
    char path[AZ::IO::IArchive::MaxPath] = "";
    gEnv->pCryPak->AdjustFileName(filePath.c_str(), path, AZ_ARRAY_SIZE(path), AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FLAGS_FOR_WRITING);
    gEnv->pCryPak->MakeDir(szTestResults);

    AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
    gEnv->pFileIO->Open(path, AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary, fileHandle);
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return;
    }

    char buf[512];
    const unsigned int buf_size = sizeof(buf);

    sprintf_s(buf, buf_size, "<root>\n");
    AZ::IO::Print(fileHandle, buf);

    const size_t numThreads = m_threadCounter;
    for (size_t i = 0; i < numThreads; ++i)
    {
        CBootProfilerRecord* pRoot = m_threadsProfileInfo[i].m_pRoot;
        if (pRoot)
        {
            pRoot->m_stopTimeStamp = m_stopTimeStamp;

            const char* threadName = GetThreadNameByIndex(i);
            if (!threadName)
            {
                threadName = "UNKNOWN";
            }


            const float time = (float)(pRoot->m_stopTimeStamp.QuadPart - pRoot->m_startTimeStamp.QuadPart) * 1000.f / (float)pRoot->m_freq.QuadPart;

            sprintf_s(buf, buf_size, "\t<thread name=\"%s\" totalTimeMS=\"%f\" startTime=\"%" PRIu64 "\" stopTime=\"%" PRIu64 "\" > \n", threadName, time,
                pRoot->m_startTimeStamp.QuadPart, pRoot->m_stopTimeStamp.QuadPart);
            AZ::IO::Print(fileHandle, buf);

            for (size_t recordIdx = 0; recordIdx < pRoot->m_Childs.size(); ++recordIdx)
            {
                CBootProfilerRecord* record = pRoot->m_Childs[recordIdx];
                assert(record);
                record->Print(fileHandle, buf, buf_size, 2, m_stopTimeStamp, threadName, timeThreshold);
            }

            sprintf_s(buf, buf_size, "\t</thread>\n");
            AZ::IO::Print(fileHandle, buf);
        }
    }

    sprintf_s(buf, buf_size, "</root>\n");
    AZ::IO::Print(fileHandle, buf);
    gEnv->pFileIO->Close(fileHandle);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CBootProfiler& CBootProfiler::GetInstance()
{
    return gProfilerInstance;
}

CBootProfiler::CBootProfiler()
    : m_pCurrentSession(NULL)
    , m_pFrameRecord(NULL)
    , m_levelLoadAdditionalFrames(0)
{
}

CBootProfiler::~CBootProfiler()
{
    AZStd::lock_guard<AZStd::recursive_mutex> recordGuard{ m_recordMutex };
    for (TSessionMap::iterator it = m_sessions.begin(); it != m_sessions.end(); ++it)
    {
        CBootProfilerSession* session = it->second;
        delete session;
    }
}

// start session
void CBootProfiler::StartSession(const char* sessionName)
{
    AZStd::lock_guard<AZStd::recursive_mutex> recordGuard{ m_recordMutex };

    TSessionMap::const_iterator it = m_sessions.find(sessionName);
    if (it == m_sessions.end())
    {
        m_pCurrentSession = new CBootProfilerSession();
        m_sessions[sessionName] = m_pCurrentSession;
        m_pCurrentSession->Start();
    }
}

// stop session
void CBootProfiler::StopSession(const char* sessionName)
{
    AZStd::lock_guard<AZStd::recursive_mutex> recordGuard{ m_recordMutex };
    if (m_pCurrentSession)
    {
        TSessionMap::iterator it = m_sessions.find(sessionName);
        if (it != m_sessions.end())
        {
            if (m_pCurrentSession == it->second)
            {
                CBootProfilerSession* session = m_pCurrentSession;
                m_pCurrentSession = NULL;

                session->Stop();
                session->CollectResults(sessionName, CV_sys_bp_time_threshold);

                delete session;
            }
            m_sessions.erase(it);
        }
    }
}

CBootProfilerRecord* CBootProfiler::StartBlock(const char* name, const char* args)
{
    AZStd::lock_guard<AZStd::recursive_mutex> recordGuard{ m_recordMutex };
    if (m_pCurrentSession)
    {
        return m_pCurrentSession->StartBlock(name, args);
    }
    return NULL;
}

void CBootProfiler::StopBlock(CBootProfilerRecord* record)
{
    AZStd::lock_guard<AZStd::recursive_mutex> recordGuard{ m_recordMutex };
    if (m_pCurrentSession)
    {
        m_pCurrentSession->StopBlock(record);
    }
}

void CBootProfiler::StartFrame(const char* name)
{
    AZStd::lock_guard<AZStd::recursive_mutex> recordGuard{ m_recordMutex };
    if (CV_sys_bp_frames)
    {
        StartSession("frames");
        m_pFrameRecord = StartBlock(name, NULL);
    }
}

void CBootProfiler::StopFrame()
{
    AZStd::lock_guard<AZStd::recursive_mutex> recordGuard{ m_recordMutex };
    if (m_pCurrentSession && CV_sys_bp_frames)
    {
        StopBlock(m_pFrameRecord);
        m_pFrameRecord = NULL;

        --CV_sys_bp_frames;
        if (0 == CV_sys_bp_frames)
        {
            StopSession("frames");
        }
    }

    if (m_pCurrentSession && m_levelLoadAdditionalFrames)
    {
        --m_levelLoadAdditionalFrames;
        if (0 == m_levelLoadAdditionalFrames)
        {
            StopSession("level");
        }
    }
}

void CBootProfiler::Init(ISystem* pSystem)
{
    //REGISTER_CVAR(sys_BootProfiler, 1, VF_DEV_ONLY,
    //  "Collect and output session statistics into TestResults/bp_(session_name).xml   \n"
    //  "0 = Disabled\n"
    //  "1 = Enabled\n");

    pSystem->GetISystemEventDispatcher()->RegisterListener(this);
    StartSession("boot");
}

void CBootProfiler::RegisterCVars()
{
    REGISTER_CVAR2("sys_bp_frames", &CV_sys_bp_frames, 0, VF_DEV_ONLY, "Starts frame profiling for specified number of frames using BootProfiler");
    REGISTER_CVAR2("sys_bp_time_threshold", &CV_sys_bp_time_threshold, 0.1f, VF_DEV_ONLY, "If greater than 0 don't write blocks that took less time (default 0.1 ms)");
}

void CBootProfiler::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
{
    switch (event)
    {
    case ESYSTEM_EVENT_GAME_POST_INIT_DONE:
    {
        StopSession("boot");
        break;
    }
    case ESYSTEM_EVENT_GAME_MODE_SWITCH_START:
    {
        break;
    }

    case ESYSTEM_EVENT_GAME_MODE_SWITCH_END:
    {
        break;
    }

    case ESYSTEM_EVENT_LEVEL_LOAD_START:
    {
        break;
    }
    case ESYSTEM_EVENT_LEVEL_LOAD_PREPARE:
    {
        StartSession("level");
        break;
    }
    case ESYSTEM_EVENT_LEVEL_LOAD_END:
    {
        StopSession("level");
        break;
    }
    case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
    {
        //level loading can be stopped here, or m_levelLoadAdditionalFrames can be used to prolong dump for this amount of frames
        //StopSession("level");
        m_levelLoadAdditionalFrames = 20;
        break;
    }
    }
}

void CBootProfiler::SetFrameCount(int frameCount)
{
    CV_sys_bp_frames = frameCount;
}
#endif
