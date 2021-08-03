/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
