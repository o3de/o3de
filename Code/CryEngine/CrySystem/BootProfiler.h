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

#ifndef CRYINCLUDE_CRYSYSTEM_BOOTPROFILER_H
#define CRYINCLUDE_CRYSYSTEM_BOOTPROFILER_H
#pragma once

#if defined(ENABLE_LOADING_PROFILER)

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>

class CBootProfilerRecord;
class CBootProfilerSession;

class CBootProfiler
    : public ISystemEventListener
{
    friend class CBootProfileBLock;
public:
    CBootProfiler();
    ~CBootProfiler();

    static CBootProfiler& GetInstance();

    void Init(ISystem* pSystem);
    void RegisterCVars();

    void StartSession(const char* sessionName);
    void StopSession(const char* sessionName);

    CBootProfilerRecord* StartBlock(const char* name, const char* args);
    void StopBlock(CBootProfilerRecord* record);

    void StartFrame(const char* name);
    void StopFrame();
protected:
    // === ISystemEventListener
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    void SetFrameCount(int frameCount);

private:
    CBootProfilerSession* m_pCurrentSession;
    typedef AZStd::unordered_map<AZStd::string, CBootProfilerSession*> TSessionMap;
    TSessionMap m_sessions;

    static int                      CV_sys_bp_frames;
    static float                    CV_sys_bp_time_threshold;
    CBootProfilerRecord*    m_pFrameRecord;
    AZStd::recursive_mutex            m_recordMutex;
    int m_levelLoadAdditionalFrames;
};

#endif

#endif // CRYINCLUDE_CRYSYSTEM_BOOTPROFILER_H
