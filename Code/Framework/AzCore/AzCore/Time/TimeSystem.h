/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Time/ITime.h>

namespace AZ
{
    class ReflectContext;

    //! Implementation of the ITime system interface.
    class TimeSystem
        : public ITimeRequestBus::Handler
    {
    public:
        AZ_RTTI(AZ::TimeSystem, "{CE1C5E4F-7DC1-4248-B10C-AC55E8924A48}", AZ::ITime);

        static void Reflect(AZ::ReflectContext* context);

        TimeSystem();
        virtual ~TimeSystem();

        //! ITime overrides.
        //! @{
        TimeMs GetElapsedTimeMs() const override;
        TimeUs GetElapsedTimeUs() const override;
        TimeMs GetRealElapsedTimeMs() const override;
        TimeUs GetRealElapsedTimeUs() const override;
        void SetElapsedTimeMsDebug(TimeMs time) override;
        void SetElapsedTimeUsDebug(TimeUs time) override;
        TimeUs GetSimulationTickDeltaTimeUs() const override;
        TimeUs GetRealTickDeltaTimeUs() const override;
        TimeUs GetLastSimulationTickTime() const override;
        void SetSimulationTickDeltaOverride(TimeMs timeMs) override;
        void SetSimulationTickDeltaOverride(TimeUs timeUs) override;
        TimeUs GetSimulationTickDeltaOverride() const override;
        void SetSimulationTickScale(float scale) override;
        float GetSimulationTickScale() const override;
        void SetSimulationTickRate(int rate) override;
        int32_t GetSimulationTickRate() const override;
        //! @}
        
        //! Advances the Simulation and Real tick delta time counters.
        //! This is called from the owner of the TimeSystem, ComponentApplication in Tick().
        //! @return The delta in microseconds from the last call to AdvanceTickDeltaTimes(). Value will be the same as GetSimulationTickDeltaTimeUs().
        TimeUs AdvanceTickDeltaTimes();

        //! If t_simulationTickRate is >0 this will try to have the game delta time run at a maximum of the rate set.
        //! This is called from the owner of the TimeSystem, ComponentApplication in Tick().
        //! example. If t_simulationTickRate is set to 60Fps, and the game tick delta is <17ms(60fps), this will add a sleep for the remaining time.
        //! example. If t_simulationTickRate is set to 60Fps, and the game tick delta is >=17ms(60fps), this will not sleep at all.
        //! @note It is not guaranteed to hit the requested tick rate exactly.
        void ApplyTickRateLimiterIfNeeded();
    private:
        //! Used to calculate the delta time between calls to GetElapsedTimeMs/TimeUs().
        //! Mutable to allow GetElapsedTimeMs/TimeUs() to be a const functions.
        mutable TimeUs m_lastInvokedTimeUs = AZ::Time::ZeroTimeUs;

        //! Accumulates the delta time of GetElapsedTimeMs/TimeUs() calls.
        //! Mutable to allow GetElapsedTimeMs/TimeUs() to be a const functions.
        mutable TimeUs m_accumulatedTimeUs = AZ::Time::ZeroTimeUs;

        //! Used to calculate the delta time between calls to GetRealElapsedTimeMs/TimeUs().
        //! Mutable to allow GetRealElapsedTimeMs/TimeUs() to be a const functions.
        mutable TimeUs m_realLastInvokedTimeUs = AZ::Time::ZeroTimeUs;

        //! Accumulates the delta time of GetRealElapsedTimeMs/TimeUs() calls.
        //! Mutable to allow GetRealElapsedTimeMs/TimeUs() to be a const functions.
        mutable TimeUs m_realAccumulatedTimeUs = AZ::Time::ZeroTimeUs;

        //! The current game tick delta time.
        //! Can be affected by time system cvars.
        //! Updated in AdvanceTickDeltaTimes().
        TimeUs m_simulationTickDeltaTimeUs = AZ::Time::ZeroTimeUs;

        //! The current real tick delta time.
        //! Will not be affected by time system cvars.
        //! Updated in AdvanceTickDeltaTimes().
        TimeUs m_realTickDeltaTimeUs = AZ::Time::ZeroTimeUs;

        TimeUs m_lastSimulationTickTimeUs = AZ::Time::ZeroTimeUs; //!< Used to determine the game tick delta time.

        TimeUs m_simulationTickDeltaOverride = AZ::Time::ZeroTimeUs; //<! Stores the TimeUs value of the t_simulationTickDeltaOverride cvar.
        TimeUs m_simulationTickLimitTimeUs = AZ::Time::ZeroTimeUs; //<! Stores the TimeUs value of the t_simulationTickRate cvar.
        int32_t m_simulationTickLimitRate = 0; //<! Stores the simulation rate limit in frames per second.
    };
}
