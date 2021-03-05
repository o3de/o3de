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

// Description : Handle raising/lowering the frame rate on server
//               based upon CPU usage


#ifndef CRYINCLUDE_CRYSYSTEM_SERVERTHROTTLE_H
#define CRYINCLUDE_CRYSYSTEM_SERVERTHROTTLE_H
#pragma once


struct ISystem;

class CCPUMonitor;

class CServerThrottle
{
public:
    CServerThrottle(ISystem* pSys, int nCPUs);
    ~CServerThrottle();
    void Update();

private:
    std::unique_ptr<CCPUMonitor> m_pCPUMonitor;

    void SetStep(int step, float* dueToCPU);

    float m_minFPS;
    float m_maxFPS;
    int m_nSteps;
    int m_nCurStep;
    ICVar* m_pDedicatedMaxRate;
    ICVar* m_pDedicatedCPU;
    ICVar* m_pDedicatedCPUVariance;
};

#endif // CRYINCLUDE_CRYSYSTEM_SERVERTHROTTLE_H
