/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ITimer.h>

//! Simple stub timer that exposes a single simple interface for setting the current time.
class StubTimer
    : public ITimer
{
public:
    // Stub methods
    void SetTime(float seconds)
    {
        m_frameStartTime.SetSeconds(seconds);
    }
    //~Stub methods

    StubTimer(float frameTime)
        : m_frameTime(frameTime)
        , m_frameRate(1.0f / frameTime)
        , m_frameStartTime(0.0f)
    {
    }
    virtual ~StubTimer() {};

    // ITimer
    void ResetTimer() override {}
    void UpdateOnFrameStart() override {}
    float GetCurrTime([[maybe_unused]] ETimer which = ETIMER_GAME) const override
    {
        // return the same as the frame start time
        return m_frameStartTime.GetSeconds();
    }
    const CTimeValue& GetFrameStartTime([[maybe_unused]] ETimer which = ETIMER_GAME) const override
    {
        return m_frameStartTime;
    }
    CTimeValue GetAsyncTime() const override
    {
        return m_frameStartTime;
    }
    float GetAsyncCurTime() override
    {
        return m_frameStartTime.GetSeconds();
    }
    float GetFrameTime([[maybe_unused]] ETimer which = ETIMER_GAME) const override
    {
        return m_frameTime;
    }
    float GetRealFrameTime() const override
    {
        return m_frameTime;
    }
    float GetTimeScale() const override
    {
        return 1.0f;
    }
    float GetTimeScale([[maybe_unused]] uint32 channel) const override
    {
        return 1.0f;
    }
    void ClearTimeScales() override {}
    void SetTimeScale([[maybe_unused]] float s, [[maybe_unused]] uint32 channel = 0) override {}
    void EnableTimer([[maybe_unused]] bool bEnable) override {}
    bool IsTimerEnabled() const override
    {
        return true;
    }
    float GetFrameRate() override
    {
        return m_frameRate;
    }
    float GetProfileFrameBlending([[maybe_unused]] float* pfBlendTime = 0, [[maybe_unused]] int* piBlendMode = 0) override
    {
        return 0.0f;
    }
    void Serialize([[maybe_unused]] TSerialize ser) override {}
    bool PauseTimer([[maybe_unused]] ETimer which, [[maybe_unused]] bool bPause) override { return false; }
    bool IsTimerPaused([[maybe_unused]] ETimer which) override { return false; }
    bool SetTimer([[maybe_unused]] ETimer which, [[maybe_unused]] float timeInSeconds) override { return false; }
    void SecondsToDateUTC([[maybe_unused]] time_t time, [[maybe_unused]] struct tm& outDateUTC) override {}
    time_t DateToSecondsUTC([[maybe_unused]] struct tm& timePtr) override
    {
        return 0;
    }
    float TicksToSeconds([[maybe_unused]] int64 ticks) override
    {
        return 0.0f;
    }
    int64 GetTicksPerSecond() override
    {
        return 0;
    }
    ITimer* CreateNewTimer() override
    {
        return nullptr;
    }
    void EnableFixedTimeMode([[maybe_unused]] bool enable, [[maybe_unused]] float timeStep) override {}
    // ~ITimer

private:
    CTimeValue m_frameStartTime;
    float m_frameTime;
    float m_frameRate;
};
