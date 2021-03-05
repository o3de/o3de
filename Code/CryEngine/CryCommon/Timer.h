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

#pragma once
#ifndef CRYINCLUDE_CRYCOMMON_TIMER_H
#define CRYINCLUDE_CRYCOMMON_TIMER_H

struct Timer
{
    Timer()
        : endTime(-1.0f)
    {
    }

    void Reset(float duration, float variation = 0.0f)
    {
        endTime = gEnv->pSystem->GetITimer()->GetFrameStartTime() + CTimeValue(duration) + CTimeValue(cry_random(0.0f, variation));
    }

    bool Elapsed() const
    {
        return endTime >= 0.0f && gEnv->pSystem->GetITimer()->GetFrameStartTime() >= endTime;
    }

    float GetSecondsLeft() const
    {
        return (endTime - gEnv->pSystem->GetITimer()->GetFrameStartTime()).GetSeconds();
    }

    CTimeValue endTime;
};
#endif // CRYINCLUDE_CRYCOMMON_TIMER_H
