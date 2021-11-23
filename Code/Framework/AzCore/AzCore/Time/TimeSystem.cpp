/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Time/TimeSystem.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace
    {
        void cvar_t_simulationTickScale_Changed(const float& value)
        {
            if (auto* timeSystem = AZ::Interface<ITime>::Get())
            {
                timeSystem->SetSimulationTickScale(value);
            }
        }

        void cvar_t_simulationTickDeltaOverride_Changed(const float& value)
        {
            if (auto* timeSystem = AZ::Interface<ITime>::Get())
            {
                timeSystem->SetSimulationTickDeltaOverride(AZ::SecondsToTimeMs(value));
            }
        }

        void cvar_t_simulationTickRate_Changed(const int& rate)
        {
            AZ_Warning("tick", false, "Simulation tick rate limiting is currently disabled. Setting will not be applied.");
            if (auto* timeSystem = AZ::Interface<ITime>::Get())
            {
                timeSystem->SetSimulationTickRate(rate);
            }
        }
    } // namespace

    AZ_CVAR(float, t_simulationTickScale, 1.0f, cvar_t_simulationTickScale_Changed, AZ::ConsoleFunctorFlags::Null,
        "A scalar amount to adjust time passage by, 1.0 == realtime, 0.5 == half realtime, 2.0 == doubletime");

    AZ_CVAR(float, t_simulationTickDeltaOverride, 0.0f, cvar_t_simulationTickDeltaOverride_Changed, AZ::ConsoleFunctorFlags::Null,
        "If > 0, overrides the simulation tick delta time with the provided value (Seconds) and ignores any t_simulationTickScale value.");

    AZ_CVAR(int, t_simulationTickRate, 0, cvar_t_simulationTickRate_Changed, AZ::ConsoleFunctorFlags::Null,
        "The minimum rate to force the game simulation tick to run. 0 for as fast as possible. 30 = ~33ms, 60 = ~16ms");

    void TimeSystem::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<TimeSystem, ITime>()
                ->Version(1);
        }
    }

    TimeSystem::TimeSystem()
    {
        m_lastInvokedTimeUs = static_cast<TimeUs>(AZStd::GetTimeNowMicroSecond());
        AZ::Interface<ITime>::Register(this);
        ITimeRequestBus::Handler::BusConnect();
    }

    TimeSystem::~TimeSystem()
    {
        AZ::Interface<ITime>::Unregister(this);
        ITimeRequestBus::Handler::BusDisconnect();
    }

    TimeMs TimeSystem::GetElapsedTimeMs() const
    {
        return AZ::TimeUsToMs(GetElapsedTimeUs());
    }

    TimeUs TimeSystem::GetElapsedTimeUs() const
    {
        TimeUs currentTime = static_cast<TimeUs>(AZStd::GetTimeNowMicroSecond());
        TimeUs deltaTime = currentTime - m_lastInvokedTimeUs;

        if (t_simulationTickScale != 1.0f)
        {
            const float floatDelta = AZStd::GetMax(static_cast<float>(deltaTime) * t_simulationTickScale, 1.0f);
            deltaTime = static_cast<TimeUs>(static_cast<int64_t>(floatDelta));
        }

        m_accumulatedTimeUs += deltaTime;
        m_lastInvokedTimeUs = currentTime;

        return m_accumulatedTimeUs;
    }

    TimeMs TimeSystem::GetRealElapsedTimeMs() const
    {
        return AZ::TimeUsToMs(GetRealElapsedTimeUs());
    }

    TimeUs TimeSystem::GetRealElapsedTimeUs() const
    {
        return static_cast<TimeUs>(AZStd::GetTimeNowMicroSecond());
    }

    TimeUs TimeSystem::GetSimulationTickDeltaTimeUs() const
    {
        return m_simulationTickDeltaTimeUs;
    }

    TimeUs TimeSystem::GetRealTickDeltaTimeUs() const
    {
        return m_realTickDeltaTimeUs;
    }

    TimeUs TimeSystem::GetLastSimulationTickTime() const
    {
        return m_lastSimulationTickTimeUs;
    }

    TimeUs TimeSystem::AdvanceTickDeltaTimes()
    {
        const TimeUs currentTimeUs = static_cast<TimeUs>(AZStd::GetTimeNowMicroSecond());

        //real time
        m_realTickDeltaTimeUs = currentTimeUs - m_lastSimulationTickTimeUs;

        //game time
        if (m_simulationTickDeltaOverride > AZ::Time::ZeroTimeUs)
        {
            m_simulationTickDeltaTimeUs = m_simulationTickDeltaOverride;
            m_lastSimulationTickTimeUs = currentTimeUs;
            return m_simulationTickDeltaTimeUs;
        }

        m_simulationTickDeltaTimeUs = currentTimeUs - m_lastSimulationTickTimeUs;

        if (!AZ::IsClose(t_simulationTickScale, 1.0f))
        {
            const double floatDelta = AZStd::GetMax(static_cast<double>(m_simulationTickDeltaTimeUs) * static_cast<double>(t_simulationTickScale), 1.0);
            m_simulationTickDeltaTimeUs = static_cast<TimeUs>(static_cast<int64_t>(floatDelta));
        }
        m_lastSimulationTickTimeUs = currentTimeUs;

        return m_simulationTickDeltaTimeUs;
    }

    void TimeSystem::ApplyTickRateLimiterIfNeeded()
    {
        // Currently disabling the Tick rate limiter as there are some reported issues when using it.
        #ifdef ENABLE_TICK_RATE_LIMITER
        // If tick rate limiting is on, ensure (1 / t_simulationTickRate) ms has elapsed since the last frame,
        // sleeping if there's still time remaining.
        if (t_simulationTickRate > 0)
        {
            const TimeUs currentTimeUs = AZ::GetRealElapsedTimeUs();
            const TimeUs timeUntilNextTick = (m_lastSimulationTickTimeUs + m_simulationTickLimitTimeUs) - currentTimeUs;
            if (timeUntilNextTick > AZ::Time::ZeroTimeUs)
            {
                AZ_TracePrintf("tick", "Sleeping for %.2f", AZ::TimeUsToSecondsDouble(timeUntilNextTick));
                AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(static_cast<int64_t>(timeUntilNextTick)));
            }
        }
        #endif // #ifdef ENABLE_TICK_RATE_LIMITER
    }

    void TimeSystem::SetSimulationTickDeltaOverride(TimeMs timeMs)
    {
        const TimeUs timeUs = AZ::TimeMsToUs(timeMs);
        if (timeUs != m_simulationTickDeltaOverride)
        {
            m_simulationTickDeltaOverride = timeUs;
            t_simulationTickDeltaOverride = AZ::TimeUsToSeconds(timeUs); //update the cvar
        }
    }

    TimeMs TimeSystem::GetSimulationTickDeltaOverride() const
    {
        return AZ::TimeUsToMs(m_simulationTickDeltaOverride);
    }

    void TimeSystem::SetSimulationTickScale(float scale)
    {
        if (!AZ::IsClose(scale, t_simulationTickScale))
        {
            t_simulationTickScale = scale;
        }
    }

    float TimeSystem::GetSimulationTickScale() const
    {
        return t_simulationTickScale;
    }

    void TimeSystem::SetSimulationTickRate(int rate)
    {
        m_simulationTickLimitRate = AZStd::abs(rate);
        if (m_simulationTickLimitRate != 0)
        {
            m_simulationTickLimitTimeUs = AZ::SecondsToTimeUs(1.0f / m_simulationTickLimitRate);
        }
        else
        {
            m_simulationTickLimitTimeUs = AZ::Time::ZeroTimeUs;
        }
    }

    int32_t TimeSystem::GetSimulationTickRate() const
    {
        return m_simulationTickLimitRate;
    }
} // namespace AZ
