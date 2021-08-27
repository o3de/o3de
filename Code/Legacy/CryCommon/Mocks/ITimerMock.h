/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef CRYINCLUDE_CRYSYSTEM_ITIMERMOCK_H
#define CRYINCLUDE_CRYSYSTEM_ITIMERMOCK_H
#pragma once

#include <ISerialize.h>
#include <ITimer.h>
#include <AzTest/AzTest.h>

// Implements all common timing routines
class TimerMock
    : public ITimer
{
public:
    MOCK_METHOD0(ResetTimer, void());
    MOCK_METHOD0(UpdateOnFrameStart, void());
    MOCK_CONST_METHOD1(GetCurrTime, float(ETimer which));
    MOCK_CONST_METHOD0(GetAsyncTime, CTimeValue());
    MOCK_METHOD0(GetAsyncCurTime, float());
    MOCK_CONST_METHOD1(GetFrameTime, float(ETimer which));
    MOCK_CONST_METHOD0(GetRealFrameTime, float());
    MOCK_CONST_METHOD0(GetTimeScale, float());
    MOCK_CONST_METHOD1(GetTimeScale, float(uint32 channel));
    MOCK_METHOD2(SetTimeScale, void(float scale, uint32 channel));
    MOCK_METHOD0(ClearTimeScales, void());
    MOCK_METHOD1(EnableTimer, void(bool bEnable));
    MOCK_METHOD0(GetFrameRate, float());
    MOCK_METHOD2(GetProfileFrameBlending, float(float* pfBlendTime, int* piBlendMode));
    MOCK_METHOD1(Serialize, void(TSerialize ser));
    MOCK_CONST_METHOD0(IsTimerEnabled, bool());
    MOCK_METHOD2(PauseTimer, bool(ETimer which, bool bPause));
    MOCK_METHOD1(IsTimerPaused, bool(ETimer which));
    MOCK_METHOD2(SetTimer, bool(ETimer which, float timeInSeconds));
    MOCK_METHOD2(SecondsToDateUTC, void(time_t time, struct tm& outDateUTC));
    MOCK_METHOD1(DateToSecondsUTC, time_t(struct tm& timePtr));
    MOCK_METHOD1(TicksToSeconds, float(int64 ticks));
    MOCK_METHOD0(GetTicksPerSecond, int64());

    MOCK_CONST_METHOD1(GetFrameStartTime, const CTimeValue&(ETimer which));
    MOCK_METHOD0(CreateNewTimer, ITimer * ());

    MOCK_METHOD2(EnableFixedTimeMode, void(bool enable, float timeStep));
};

#endif // CRYINCLUDE_CRYSYSTEM_ITIMERMOCK_H
