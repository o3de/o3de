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

#ifndef CRYINCLUDE_CRYSYSTEM_LOADINGPROFILER_H
#define CRYINCLUDE_CRYSYSTEM_LOADINGPROFILER_H
#pragma once


#if defined(ENABLE_LOADING_PROFILER)

struct SLoadingTimeContainer;


struct SLoadingProfilerInfo
{
    string name;
    double selfTime;
    double totalTime;
    uint32 callsTotal;
    double memorySize;

    DiskOperationInfo selfInfo;
    DiskOperationInfo totalInfo;
};


class CLoadingProfilerSystem
{
public:
    static void Init();
    static void ShutDown();
    static void CreateNoStackList(PodArray<SLoadingTimeContainer>&);
    static void OutputLoadingTimeStats(ILog* pLog, int nMode);
    static SLoadingTimeContainer* StartLoadingSectionProfiling(CLoadingTimeProfiler* pProfiler, const char* szFuncName);
    static void EndLoadingSectionProfiling(CLoadingTimeProfiler* pProfiler);
    static const char* GetLoadingProfilerCallstack();
    static void FillProfilersList(AZStd::vector<SLoadingProfilerInfo>& profilers);
    static void FlushTimeContainers();
    static void SaveTimeContainersToFile(const char*, double fMinTotalTime, bool bClean);
    static void WriteTimeContainerToFile(SLoadingTimeContainer* p, AZ::IO::HandleType &handle, unsigned int depth, double fMinTotalTime);

    static void UpdateSelfStatistics(SLoadingTimeContainer* p);
    static void Clean();
protected:
    static void AddTimeContainerFunction(PodArray<SLoadingTimeContainer>&, SLoadingTimeContainer*);
protected:
    static int nLoadingProfileMode;
    static int nLoadingProfilerNotTrackedAllocations;
    static CryCriticalSection csLock;
    static int m_iMaxArraySize;
    static SLoadingTimeContainer* m_pCurrentLoadingTimeContainer;
    static SLoadingTimeContainer* m_pRoot[2];
    static int m_iActiveRoot;
    static ICVar* m_pEnableProfile;
};

#endif

#endif // CRYINCLUDE_CRYSYSTEM_LOADINGPROFILER_H
