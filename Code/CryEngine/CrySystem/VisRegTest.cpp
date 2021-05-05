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


#include "CrySystem_precompiled.h"
#include "VisRegTest.h"

#include "ISystem.h"
#include "IRenderer.h"
#include "IConsole.h"
#include "ITimer.h"
#include "IStreamEngine.h"
#include <Pak/CryPakUtils.h>

#include <AzFramework/IO/FileOperations.h>

const float CVisRegTest::MaxStreamingWait = 30.0f;

CVisRegTest::CVisRegTest()
    : m_nextCmd(0)
    , m_waitFrames(0)
{
    CryLog("Enabled visual regression tests");
}


void CVisRegTest::Init(IConsoleCmdArgs* pParams)
{
    assert(pParams);

    stack_string configFile("visregtest.xml");

    // Reset
    m_cmdBuf.clear();
    m_dataSamples.clear();
    m_nextCmd = 0;
    m_cmdFreq = 0;
    m_waitFrames = 0;
    m_streamingTimeout = 0.0f;
    m_testName = "test";
    m_quitAfterTests = false;

    // Parse arguments
    if (pParams->GetArgCount() >= 2)
    {
        m_testName = pParams->GetArg(1);
    }
    if (pParams->GetArgCount() >= 3)
    {
        configFile = pParams->GetArg(2);
    }
    if (pParams->GetArgCount() >= 4)
    {
        if (_stricmp(pParams->GetArg(3), "quit") == 0)
        {
            m_quitAfterTests = true;
        }
    }

    // Fill cmd buffer
    if (!LoadConfig(configFile.c_str()))
    {
        CryWarning(VALIDATOR_MODULE_SYSTEM, VALIDATOR_ERROR, "VisRegTest: Failed to load config file '%s' from game folder", configFile.c_str());
        return;
    }

    gEnv->pTimer->SetTimeScale(0);
    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RANDOM_SEED, 0, 0);
    srand(0);

    gEnv->pRenderer->EnableGPUTimers2(true);
}


void CVisRegTest::AfterRender()
{
    ExecCommands();
}


bool CVisRegTest::LoadConfig(const char* configFile)
{
    XmlNodeRef rootNode = GetISystem()->LoadXmlFromFile(configFile);
    if (!rootNode || !rootNode->isTag("VisRegTest"))
    {
        return false;
    }

    m_cmdBuf.push_back(SCmd(eCMDStart, ""));

    for (int i = 0; i < rootNode->getChildCount(); ++i)
    {
        XmlNodeRef node = rootNode->getChild(i);

        if (node->isTag("Config"))
        {
            SCmd cmd;
            for (int j = 0; j < node->getChildCount(); ++j)
            {
                XmlNodeRef node2 = node->getChild(j);

                if (node2->isTag("ConsoleCmd"))
                {
                    m_cmdBuf.push_back(SCmd(eCMDConsoleCmd, node2->getAttr("cmd")));
                }
            }
        }
        else if (node->isTag("Map"))
        {
            const char* mapName = node->getAttr("name");
            int imageIndex = 0;

            m_cmdBuf.push_back(SCmd(eCMDLoadMap, mapName));
            m_cmdBuf.push_back(SCmd(eCMDOnMapLoaded, ""));

            for (int j = 0; j < node->getChildCount(); ++j)
            {
                XmlNodeRef node2 = node->getChild(j);

                if (node2->isTag("ConsoleCmd"))
                {
                    m_cmdBuf.push_back(SCmd(eCMDConsoleCmd, node2->getAttr("cmd")));
                }
                else if (node2->isTag("Sample"))
                {
                    m_cmdBuf.push_back(SCmd(eCMDGoto, node2->getAttr("location")));
                    m_cmdBuf.push_back(SCmd(eCMDWaitStreaming, ""));

                    char strbuf[256];
#ifdef WIN32
                    sprintf_s(strbuf, "%s_%i.bmp", mapName, imageIndex++);
#else
                    sprintf_s(strbuf, "%s_%i.tga", mapName, imageIndex++);
#endif
                    m_cmdBuf.push_back(SCmd(eCMDCaptureSample, strbuf, SampleCount));
                }
            }
        }
    }

    m_cmdBuf.push_back(SCmd(eCMDFinish, ""));

    return true;
}


void CVisRegTest::ExecCommands()
{
    if (m_nextCmd >= m_cmdBuf.size())
    {
        return;
    }

    float col[] = {0, 1, 0, 1};
    gEnv->pRenderer->Draw2dLabel(10, 10, 2, col, false, "Visual Regression Test");

    if (m_waitFrames > 0)
    {
        --m_waitFrames;
        return;
    }
    else if (m_waitFrames < 0)   // Wait for streaming
    {
        SStreamEngineOpenStats stats;
        gEnv->pSystem->GetStreamEngine()->GetStreamingOpenStatistics(stats);
        if (stats.nOpenRequestCount > 0 && m_streamingTimeout > 0)
        {
            gEnv->pConsole->ExecuteString("t_FixedStep 0");
            m_streamingTimeout -= gEnv->pTimer->GetRealFrameTime();
            m_waitFrames = -16;
        }
        else if (++m_waitFrames == 0)
        {
            gEnv->pConsole->ExecuteString("t_FixedStep 0.033333");
            m_waitFrames = 64;  // Wait some more frames for tone-mapper to adapt
        }

        return;
    }

    stack_string tmp;

    while (m_nextCmd < m_cmdBuf.size())
    {
        SCmd& cmd = m_cmdBuf[m_nextCmd];

        if (m_cmdFreq == 0)
        {
            m_cmdFreq = cmd.freq;
        }

        switch (cmd.cmd)
        {
        case eCMDStart:
            break;
        case eCMDFinish:
            Finish();
            break;
        case eCMDOnMapLoaded:
            gEnv->pTimer->SetTimer(ITimer::ETIMER_GAME, 0);
            gEnv->pTimer->SetTimer(ITimer::ETIMER_UI, 0);
            gEnv->pConsole->ExecuteString("t_FixedStep 0.033333");
            GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RANDOM_SEED, 0, 0);
            srand(0);
            break;
        case eCMDConsoleCmd:
            gEnv->pConsole->ExecuteString(cmd.args.c_str());
            break;
        case eCMDLoadMap:
            LoadMap(cmd.args.c_str());
            break;
        case eCMDWaitStreaming:
            m_waitFrames = -1;
            m_streamingTimeout = MaxStreamingWait;
            break;
        case eCMDWaitFrames:
            m_waitFrames = (uint32)atoi(cmd.args.c_str());
            break;
        case eCMDGoto:
            tmp = "playerGoto ";
            tmp.append(cmd.args.c_str());
            gEnv->pConsole->ExecuteString(tmp.c_str());
            m_waitFrames = 1;
            break;
        case eCMDCaptureSample:
            CaptureSample(cmd);
            break;
        }

        if (m_cmdFreq-- == 1)
        {
            ++m_nextCmd;
        }

        if (m_waitFrames != 0)
        {
            break;
        }
    }
}


void CVisRegTest::LoadMap(const char* mapName)
{
    string mapCmd("map ");
    mapCmd.append(mapName);
    gEnv->pConsole->ExecuteString(mapCmd.c_str());

    gEnv->pTimer->SetTimer(ITimer::ETIMER_GAME, 0);
    gEnv->pTimer->SetTimer(ITimer::ETIMER_UI, 0);
    gEnv->pConsole->ExecuteString("t_FixedStep 0");
    GetISystem()->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_RANDOM_SEED, 0, 0);
    srand(0);
}


void CVisRegTest::CaptureSample(const SCmd& cmd)
{
    if (m_cmdFreq == SampleCount)   // First sample
    {
        m_dataSamples.push_back(SSample());
        gEnv->pConsole->ExecuteString("t_FixedStep 0");
    }

    SSample& sample = m_dataSamples.back();

    // Collect stats
    sample.imageName = cmd.args.c_str();
    sample.frameTime += gEnv->pTimer->GetRealFrameTime() * 1000.f;
    sample.drawCalls += gEnv->pRenderer->GetCurrentNumberOfDrawCalls();

    const RPProfilerStats* pRPPStats = gEnv->pRenderer->GetRPPStatsArray();

    if (pRPPStats)
    {
        sample.gpuTimes[0] += pRPPStats[eRPPSTATS_OverallFrame].gpuTime;

        sample.gpuTimes[1] += pRPPStats[eRPPSTATS_SceneOverall].gpuTime;
        sample.gpuTimes[2] += pRPPStats[eRPPSTATS_ShadowsOverall].gpuTime;
        sample.gpuTimes[3] += pRPPStats[eRPPSTATS_LightingOverall].gpuTime;
        sample.gpuTimes[4] += pRPPStats[eRPPSTATS_VfxOverall].gpuTime;
    }

    if (m_cmdFreq == 1)   // Final sample
    {
        // Screenshot
        stack_string filename("@usercache@/TestResults/VisReg/");   // the default unaliased assets folder is read-only!
        filename += m_testName + "/" + cmd.args.c_str();
        gEnv->pRenderer->ScreenShot(filename);

        // Average results
        sample.frameTime /= (float)SampleCount;
        sample.drawCalls /= SampleCount;
        for (uint32 i = 0; i < MaxNumGPUTimes; ++i)
        {
            sample.gpuTimes[i] /= (float)SampleCount;
        }

        gEnv->pConsole->ExecuteString("t_FixedStep 0.033333");
    }
}


void CVisRegTest::Finish()
{
    WriteResults();

    gEnv->pConsole->ExecuteString("t_FixedStep 0");
    gEnv->pTimer->SetTimeScale(1);

    gEnv->pRenderer->EnableGPUTimers2(false);

    CryLog("VisRegTest: Finished tests");

    if (m_quitAfterTests)
    {
        gEnv->pConsole->ExecuteString("quit");
        return;
    }
}


bool CVisRegTest::WriteResults()
{
    stack_string filename("@usercache@/TestResults/VisReg/");
    filename += m_testName + "/visreg_results.xml";

    AZ::IO::HandleType fileHandle = fxopen(filename.c_str(), "wb");
    if (fileHandle == AZ::IO::InvalidHandle)
    {
        return false;
    }

    AZ::IO::Print(fileHandle, "<testsuites>\n");
    AZ::IO::Print(fileHandle, "\t<testsuite name=\"Visual Regression Test\" LevelName=\"\">\n");

    for (int i = 0; i < (int)m_dataSamples.size(); ++i)
    {
        SSample sample = m_dataSamples[i];

        AZ::IO::Print(fileHandle, "\t\t<phase name=\"%i\" duration=\"1\" image=\"%s\">\n", i, sample.imageName);

        AZ::IO::Print(fileHandle, "\t\t\t<metrics name=\"general\">\n");
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"frameTime\" value=\"%f\" />\n", sample.frameTime);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"drawCalls\" value=\"%i\" />\n", sample.drawCalls);
        AZ::IO::Print(fileHandle, "\t\t\t</metrics>\n");

        AZ::IO::Print(fileHandle, "\t\t\t<metrics name=\"gpu_times\">\n");
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"scene\" value=\"%f\" />\n", sample.gpuTimes[0]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"shadows\" value=\"%f\" />\n", sample.gpuTimes[1]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"zpass\" value=\"%f\" />\n", sample.gpuTimes[2]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"deferred_decals\" value=\"%f\" />\n", sample.gpuTimes[3]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"deferred_lighting\" value=\"%f\" />\n", sample.gpuTimes[4]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"dl_ambient\" value=\"%f\" />\n", sample.gpuTimes[5]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"dl_cubemaps\" value=\"%f\" />\n", sample.gpuTimes[6]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"dl_lights\" value=\"%f\" />\n", sample.gpuTimes[7]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"gi_and_ssao\" value=\"%f\" />\n", sample.gpuTimes[8]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"opaque\" value=\"%f\" />\n", sample.gpuTimes[9]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"transparent\" value=\"%f\" />\n", sample.gpuTimes[10]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"hdr\" value=\"%f\" />\n", sample.gpuTimes[11]);
        AZ::IO::Print(fileHandle, "\t\t\t\t<metric name=\"postfx\" value=\"%f\" />\n", sample.gpuTimes[12]);
        AZ::IO::Print(fileHandle, "\t\t\t</metrics>\n");

        AZ::IO::Print(fileHandle, "\t\t</phase>\n");
    }

    AZ::IO::Print(fileHandle, "\t</testsuite>\n");
    AZ::IO::Print(fileHandle, "</testsuites>");
    gEnv->pFileIO->Close(fileHandle);

    return true;
}
