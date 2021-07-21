/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_ITIMER_H
#define CRYINCLUDE_CRYCOMMON_ITIMER_H
#pragma once


#include "TimeValue.h"              // CTimeValue
#include "SerializeFwd.h"

struct tm;

// Summary:
// Interface to the Timer System.
struct ITimer
{
    enum ETimer
    {
        ETIMER_GAME = 0, // Pausable, serialized, frametime is smoothed/scaled/clamped.
        ETIMER_UI,       // Non-pausable, non-serialized, frametime unprocessed.
        ETIMER_LAST
    };

    enum ETimeScaleChannels
    {
        eTSC_Trackview = 0,
        eTSC_GameStart
    };

    // <interfuscator:shuffle>
    virtual ~ITimer() {};

    // Summary:
    //   Resets the timer
    // Notes:
    //   Only needed because float precision wasn't last that long - can be removed if 64bit is used everywhere.
    virtual void ResetTimer() = 0;

    // Summary:
    //   Updates the timer every frame, needs to be called by the system.
    virtual void UpdateOnFrameStart() = 0;

    // Summary:
    //   Returns the absolute time at the last UpdateOnFrameStart() call.
    // Todo:
    //   Remove, use GetFrameStartTime() instead.
    // See also:
    //   UpdateOnFrameStart(),GetFrameStartTime()
    virtual float GetCurrTime(ETimer which = ETIMER_GAME) const = 0;

    // Summary:
    //   Returns the absolute time at the last UpdateOnFrameStart() call.
    // See also:
    //   UpdateOnFrameStart()
    //virtual const CTimeValue& GetFrameStartTime(ETimer which = ETIMER_GAME) const = 0;
    virtual const CTimeValue& GetFrameStartTime(ETimer which = ETIMER_GAME) const = 0;

    // Summary:
    //   Returns the absolute current time.
    // Notes:
    //   The value continuously changes, slower than GetFrameStartTime().
    // See also:
    //   GetFrameStartTime()
    virtual CTimeValue GetAsyncTime() const = 0;

    // Summary:
    //  Returns the absolute current time at the moment of the call.
    virtual float GetAsyncCurTime() = 0;

    // Summary:
    //   Returns the relative time passed from the last UpdateOnFrameStart() in seconds.
    // See also:
    //   UpdateOnFrameStart()
    virtual float GetFrameTime(ETimer which = ETIMER_GAME) const = 0;

    // Description:
    //   Returns the relative time passed from the last UpdateOnFrameStart() in seconds without any dilation, smoothing, clamping, etc...
    // See also:
    //   UpdateOnFrameStart()
    virtual float GetRealFrameTime() const = 0;

    // Summary:
    //   Returns the time scale applied to time values.
    virtual float GetTimeScale() const = 0;

    // Summary:
    //  Returns the time scale factor for the given channel
    virtual float GetTimeScale(uint32 channel) const = 0;

    // Summary:
    //  Clears all current time scale requests
    virtual void ClearTimeScales() = 0;

    // Summary:
    //  Sets the time scale applied to time values.
    virtual void SetTimeScale(float s, uint32 channel = 0) = 0;

    // Summary:
    //   Enables/disables timer.
    virtual void EnableTimer(bool bEnable) = 0;

    // Return Value:
    //   True if timer is enabled
    virtual bool IsTimerEnabled() const = 0;

    // Summary:
    //   Returns the current framerate in frames/second.
    virtual float   GetFrameRate() = 0;

    // Summary:
    //   Returns the fraction to blend current frame in profiling stats.
    virtual float GetProfileFrameBlending(float* pfBlendTime = 0, int* piBlendMode = 0) = 0;

    // Summary:
    //   Serialization.
    virtual void Serialize(TSerialize ser) = 0;

    // Summary:
    //   Tries to pause/unpause a timer.
    // Return Value:
    //   True if successfully paused/unpaused, false otherwise.
    virtual bool PauseTimer(ETimer which, bool bPause) = 0;

    // Summary:
    //   Determines if a timer is paused.
    // Returns:
    //   True if paused, false otherwise.
    virtual bool IsTimerPaused(ETimer which) = 0;

    // Summary:
    //   Tries to set a timer.
    // Returns:
    //   True if successful, false otherwise.
    virtual bool SetTimer(ETimer which, float timeInSeconds) = 0;

    // Summary:
    //   Makes a tm struct from a time_t in UTC
    // Example:
    //   Like gmtime.
    virtual void SecondsToDateUTC(time_t time, struct tm& outDateUTC) = 0;

    // Summary:
    //   Makes a UTC time from a tm.
    // Example:
    //   Like timegm, but not available on all platforms.
    virtual time_t DateToSecondsUTC(struct tm& timePtr) = 0;


    // Summary
    //  Convert from ticks (CryGetTicks()) to seconds
    //
    virtual float TicksToSeconds(int64 ticks) = 0;

    // Summary
    //  Get number of ticks per second
    //
    virtual int64 GetTicksPerSecond() = 0;

    // Summary
    //  Create a new timer of the same type
    //
    virtual ITimer* CreateNewTimer() = 0;

    /*!
        This is similar to the cvar t_FixedStep. However it is stronger, and will cause even GetRealFrameTime to follow the fixed time stamp.
        GetRealFrameTime will always return the same value as GetFrameTime. This mode is mostly intended for Feature tests that have strict requirements
        for determinism. It will cause even fps counters to return a fixed value that does not match the actual fps. I could see this also being useful
        if rendering a video.
    */
    virtual void EnableFixedTimeMode(bool enable, float timeStep) = 0;
    // </interfuscator:shuffle>
};

// Description:
//   This class is used for automatic profiling of a section of the code.
//   Creates an instance of this class, and upon exiting from the code section.
template <typename time>
class CITimerAutoProfiler
{
public:
    CITimerAutoProfiler (ITimer* pTimer, time& rTime)
        : m_pTimer (pTimer)
        , m_rTime (rTime)
    {
        rTime -= pTimer->GetAsyncCurTime();
    }

    ~CITimerAutoProfiler ()
    {
        m_rTime += m_pTimer->GetAsyncCurTime();
    }

protected:
    ITimer* m_pTimer;
    time& m_rTime;
};

// Description:
//   Include this string AUTO_PROFILE_SECTION(pITimer, g_fTimer) for the section of code where the profiler timer must be turned on and off.
//   The profiler timer is just some global or static float or double value that accumulates the time (in seconds) spent in the given block of code.
//   pITimer is a pointer to the ITimer interface, g_fTimer is the global accumulator.
#define AUTO_PROFILE_SECTION(pITimer, g_fTimer) CITimerAutoProfiler<double> __section_auto_profiler(pITimer, g_fTimer)

#endif // CRYINCLUDE_CRYCOMMON_ITIMER_H
