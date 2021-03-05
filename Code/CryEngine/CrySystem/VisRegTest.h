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

// Description : Visual Regression Test


#ifndef CRYINCLUDE_CRYSYSTEM_VISREGTEST_H
#define CRYINCLUDE_CRYSYSTEM_VISREGTEST_H
#pragma once


struct IConsoleCmdArgs;

class CVisRegTest
{
public:
    static const uint32 SampleCount = 16;
    static const uint32 MaxNumGPUTimes = 13;
    static const float MaxStreamingWait;

protected:

    enum ECmd
    {
        eCMDStart,
        eCMDFinish,
        eCMDOnMapLoaded,
        eCMDConsoleCmd,
        eCMDLoadMap,
        eCMDWaitStreaming,
        eCMDWaitFrames,
        eCMDGoto,
        eCMDCaptureSample
    };

    struct SCmd
    {
        ECmd    cmd;
        uint32  freq;
        string  args;

        SCmd() {}
        SCmd(ECmd _cmd, const string& _args, uint32 _freq = 1)
        { this->cmd = _cmd; this->args = _args; this->freq = _freq; }
    };

    struct SSample
    {
        const char* imageName;
        float       frameTime;
        uint32      drawCalls;
        float       gpuTimes[MaxNumGPUTimes];

        SSample()
            : imageName(NULL)
            , frameTime(0.f)
            , drawCalls(0)
        {
            for (uint32 i = 0; i < MaxNumGPUTimes; ++i)
            {
                gpuTimes[i] = 0;
            }
        }
    };

protected:
    string                  m_testName;
    std::vector< SCmd >     m_cmdBuf;
    std::vector< SSample >  m_dataSamples;
    uint32                  m_nextCmd;
    uint32                  m_cmdFreq;
    int                     m_waitFrames;
    float                                       m_streamingTimeout;
    int                     m_curSample;
    bool                    m_quitAfterTests;

public:

    CVisRegTest();

    void Init(IConsoleCmdArgs* pParams);
    void AfterRender();

protected:

    bool LoadConfig(const char* configFile);
    void ExecCommands();
    void LoadMap(const char* mapName);
    void CaptureSample(const SCmd& cmd);
    void Finish();
    bool WriteResults();
};

#endif // CRYINCLUDE_CRYSYSTEM_VISREGTEST_H
