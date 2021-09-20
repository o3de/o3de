/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYSYSTEM_TIMER_H
#define CRYINCLUDE_CRYSYSTEM_TIMER_H

# pragma once
#include <ITimer.h>

// Implements all common timing routines
class CTimer
    : public ITimer
{
public:
    // constructor
    CTimer();
    // destructor
    ~CTimer() = default;

    bool Init();

    // interface ITimer ----------------------------------------------------------

    // TODO: Review m_time usage in System.cpp
    //       if it wants Game Time / UI Time or a new Render Time?

    void ResetTimer() override;
    void UpdateOnFrameStart() override;
    float GetCurrTime(ETimer which = ETIMER_GAME) const override;
    CTimeValue GetAsyncTime() const override;
    float GetAsyncCurTime() override; // retrieve the actual wall clock time passed since the game started, in seconds
    float GetFrameTime(ETimer which = ETIMER_GAME) const override;
    float GetRealFrameTime() const override;
    float GetTimeScale() const override;
    float GetTimeScale(uint32 channel) const override;
    void SetTimeScale(float scale, uint32 channel = 0) override;
    void ClearTimeScales() override;
    void EnableTimer(bool bEnable) override;
    float GetFrameRate() override;
    float GetProfileFrameBlending(float* pfBlendTime = nullptr, int* piBlendMode = nullptr) override;
    void Serialize(TSerialize ser) override;
    bool IsTimerEnabled() const override;

    //! try to pause/unpause a timer
    //  returns true if successfully paused/unpaused, false otherwise
    bool PauseTimer(ETimer which, bool bPause) override;

    //! determine if a timer is paused
    //  returns true if paused, false otherwise
    bool IsTimerPaused(ETimer which) override;

    //! try to set a timer
    //  return true if successful, false otherwise
    bool SetTimer(ETimer which, float timeInSeconds) override;

    //! make a tm struct from a time_t in UTC (like gmtime)
    void SecondsToDateUTC(time_t time, struct tm& outDateUTC) override;

    //! make a UTC time from a tm (like timegm, but not available on all platforms)
    time_t DateToSecondsUTC(struct tm& timePtr) override;

    //! Convert from Tics to Seconds
    float TicksToSeconds(int64 ticks) override
    {
        return float((double)ticks * m_fSecsPerTick);
    }

    //! Get number of ticks per second
    int64 GetTicksPerSecond() override
    {
        return m_lTicksPerSec;
    }

    const CTimeValue& GetFrameStartTime(ETimer which = ETIMER_GAME) const override { return m_CurrTime[(int)which]; }
    ITimer* CreateNewTimer() override;

    void EnableFixedTimeMode(bool enable, float timeStep) override;

private: // ---------------------------------------------------------------------

    // ---------------------------------------------------------------------------

    // updates m_CurrTime (either pass m_lCurrentTime or custom curTime)
    void RefreshGameTime(int64 curTime);
    void RefreshUITime(int64 curTime);
    void UpdateBlending();
    float GetAverageFrameTime();

    // Updates the game-time offset to match the the specified time.
    // The argument is the new number of ticks since the last Reset().
    void SetOffsetToMatchGameTime(int64 ticks);

    // Convert seconds to ticks using the timer frequency.
    // Note: Loss of precision may occur, especially if magnitude of argument or timer frequency is large.
    int64 SecondsToTicks(double seconds) const;

    enum
    {
        MAX_FRAME_AVERAGE  = 100,
        NUM_TIME_SCALE_CHANNELS = 8,
    };

    //////////////////////////////////////////////////////////////////////////
    // Dynamic state, reset by ResetTimer()
    //////////////////////////////////////////////////////////////////////////
    CTimeValue m_CurrTime[ETIMER_LAST]; // Time since last Reset(), cached during Update()

    int64 m_lBaseTime; // Ticks elapsed since system boot, all other tick-unit variables are relative to this.
    int64 m_lLastTime; // Ticks since last Reset(). This is the base for UI time. UI time is monotonic, it always moves forward at a constant rate until the timer is Reset()).
    int64 m_lOffsetTime; // Additional ticks for Game time (relative to UI time). Game time can be affected by loading, pausing, time smoothing and time clamping, as well as SetTimer().

    //// the GetcurAsyncTime function appears to want to return the actual wall clock time delta
    //// but its using the base time (above) which is adjusted when there is a frame skip.
    //int64 m_lBaseTime_Async;

    float m_fFrameTime; // In seconds since the last Update(), clamped/smoothed etc.
    float m_fRealFrameTime; // In real seconds since the last Update(), non-clamped/un-smoothed etc.

    bool  m_bGameTimerPaused; // Set if the game is paused. GetFrameTime() will return 0, GetCurrTime(ETIMER_GAME) will not progress.
    int64 m_lGameTimerPausedTime; // The UI time when the game timer was paused. On un-pause, offset will be adjusted to match.

    //////////////////////////////////////////////////////////////////////////
    // Persistant state, kept by ResetTimer()
    //////////////////////////////////////////////////////////////////////////
    bool m_bEnabled;
    unsigned int m_nFrameCounter;

    int64 m_lTicksPerSec; // Ticks per second
    double m_fSecsPerTick; // Seconds per tick

    // smoothing
    float m_arrFrameTimes[MAX_FRAME_AVERAGE];
    float m_fAverageFrameTime; // used for smoothing (AverageFrameTime())

    float m_fAvgFrameTime;     // used for blend weighting (UpdateBlending())
    float m_fProfileBlend;     // current blending amount for profile.
    float m_fSmoothTime;       // smoothing interval (up to m_profile_smooth_time).

    // time scale
    float m_timeScaleChannels[NUM_TIME_SCALE_CHANNELS];
    float m_totalTimeScale;

    //////////////////////////////////////////////////////////////////////////
    // Console vars, always have default value on secondary CTimer instances
    //////////////////////////////////////////////////////////////////////////
    float m_fixed_time_step;   // in seconds
    float m_max_time_step;     // in seconds
    float m_cvar_time_scale;   // slow down time cvar
    int   m_TimeSmoothing;     // Console Variable, 0=off, otherwise on
    int             m_TimeDebug;                                // Console Variable, 0=off, otherwise on

    // Profile averaging help.
    float m_profile_smooth_time;  // seconds to exponentially smooth profile results.
    int   m_profile_weighting;    // weighting mode (see RegisterVar desc).

    //bool m_fixedTimeModeEnabled;
    //float m_fixedTimeModeStep;
};

#endif // CRYINCLUDE_CRYSYSTEM_TIMER_H
