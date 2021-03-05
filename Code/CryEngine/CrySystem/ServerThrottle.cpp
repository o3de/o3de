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
#include "ServerThrottle.h"
#include "TimeValue.h"
#include "ISystem.h"
#include "ITimer.h"
#include "IConsole.h"

#if defined(WIN32)
static float ftdiff(const FILETIME& b, const FILETIME& a)
{
    uint64 aa = *reinterpret_cast<const uint64*>(&a);
    uint64 bb = *reinterpret_cast<const uint64*>(&b);
    return (bb - aa) * 1e-7f;
}

class CCPUMonitor
{
public:
    CCPUMonitor(ISystem* pSystem, int nCPUs)
        : m_lastUpdate(0.0f)
        , m_pTimer(pSystem->GetITimer())
        , m_nCPUs(nCPUs)
    {
        FILETIME notNeeded;
        GetProcessTimes(GetCurrentProcess(), &notNeeded, &notNeeded, &m_lastKernel, &m_lastUser);
    }

    float* Update()
    {
        CTimeValue frameTime = gEnv->pTimer->GetFrameStartTime();
        if (frameTime - m_lastUpdate > 5.0f)
        {
            m_lastUpdate = frameTime;

            static float result = 0.0f;

            FILETIME kernel, user, cur;
            FILETIME notNeeded;
            GetSystemTimeAsFileTime(&cur);
            GetProcessTimes(GetCurrentProcess(), &notNeeded, &notNeeded, &kernel, &user);

            float sKernel = ftdiff(kernel, m_lastKernel);
            float sUser = ftdiff(user, m_lastUser);
            float sCur = ftdiff(cur, m_lastTime);

            result = 100 * (sKernel + sUser) / sCur / m_nCPUs;

            m_lastTime = cur;
            m_lastKernel = kernel;
            m_lastUser = user;

            return &result;
        }
        return 0;
    }

private:
    ITimer* m_pTimer;
    CTimeValue m_lastUpdate;
    FILETIME m_lastKernel, m_lastUser, m_lastTime;
    int m_nCPUs;
};
#else
class CCPUMonitor
{
public:
    CCPUMonitor(ISystem*, int) {}

    float* Update() { return 0; }
};
#endif

CServerThrottle::CServerThrottle(ISystem* pSys, int nCPUs)
{
    m_pCPUMonitor.reset(new CCPUMonitor(pSys, nCPUs));
    m_pDedicatedMaxRate = pSys->GetIConsole()->GetCVar("sv_DedicatedMaxRate");
    m_pDedicatedCPU = pSys->GetIConsole()->GetCVar("sv_DedicatedCPUPercent");
    m_pDedicatedCPUVariance = pSys->GetIConsole()->GetCVar("sv_DedicatedCPUVariance");

    m_minFPS = 20;
    m_maxFPS = 60;
    m_nSteps = 8;
    m_nCurStep = 0;

    if (m_pDedicatedCPU->GetFVal() >= 1.0f)
    {
        SetStep(m_nSteps / 2, 0);
    }
}

CServerThrottle::~CServerThrottle()
{
}

void CServerThrottle::Update()
{
    float tgtCPU = m_pDedicatedCPU->GetFVal();
    if (tgtCPU < 1)
    {
        return;
    }
    if (float* pCPU = m_pCPUMonitor->Update())
    {
        float varCPU = m_pDedicatedCPUVariance->GetFVal();
        if (tgtCPU < 5)
        {
            tgtCPU = 5;
        }
        else if (tgtCPU > 95)
        {
            tgtCPU = 95;
        }
        float minCPU = std::max(tgtCPU - varCPU, tgtCPU / 2.0f);
        float maxCPU = std::min(tgtCPU + varCPU, (100.0f + tgtCPU) / 2.0f);

        if (*pCPU > maxCPU)
        {
            SetStep(m_nCurStep - 1, pCPU);
        }
        else if (*pCPU < minCPU)
        {
            SetStep(m_nCurStep + 1, pCPU);
        }
    }
}

void CServerThrottle::SetStep(int step, float* pDueToCPU)
{
    if (step < 0)
    {
        step = 0;
    }
    else if (step > m_nSteps)
    {
        step = m_nSteps;
    }
    if (step != m_nCurStep)
    {
        float fps = step * (m_maxFPS - m_minFPS) / m_nSteps + m_minFPS;
        m_pDedicatedMaxRate->Set(fps);
        if (pDueToCPU)
        {
            CryLog("ServerThrottle: Set framerate to %.1f fps [due to cpu being %d%%]", fps, int(*pDueToCPU + 0.5f));
        }
        else
        {
            CryLog("ServerThrottle: Set framerate to %.1f fps", fps);
        }
        m_nCurStep = step;
    }
}
