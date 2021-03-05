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

#ifndef CRYINCLUDE_CRYSYSTEM_SYSTEMSCHEDULER_H
#define CRYINCLUDE_CRYSYSTEM_SYSTEMSCHEDULER_H

#pragma once

#include "System.h"
#include <ISystemScheduler.h>

class CSystemScheduler
    : public ISystemScheduler
{
public:
    CSystemScheduler(CSystem* pSystem);
    virtual ~CSystemScheduler(void);

    // ISystemScheduler
    virtual void SliceAndSleep(const char* sliceName, int line);
    virtual void SliceLoadingBegin();
    virtual void SliceLoadingEnd();

    virtual void SchedulingSleepIfNeeded(void);
    // ~ISystemScheduler

protected:
    void SchedulingModeUpdate(void);

private:
    CSystem* m_pSystem;
    ICVar* m_svSchedulingAffinity;
    ICVar* m_svSchedulingClientTimeout;
    ICVar* m_svSchedulingServerTimeout;
    ICVar* m_svSchedulingBucket;
    ICVar* m_svSchedulingMode;
    ICVar* m_svSliceLoadEnable;
    ICVar* m_svSliceLoadBudget;
    ICVar* m_svSliceLoadLogging;

    CTimeValue m_lastSliceCheckTime;

    int m_sliceLoadingRef;

    const char* m_pLastSliceName;
    int         m_lastSliceLine;
};

// Summary:
//   Creates the system scheduler interface.
void CreateSystemScheduler(CSystem* pSystem);

#endif // CRYINCLUDE_CRYSYSTEM_SYSTEMSCHEDULER_H
