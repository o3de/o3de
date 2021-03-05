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

#include "System.h"
#include "LoadingProfiler.h"

#define LOADING_TIME_CONTAINER_MAX_TEXT_SIZE 1024
#define MAX_LOADING_TIME_PROFILER_STACK_DEPTH 16

//#define SAVE_SAVELEVELSTATS_IN_ROOT

struct SLoadingTimeContainer
    : public _i_reference_target_t
{
    SLoadingTimeContainer()     {}

    SLoadingTimeContainer(SLoadingTimeContainer* pParent, const char* pPureFuncName, const int nRootIndex)
    {
        m_dSelfMemUsage = m_dTotalMemUsage = m_dSelfTime = m_dTotalTime = 0;
        m_nCounter = 1;
        m_pFuncName = pPureFuncName;
        m_pParent = pParent;
        m_nRootIndex = nRootIndex;
    }

    static int Cmp_SLoadingTimeContainer_Time(const void* v1, const void* v2)
    {
        SLoadingTimeContainer* pChunk1 = (SLoadingTimeContainer*)v1;
        SLoadingTimeContainer* pChunk2 = (SLoadingTimeContainer*)v2;

        if (pChunk1->m_dSelfTime > pChunk2->m_dSelfTime)
        {
            return -1;
        }
        else if (pChunk1->m_dSelfTime < pChunk2->m_dSelfTime)
        {
            return 1;
        }

        return 0;
    }

    static int Cmp_SLoadingTimeContainer_MemUsage(const void* v1, const void* v2)
    {
        SLoadingTimeContainer* pChunk1 = (SLoadingTimeContainer*)v1;
        SLoadingTimeContainer* pChunk2 = (SLoadingTimeContainer*)v2;

        if (pChunk1->m_dSelfMemUsage > pChunk2->m_dSelfMemUsage)
        {
            return -1;
        }
        else if (pChunk1->m_dSelfMemUsage < pChunk2->m_dSelfMemUsage)
        {
            return 1;
        }

        return 0;
    }

    static double GetUsedMemory(ISystem* pSysytem)
    {
        static IMemoryManager::SProcessMemInfo processMemInfo;
        pSysytem->GetIMemoryManager()->GetProcessMemInfo(processMemInfo);
        return double(processMemInfo.PagefileUsage) / double(1024 * 1024);
    }


    void Clear()
    {
        for (size_t i = 0, end = m_pChilds.size(); i < end; ++i)
        {
            delete m_pChilds[i];
        }
    }

    ~SLoadingTimeContainer()
    {
        Clear();
    }


    double m_dSelfTime, m_dTotalTime;
    double m_dSelfMemUsage, m_dTotalMemUsage;
    uint32 m_nCounter;

    const char* m_pFuncName;
    SLoadingTimeContainer* m_pParent;
    int m_nRootIndex;
    std::vector<SLoadingTimeContainer*> m_pChilds;

    DiskOperationInfo m_selfInfo;
    DiskOperationInfo m_totalInfo;
    bool m_bUsed;
};

bool operator== (const SLoadingTimeContainer& a, const SLoadingTimeContainer& b)
{
    return b.m_pFuncName == a.m_pFuncName;
}

bool operator== (const SLoadingTimeContainer& a, const char* b)
{
    return b == a.m_pFuncName;
}


SLoadingTimeContainer* CLoadingProfilerSystem::m_pCurrentLoadingTimeContainer = 0;
SLoadingTimeContainer* CLoadingProfilerSystem::m_pRoot[2] = {0, 0};
int CLoadingProfilerSystem::m_iActiveRoot = 0;
ICVar* CLoadingProfilerSystem::m_pEnableProfile = 0;
int CLoadingProfilerSystem::nLoadingProfileMode = 1;
int CLoadingProfilerSystem::nLoadingProfilerNotTrackedAllocations = -1;
CryCriticalSection CLoadingProfilerSystem::csLock;

//////////////////////////////////////////////////////////////////////////
void CLoadingProfilerSystem::OutputLoadingTimeStats(ILog* pLog, int nMode)
{
    nLoadingProfileMode = nMode;

    PodArray<SLoadingTimeContainer> arrNoStack;
    CreateNoStackList(arrNoStack);


    if (nLoadingProfileMode > 0)
    { // loading mem stats per func
        pLog->Log("------ Level loading memory allocations (MB) by function ------------");
        pLog->Log(" ||Self |  Total |  Calls | Function (%d MB lost)||", nLoadingProfilerNotTrackedAllocations);
        pLog->Log("---------------------------------------------------------------------");

        qsort(arrNoStack.GetElements(), arrNoStack.Count(), sizeof(arrNoStack[0]), SLoadingTimeContainer::Cmp_SLoadingTimeContainer_MemUsage);

        for (int i = 0; i < arrNoStack.Count(); i++)
        {
            const SLoadingTimeContainer* pTimeContainer = &arrNoStack[i];
            pLog->Log("|%6.1f | %6.1f | %6d | %s|",
                pTimeContainer->m_dSelfMemUsage, pTimeContainer->m_dTotalMemUsage, (int)pTimeContainer->m_nCounter, pTimeContainer->m_pFuncName);
        }

        pLog->Log("---------------------------------------------------------------------");
    }

    if (nLoadingProfileMode > 0)
    { // loading time stats per func
        pLog->Log("----------- Level loading time (sec) by function --------------------");
        pLog->Log(" ||Self |  Total |  Calls | Function||");
        pLog->Log("---------------------------------------------------------------------");

        qsort(arrNoStack.GetElements(), arrNoStack.Count(), sizeof(arrNoStack[0]), SLoadingTimeContainer::Cmp_SLoadingTimeContainer_Time);

        for (int i = 0; i < arrNoStack.Count(); i++)
        {
            const SLoadingTimeContainer* pTimeContainer = &arrNoStack[i];
            pLog->Log("|%6.1f | %6.1f | %6d | %s|",
                pTimeContainer->m_dSelfTime, pTimeContainer->m_dTotalTime, (int)pTimeContainer->m_nCounter, pTimeContainer->m_pFuncName);
        }

        if (nLoadingProfileMode == 1)
        {
            pLog->Log("----- ( Use sys_ProfileLevelLoading 2 for more detailed stats ) -----");
        }
        else
        {
            pLog->Log("---------------------------------------------------------------------");
        }
    }

    if (nLoadingProfileMode > 0)
    { // file info
        pLog->Log("----------------------------- Level file information by function --------------------------------");
        pLog->Log("||           Self          |           Total         |Bandwith|  Calls | Function||");
        pLog->Log("|| Seeks |FileOpen|FileRead| Seeks |FileOpen|FileRead|  Kb/s  |        |         ||");

        qsort(arrNoStack.GetElements(), arrNoStack.Count(), sizeof(arrNoStack[0]), SLoadingTimeContainer::Cmp_SLoadingTimeContainer_Time);

        for (int i = 0; i < arrNoStack.Count(); i++)
        {
            const SLoadingTimeContainer* pTimeContainer = &arrNoStack[i];
            double bandwidth = pTimeContainer->m_dSelfTime > 0 ? (pTimeContainer->m_selfInfo.m_dOperationSize / pTimeContainer->m_dSelfTime / 1024.0) : 0.;
            pLog->Log("|%6d | %6d | %6d |%6d | %6d | %6d | %6.1f | %6d | %s|",
                pTimeContainer->m_selfInfo.m_nSeeksCount, pTimeContainer->m_selfInfo.m_nFileOpenCount, pTimeContainer->m_selfInfo.m_nFileReadCount,
                pTimeContainer->m_totalInfo.m_nSeeksCount, pTimeContainer->m_totalInfo.m_nFileOpenCount, pTimeContainer->m_totalInfo.m_nFileReadCount,
                bandwidth, (int)pTimeContainer->m_nCounter, pTimeContainer->m_pFuncName);
        }

        if (nLoadingProfileMode == 1)
        {
            pLog->Log("----- ( Use sys_ProfileLevelLoading 2 for more detailed stats ) -----");
        }
        else
        {
            pLog->Log("---------------------------------------------------------------------");
        }
    }
}

struct CSystemEventListner_LoadingProfiler
    : public ISystemEventListener
{
private:
    CLoadingTimeProfiler* m_pPrecacheProfiler;
    ESystemEvent lastEvent;
public:
    CSystemEventListner_LoadingProfiler()
        : m_pPrecacheProfiler(NULL) {}

    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
        case ESYSTEM_EVENT_GAME_MODE_SWITCH_START:
        {
            CLoadingProfilerSystem::Clean();
            if (m_pPrecacheProfiler == NULL)
            {
                m_pPrecacheProfiler = new CLoadingTimeProfiler(gEnv->pSystem, "ModeSwitch");
            }
            break;
        }

        case ESYSTEM_EVENT_GAME_MODE_SWITCH_END:
        {
            SAFE_DELETE(m_pPrecacheProfiler);
            CLoadingProfilerSystem::SaveTimeContainersToFile(gEnv->bMultiplayer == true ? "mode_switch_mp.lmbrlp" : "mode_switch_sp.lmbrlp", 0.0, true);
        }

        case ESYSTEM_EVENT_LEVEL_LOAD_PREPARE:
        {
            CLoadingProfilerSystem::Clean();
            if (m_pPrecacheProfiler == NULL)
            {
                m_pPrecacheProfiler = new CLoadingTimeProfiler(gEnv->pSystem, "LevelLoading");
            }
            break;
        }

        case ESYSTEM_EVENT_LEVEL_LOAD_END:
        {
            delete m_pPrecacheProfiler;
            m_pPrecacheProfiler = new CLoadingTimeProfiler(gEnv->pSystem, "Precache");
            break;
        }
        case ESYSTEM_EVENT_LEVEL_PRECACHE_END:
        {
            if (lastEvent == ESYSTEM_EVENT_LEVEL_PRECACHE_FIRST_FRAME)
            {
                SAFE_DELETE(m_pPrecacheProfiler);
                string levelName = "no_level";
                ICVar* sv_map = gEnv->pConsole->GetCVar("sv_map");
                if (sv_map)
                {
                    levelName = sv_map->GetString();
                }

                string levelNameFullProfile = levelName + "_LP.lmbrlp";
                string levelNameThreshold = levelName + "_LP_OneSec.lmbrlp";
                CLoadingProfilerSystem::SaveTimeContainersToFile(levelNameFullProfile.c_str(), 0.0, false);
                CLoadingProfilerSystem::SaveTimeContainersToFile(levelNameThreshold.c_str(), 1.0, true);
            }
            break;
        }
        case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
        {
            // Ensure that the precache profiler is dead
            SAFE_DELETE(m_pPrecacheProfiler);
            break;
        }
        }

        if (event != ESYSTEM_EVENT_RANDOM_SEED)
        {
            lastEvent = event;
        }
    }
};

static CSystemEventListner_LoadingProfiler g_system_event_listener_loadingProfiler;

void CLoadingProfilerSystem::Init()
{
    gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(&g_system_event_listener_loadingProfiler);
}


//////////////////////////////////////////////////////////////////////////
void CLoadingProfilerSystem::ShutDown()
{
    if (gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
    {
        gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(&g_system_event_listener_loadingProfiler);
    }
}

//////////////////////////////////////////////////////////////////////////
SLoadingTimeContainer* CLoadingProfilerSystem::StartLoadingSectionProfiling(CLoadingTimeProfiler* pProfiler, const char* szFuncName)
{
    if (!nLoadingProfileMode || !gEnv->pConsole)
    {
        return NULL;
    }

    DWORD threadID = GetCurrentThreadId();

    static DWORD dwMainThreadId = GetCurrentThreadId();
    if (threadID != dwMainThreadId)
    {
        return NULL;
    }

    if (!m_pEnableProfile)
    {
        if (gEnv->pConsole)
        {
            m_pEnableProfile = gEnv->pConsole->GetCVar("sys_ProfileLevelLoading");
            if (!m_pEnableProfile)
            {
                return 0;
            }
        }
        else
        {
            return 0;
        }
    }

    if (m_pEnableProfile->GetIVal() <= 0)
    {
        return 0;
    }

    //if (m_pCurrentLoadingTimeContainer == m_pRoot && strstr(szFuncName,"Open"))
    //{
    //  pProfiler->m_constructorInfo.m_nFileOpenCount +=1;
    //}

    CryAutoCriticalSection lock(csLock);

    if (true /*pProfiler && pProfiler->m_pSystem*/)
    {
        ITimer* pTimer = pProfiler->m_pSystem->GetITimer();
        pProfiler->m_fConstructorTime = pTimer->GetAsyncTime().GetSeconds();
        pProfiler->m_fConstructorMemUsage = SLoadingTimeContainer::GetUsedMemory(pProfiler->m_pSystem);

        DiskOperationInfo info;
        pProfiler->m_constructorInfo = info;

        if (nLoadingProfilerNotTrackedAllocations < 0)
        {
            nLoadingProfilerNotTrackedAllocations = (int)pProfiler->m_fConstructorMemUsage;
        }
    }

    SLoadingTimeContainer* pParent = m_pCurrentLoadingTimeContainer;
    if (!pParent)
    {
        pParent = m_pCurrentLoadingTimeContainer = m_pRoot[m_iActiveRoot] = new SLoadingTimeContainer(0, "Root", m_iActiveRoot);
    }

    for (size_t i = 0, end = m_pCurrentLoadingTimeContainer->m_pChilds.size(); i < end; ++i)
    {
        if (m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_pFuncName == szFuncName)
        {
            assert(m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_pParent == m_pCurrentLoadingTimeContainer);
            assert(!m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_bUsed);
            m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_bUsed = true;
            m_pCurrentLoadingTimeContainer->m_pChilds[i]->m_nCounter++;
            m_pCurrentLoadingTimeContainer = m_pCurrentLoadingTimeContainer->m_pChilds[i];
            return m_pCurrentLoadingTimeContainer;
        }
    }

    m_pCurrentLoadingTimeContainer = new SLoadingTimeContainer(pParent, szFuncName, pParent->m_nRootIndex);
    m_pCurrentLoadingTimeContainer->m_bUsed = true;
    {
        // Need to iterate from the end than
        pParent->m_pChilds.push_back(m_pCurrentLoadingTimeContainer);
    }

    return m_pCurrentLoadingTimeContainer;
}

void CLoadingProfilerSystem::EndLoadingSectionProfiling(CLoadingTimeProfiler* pProfiler)
{
    if (!nLoadingProfileMode)
    {
        return;
    }

    static DWORD dwMainThreadId = GetCurrentThreadId();

    if (GetCurrentThreadId() != dwMainThreadId)
    {
        return;
    }

    if (!pProfiler->m_pTimeContainer)
    {
        return;
    }

    CryAutoCriticalSection lock(csLock);

    if (true /*pProfiler && pProfiler->m_pSystem*/)
    {
        ITimer* pTimer = pProfiler->m_pSystem->GetITimer();
        double fSelfTime = pTimer->GetAsyncTime().GetSeconds() - pProfiler->m_fConstructorTime;
        double fMemUsage = SLoadingTimeContainer::GetUsedMemory(pProfiler->m_pSystem);
        double fSelfMemUsage = fMemUsage - pProfiler->m_fConstructorMemUsage;


        if (fSelfTime < 0.0)
        {
            assert(0);
        }
        pProfiler->m_pTimeContainer->m_dSelfTime += fSelfTime;
        pProfiler->m_pTimeContainer->m_dTotalTime += fSelfTime;
        pProfiler->m_pTimeContainer->m_dSelfMemUsage += fSelfMemUsage;
        pProfiler->m_pTimeContainer->m_dTotalMemUsage += fSelfMemUsage;

        DiskOperationInfo info;
        info -= pProfiler->m_constructorInfo;
        pProfiler->m_pTimeContainer->m_totalInfo += info;
        pProfiler->m_pTimeContainer->m_selfInfo += info;
        pProfiler->m_pTimeContainer->m_bUsed = false;

        SLoadingTimeContainer* pParent = pProfiler->m_pTimeContainer->m_pParent;
        pParent->m_selfInfo -= info;
        pParent->m_dSelfTime -= fSelfTime;
        pParent->m_dSelfMemUsage -= fSelfMemUsage;
        if (pProfiler->m_pTimeContainer->m_pParent && pProfiler->m_pTimeContainer->m_pParent->m_nRootIndex == m_iActiveRoot)
        {
            m_pCurrentLoadingTimeContainer = pProfiler->m_pTimeContainer->m_pParent;
        }
    }
}

const char* CLoadingProfilerSystem::GetLoadingProfilerCallstack()
{
    CryAutoCriticalSection lock(csLock);

    static char szStack[1024];

    szStack[0] = 0;

    SLoadingTimeContainer* pC = m_pCurrentLoadingTimeContainer;

    PodArray<SLoadingTimeContainer*> arrItems;

    while (pC)
    {
        arrItems.Add(pC);
        pC = pC->m_pParent;
    }

    for (int i = arrItems.Count() - 1; i >= 0; i--)
    {
        cry_strcat(szStack, " > ");
        cry_strcat(szStack, arrItems[i]->m_pFuncName);
    }

    return &szStack[0];
}

void CLoadingProfilerSystem::FillProfilersList(AZStd::vector<SLoadingProfilerInfo>& profilers)
{
    UpdateSelfStatistics(m_pRoot[m_iActiveRoot]);

    PodArray<SLoadingTimeContainer> arrNoStack;
    CreateNoStackList(arrNoStack);
    //qsort(arrNoStack.GetElements(), arrNoStack.Count(), sizeof(arrNoStack[0]), SLoadingTimeContainer::Cmp_SLoadingTimeContainer_Time);

    uint32 count = arrNoStack.Size();
    profilers.resize(count);

    for (uint32 i = 0; i < count; ++i)
    {
        profilers[i].name = arrNoStack[i].m_pFuncName;
        profilers[i].selfTime = arrNoStack[i].m_dSelfTime;
        profilers[i].callsTotal = arrNoStack[i].m_nCounter;
        profilers[i].totalTime = arrNoStack[i].m_dTotalTime;
        profilers[i].memorySize = arrNoStack[i].m_dTotalMemUsage;
        profilers[i].selfInfo = arrNoStack[i].m_selfInfo;
        profilers[i].totalInfo = arrNoStack[i].m_totalInfo;
    }
}


void CLoadingProfilerSystem::AddTimeContainerFunction(PodArray<SLoadingTimeContainer>& arrNoStack, SLoadingTimeContainer* node)
{
    if (!node)
    {
        return;
    }

    SLoadingTimeContainer* it = std::find(arrNoStack.begin(), arrNoStack.end(), node->m_pFuncName);

    if (it ==  arrNoStack.end())
    {
        arrNoStack.push_back(*node);
    }
    else
    {
        it->m_dSelfMemUsage += node->m_dSelfMemUsage;
        it->m_dSelfTime += node->m_dSelfTime;
        it->m_dTotalMemUsage += node->m_dTotalMemUsage;
        it->m_dTotalTime += node->m_dTotalTime;
        it->m_nCounter += node->m_nCounter;
        it->m_selfInfo += node->m_selfInfo;
        it->m_totalInfo += node->m_totalInfo;
    }

    for (size_t i = 0, end = node->m_pChilds.size(); i < end; ++i)
    {
        AddTimeContainerFunction(arrNoStack, node->m_pChilds[i]);
    }
}

void CLoadingProfilerSystem::CreateNoStackList(PodArray<SLoadingTimeContainer>& arrNoStack)
{
    AddTimeContainerFunction(arrNoStack, m_pRoot[m_iActiveRoot]);
}

#define g_szTestResults "@cache@\\TestResults"

void CLoadingProfilerSystem::SaveTimeContainersToFile(const char* name, double fMinTotalTime, bool bClean)
{
    if (m_pRoot[m_iActiveRoot])
    {
        const char* levelName = name;
        //Ignore any folders in the input name
        const char* folder = strrchr(name, '/');
        if (folder != NULL)
        {
            levelName = folder + 1;
        }
        char path[AZ::IO::IArchive::MaxPath];
        path[sizeof(path) - 1] = 0;

        gEnv->pCryPak->AdjustFileName(string(string(g_szTestResults) + "\\" + levelName).c_str(), path, AZ_ARRAY_SIZE(path), AZ::IO::IArchive::FLAGS_PATH_REAL | AZ::IO::IArchive::FLAGS_FOR_WRITING);
        gEnv->pCryPak->MakeDir(g_szTestResults);

        AZ::IO::HandleType handle = AZ::IO::InvalidHandle;

        AZ::IO::Result f = AZ::IO::FileIOBase::GetInstance()->Open(path,AZ::IO::OpenMode::ModeWrite, handle);

        if (handle != AZ::IO::InvalidHandle)
        {
            UpdateSelfStatistics(m_pRoot[m_iActiveRoot]);
            WriteTimeContainerToFile(m_pRoot[m_iActiveRoot], handle, 0, fMinTotalTime);
            AZ::IO::FileIOBase::GetInstance()->Close(handle);
        }

        if (bClean)
        {
            Clean();
        }
    }
}

void CLoadingProfilerSystem::WriteTimeContainerToFile(SLoadingTimeContainer* p, AZ::IO::HandleType &handle, unsigned int depth, double fMinTotalTime)
{
    if (p == NULL)
    {
        return;
    }

    if (p->m_dTotalTime < fMinTotalTime)
    {
        return;
    }

    CryFixedStringT<MAX_LOADING_TIME_PROFILER_STACK_DEPTH> sDepth;
    for (unsigned int i = 0; i < depth; i++)
    {
        sDepth += "\t";
    }

    CryFixedStringT<128> str(p->m_pFuncName);
    str.replace(':', '_');

    char data[4096];
    AZ::u64 bytesWritten;

    azsnprintf(data, sizeof(data), "%s<%s selfTime='%f' selfMemory='%f' totalTime='%f' totalMemory='%f' count='%i' totalSeeks='%i' totalReads='%i' totalOpens='%i' totalDiskSize='%f' selfSeeks='%i' selfReads='%i' selfOpens='%i' selfDiskSize='%f'>\n",
                                    sDepth.c_str(), str.c_str(), p->m_dSelfTime, p->m_dSelfMemUsage, p->m_dTotalTime, p->m_dTotalMemUsage, p->m_nCounter,
                                    p->m_totalInfo.m_nSeeksCount, p->m_totalInfo.m_nFileReadCount, p->m_totalInfo.m_nFileOpenCount, p->m_totalInfo.m_dOperationSize,
                                    p->m_selfInfo.m_nSeeksCount, p->m_selfInfo.m_nFileReadCount, p->m_selfInfo.m_nFileOpenCount, p->m_selfInfo.m_dOperationSize);

    AZ::IO::FileIOBase::GetInstance()->Write(handle, data, strlen(data), &bytesWritten);

    for (size_t i = 0, end = p->m_pChilds.size(); i < end; ++i)
    {
        WriteTimeContainerToFile(p->m_pChilds[i], handle, depth + 1, fMinTotalTime);
    }

    azsnprintf(data, sizeof(data), "%s</%s>\n", sDepth.c_str(), str.c_str());
    AZ::IO::FileIOBase::GetInstance()->Write(handle, data, strlen(data), &bytesWritten);

}

void CLoadingProfilerSystem::UpdateSelfStatistics(SLoadingTimeContainer* p)
{
    if (p == NULL)
    {
        return;
    }

    p->m_dSelfMemUsage = 0;
    p->m_dSelfTime = 0;
    p->m_nCounter = 1;
    p->m_selfInfo.m_dOperationSize = 0;
    p->m_selfInfo.m_nFileOpenCount = 0;
    p->m_selfInfo.m_nFileReadCount = 0;
    p->m_selfInfo.m_nSeeksCount = 0;

    for (size_t i = 0, end = p->m_pChilds.size(); i < end; ++i)
    {
        p->m_dTotalMemUsage += p->m_pChilds[i]->m_dTotalMemUsage;
        p->m_dTotalTime += p->m_pChilds[i]->m_dTotalTime;
        p->m_totalInfo += p->m_pChilds[i]->m_totalInfo;
    }
}

void CLoadingProfilerSystem::Clean()
{
    m_iActiveRoot = (m_iActiveRoot + 1) % 2;
    if (m_pRoot[m_iActiveRoot])
    {
        delete m_pRoot[m_iActiveRoot];
    }
    m_pCurrentLoadingTimeContainer = m_pRoot[m_iActiveRoot] = 0;
}

#endif
