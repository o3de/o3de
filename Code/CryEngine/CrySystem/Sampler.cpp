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
#include "Sampler.h"

#if defined(WIN32)

#include <ISystem.h>
#include <Mmsystem.h>
#include <AzCore/Debug/StackTracer.h>

#define MAX_SYMBOL_LENGTH 512

//////////////////////////////////////////////////////////////////////////
// Makes thread.
//////////////////////////////////////////////////////////////////////////
class CSamplingThread
{
public:
    CSamplingThread(CSampler* pSampler)
    {
        m_hThread = NULL;
        m_pSampler = pSampler;
        m_bStop = false;
        m_samplePeriodMs = pSampler->GetSamplePeriod();

        m_hProcess = GetCurrentProcess();
        m_hSampledThread = GetCurrentThread();
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &m_hSampledThread, 0, FALSE, DUPLICATE_SAME_ACCESS);
    }

    // Start thread.
    void Start();
    void Stop();

protected:
    virtual ~CSamplingThread() {};
    static DWORD WINAPI ThreadFunc(void* pThreadParam);
    void Run(); // Derived classes must override this.

    HANDLE m_hProcess;
    HANDLE m_hThread;
    HANDLE m_hSampledThread;
    DWORD m_ThreadId;

    CSampler* m_pSampler;
    bool m_bStop;
    int m_samplePeriodMs;
};

//////////////////////////////////////////////////////////////////////////
void CSamplingThread::Start()
{
    m_hThread = CreateThread(NULL, 0, ThreadFunc, this, 0, &m_ThreadId);
}

//////////////////////////////////////////////////////////////////////////
void CSamplingThread::Stop()
{
    m_bStop = true;
}

//////////////////////////////////////////////////////////////////////////
DWORD CSamplingThread::ThreadFunc(void* pThreadParam)
{
    CSamplingThread* thread = (CSamplingThread*)pThreadParam;
    thread->Run();
    // Auto destruct thread class.
    delete thread;
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CSamplingThread::Run()
{
    //SetThreadPriority( m_hThread,THREAD_PRIORITY_HIGHEST );
    SetThreadPriority(m_hThread, THREAD_PRIORITY_TIME_CRITICAL);
    while (!m_bStop)
    {
        SuspendThread(m_hSampledThread);

        CONTEXT context;
        context.ContextFlags = CONTEXT_CONTROL;

        uint64 ip = 0;
        if (GetThreadContext(m_hSampledThread, &context))
        {
#ifdef CONTEXT_i386
            ip = context.Eip;
#else
            ip = context.Rip;
#endif
        }
        ResumeThread(m_hSampledThread);

        if (!m_pSampler->AddSample(ip))
        {
            break;
        }

        Sleep(m_samplePeriodMs);
    }
}

//////////////////////////////////////////////////////////////////////////
CSampler::CSampler()
{
    m_pSamplingThread = NULL;
    SetMaxSamples(2000);
    m_bSamplingFinished = false;
    m_bSampling = false;
    m_samplePeriodMs = 1; //1ms
}

//////////////////////////////////////////////////////////////////////////
CSampler::~CSampler()
{
}

//////////////////////////////////////////////////////////////////////////
void CSampler::SetMaxSamples(int nMaxSamples)
{
    m_rawSamples.reserve(nMaxSamples);
    m_nMaxSamples = nMaxSamples;
}

//////////////////////////////////////////////////////////////////////////
void CSampler::Start()
{
    if (m_bSampling)
    {
        return;
    }

    CryLogAlways("Staring Sampling with interval %dms, max samples: %d ...", m_samplePeriodMs, m_nMaxSamples);

    m_bSampling = true;
    m_bSamplingFinished = false;
    m_pSamplingThread = new CSamplingThread(this);
    m_rawSamples.clear();
    m_functionSamples.clear();

    m_pSamplingThread->Start();
}

//////////////////////////////////////////////////////////////////////////
void CSampler::Stop()
{
    if (m_bSamplingFinished)
    {
    }
    if (m_bSampling)
    {
        m_pSamplingThread->Stop();
    }
    m_bSampling = false;
    m_pSamplingThread = 0;
}

//////////////////////////////////////////////////////////////////////////
void CSampler::Update()
{
    if (m_bSamplingFinished)
    {
        ProcessSampledData();
        m_bSamplingFinished = false;
    }
}

//////////////////////////////////////////////////////////////////////////
bool CSampler::AddSample(uint64 ip)
{
    if ((int)m_rawSamples.size() >= m_nMaxSamples)
    {
        m_bSamplingFinished = true;
        m_bSampling = false;
        m_pSamplingThread = 0;
        return false;
    }
    m_rawSamples.push_back(ip);
    return true;
}

inline bool CompareFunctionSamples(const CSampler::SFunctionSample& s1, const CSampler::SFunctionSample& s2)
{
    return s1.nSamples < s2.nSamples;
}

//////////////////////////////////////////////////////////////////////////
void CSampler::ProcessSampledData()
{
    CryLogAlways("Processing collected samples...");

    uint32 i;
    // Count duplicates.
    std::map<uint64, int> counts;
    std::map<uint64, int>::iterator cit;
    for (i = 0; i < m_rawSamples.size(); i++)
    {
        uint32 ip = (uint32)m_rawSamples[i];
        cit = counts.find(ip);
        if (cit != counts.end())
        {
            cit->second++;
        }
        else
        {
            counts[ip] = 0;
        }
    }

    std::map<string, int> funcCounts;

    AZ::Debug::SymbolStorage::StackLine func, file, module;
    int line;
    void* baseAddr;
    string funcName;
    for (i = 0; i < m_rawSamples.size(); i++)
    {
        // lookup module name here, and aggregate the results
        AZ::Debug::SymbolStorage::FindFunctionFromIP((void*)m_rawSamples[i], &func, &file, &module, line, baseAddr);

        // Developer note: this file was using the module name instead of the function name.  There was stub code
        // to use the function name instead that.  This function was updated to use FindFunctionFromIP(), but
        // continues to use the module instead of the function.
        funcName = module;
        funcCounts[funcName] += 1;
    }

    {
        // Combine function samples.
        std::map<string, int>::iterator it;
        for (it = funcCounts.begin(); it != funcCounts.end(); ++it)
        {
            SFunctionSample fs;
            fs.function = it->first;
            fs.nSamples = it->second;
            m_functionSamples.push_back(fs);
        }
    }

    // Sort vector by number of samples.
    std::sort(m_functionSamples.begin(), m_functionSamples.end(), CompareFunctionSamples);

    LogSampledData();
}

//////////////////////////////////////////////////////////////////////////
void CSampler::LogSampledData()
{
    int nTotalSamples = m_rawSamples.size();

    // Log sample info.
    CryLogAlways("=========================================================================");
    CryLogAlways("= Profiler Output");
    CryLogAlways("=========================================================================");

    float fOnePercent = (float)nTotalSamples / 100;

    float fPercentTotal = 0;
    int nSampleSum = 0;
    for (uint32 i = 0; i < m_functionSamples.size(); i++)
    {
        // Calculate percentage.
        float fPercent = m_functionSamples[i].nSamples / fOnePercent;
        const char* func = m_functionSamples[i].function;
        CryLogAlways("%6.2f%% (%4d samples) : %s", fPercent, m_functionSamples[i].nSamples, func);
        fPercentTotal += fPercent;
        nSampleSum += m_functionSamples[i].nSamples;
    }
    CryLogAlways("Samples: %d / %d (%.2f%%)", nSampleSum, nTotalSamples, fPercentTotal);
    CryLogAlways("=========================================================================");
}


#endif // defined(WIN32)
