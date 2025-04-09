/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/time.h>

namespace AZ
{
    //! This is a strong typedef for representing a millisecond value since application start.
    AZ_TYPE_SAFE_INTEGRAL(TimeMs, int64_t);

    //! This is a strong typedef for representing a microsecond value since application start.
    //! Using int64_t as the underlying type, this is good to represent approximately 292,471 years
    AZ_TYPE_SAFE_INTEGRAL(TimeUs, int64_t);

    namespace Time
    {
        static const AZ::TimeMs ZeroTimeMs = AZ::TimeMs{ 0 };
        static const AZ::TimeUs ZeroTimeUs = AZ::TimeUs{ 0 };
    }

    //! @class ITime
    //! @brief This is an AZ::Interface<> for managing time related operations.
    //! AZ::ITime and associated types may not operate in realtime. These abstractions are to allow our application
    //! simulation to operate both slower and faster than realtime in a well defined and user controllable manner
    //! The rate at which time passes for AZ::ITime is controlled by the cvar t_simulationTickScale
    //!     t_simulationTickScale == 0 means simulation time should halt
    //! 0 < t_simulationTickScale <  1 will cause time to pass slower than realtime, with t_simulationTickScale 0.1 being roughly 1/10th realtime
    //!     t_simulationTickScale == 1 will cause time to pass at roughly realtime
    //!     t_simulationTickScale >  1 will cause time to pass faster than normal, with t_simulationTickScale 10 being roughly 10x realtime
    class ITime
    {
    public:
        AZ_RTTI(ITime, "{89D22C98-1450-44F1-952F-41284CC355F9}");

        ITime() = default;
        virtual ~ITime() = default;

        //! Returns the number of milliseconds since application start scaled by t_simulationTickScale.
        //! @return The number of milliseconds that have elapsed since application start.
        virtual TimeMs GetElapsedTimeMs() const = 0;

        //! Returns the number of microseconds since application start scaled by t_simulationTickScale.
        //! @return the number of microseconds that have elapsed since application start
        virtual TimeUs GetElapsedTimeUs() const = 0;
        
        //! Returns the number of milliseconds since application start.
        //! This value is not affected by the t_simulationTickScale cvar.
        //! @return The number of milliseconds that have elapsed since application start.
        virtual TimeMs GetRealElapsedTimeMs() const = 0;

        //! Returns the number of microseconds since application start.
        //! This value is not affected by the t_simulationTickScale cvar.
        //! @return The number of microseconds that have elapsed since application start.
        virtual TimeUs GetRealElapsedTimeUs() const = 0;

        //! Sets the number of milliseconds since application start.
        //! For debug purposes only.
        //! @param The number of milliseconds that have elapsed since application start.
        virtual void SetElapsedTimeMsDebug(TimeMs time) = 0;

        //! Sets the number of microseconds since application start.
        //! For debug purposes only.
        //! @param The number of microseconds that have elapsed since application start.
        virtual void SetElapsedTimeUsDebug(TimeUs time) = 0;

        //! Returns the current simulation tick delta time.
        //! This is affected by the cvars t_simulationTickScale, t_simulationTickDeltaOverride, and t_maxGameTickDelta.
        //! @return The number of microseconds elapsed since the last game tick.
        virtual TimeUs GetSimulationTickDeltaTimeUs() const = 0;

        //! Returns the non-manipulated tick time.
        //! @return The number of microseconds elapsed since the last game tick.
        virtual TimeUs GetRealTickDeltaTimeUs() const = 0;

        //! Returns the time since application start of when the last simulation tick was updated.
        virtual TimeUs GetLastSimulationTickTime() const = 0;

        //! If > 0 this will override the simulation tick delta time with the provided value.
        //! When enabled this will ignore any set simulation tick scale.
        //! Setting to 0 disables the override.
        //! @param timeMs The time in milliseconds to use for the tick delta.
        virtual void SetSimulationTickDeltaOverride(TimeMs timeMs) = 0;
        virtual void SetSimulationTickDeltaOverride(TimeUs timeUs) = 0;

        //! Returns the current simulation tick override.
        //! 0 means disabled.
        //! @returns The current simulation tick override in milliseconds.
        virtual TimeUs GetSimulationTickDeltaOverride() const = 0;

        //! A scalar amount to adjust the passage of time by, 1.0 == realtime, 0.5 == half realtime, 2.0 == doubletime.
        //! @param scale The scalar value to apply to the simulation time.
        virtual void SetSimulationTickScale(float scale) = 0;

        //! Returns the current simulation tick scale.
        //! 1.0 == realtime, 0.5 == half realtime, 2.0 == doubletime.
        //! @returns The simulation tick scale value.
        virtual float GetSimulationTickScale() const = 0;

        //! The minimum rate to force the simulation tick to run.
        //! 0 for as fast as possible. 30 = ~33ms, 60 = ~16ms.
        //! Setting to 0 will disable rate limiting.
        //! @note It is not guaranteed to hit the requested tick rate exactly.
        //! @param rate The rate in frames per second.
        virtual void SetSimulationTickRate(int rate) = 0;

        //! Return the current simulation tick rate.
        //! 0 means disabled.
        //! @return The rate in frames per second.
        virtual int32_t GetSimulationTickRate() const = 0;

        AZ_DISABLE_COPY_MOVE(ITime);

        static const AZ::TimeMs ZeroTimeMs = AZ::TimeMs{ 0 };
        static const AZ::TimeUs ZeroTimeUs = AZ::TimeUs{ 0 };
    };

    // EBus wrapper for ScriptCanvas
    class ITimeRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using ITimeRequestBus = AZ::EBus<ITime, ITimeRequests>;

    //! This is a simple convenience wrapper
    inline TimeMs GetElapsedTimeMs()
    {
        return AZ::Interface<ITime>::Get()->GetElapsedTimeMs();
    }

    //! This is a simple convenience wrapper
    inline TimeUs GetElapsedTimeUs()
    {
        return AZ::Interface<ITime>::Get()->GetElapsedTimeUs();
    }

    //! This is a simple convenience wrapper
    inline TimeMs GetRealElapsedTimeMs()
    {
        return AZ::Interface<ITime>::Get()->GetRealElapsedTimeMs();
    }

    //! This is a simple convenience wrapper
    inline TimeUs GetRealElapsedTimeUs()
    {
        return AZ::Interface<ITime>::Get()->GetRealElapsedTimeUs();
    }

    //! This is a simple convenience wrapper
    inline TimeUs GetSimulationTickDeltaTimeUs()
    {
        return AZ::Interface<ITime>::Get()->GetSimulationTickDeltaTimeUs();
    }

    //! This is a simple convenience wrapper
    inline TimeUs GetRealTickDeltaTimeUs()
    {
        return AZ::Interface<ITime>::Get()->GetRealTickDeltaTimeUs();
    }

    //! This is a simple convenience wrapper
    inline TimeUs GetLastSimulationTickTime()
    {
        return AZ::Interface<ITime>::Get()->GetLastSimulationTickTime();
    }

    //! Converts from milliseconds to microseconds
    inline TimeUs TimeMsToUs(TimeMs value)
    {
        return static_cast<TimeUs>(value * static_cast<TimeMs>(1000));
    }

    //! Converts from microseconds to milliseconds
    inline TimeMs TimeUsToMs(TimeUs value)
    {
        return static_cast<TimeMs>(value / static_cast<TimeUs>(1000));
    }

    //! Converts from milliseconds to seconds
    inline float TimeMsToSeconds(TimeMs value)
    {
        return static_cast<float>(value) / 1000.0f;
    }

    //! Converts from milliseconds to seconds
    inline double TimeMsToSecondsDouble(TimeMs value)
    {
        return static_cast<double>(value) / 1000.0;
    }

    //! Converts from microseconds to seconds
    inline float TimeUsToSeconds(TimeUs value)
    {
        return static_cast<float>(value) / 1000000.0f;
    }

    //! Converts from microseconds to seconds
    inline double TimeUsToSecondsDouble(TimeUs value)
    {
        return static_cast<double>(value) / 1000000.0;
    }

    //! Converts from milliseconds to AZStd::chrono::time_point
    inline auto TimeMsToChrono(TimeMs value)
    {
        AZStd::chrono::steady_clock::time_point epoch;
        auto chronoValue = AZStd::chrono::milliseconds(aznumeric_cast<int64_t>(value));
        return epoch + chronoValue;
    }

    //! Converts from microseconds to AZStd::chrono::time_point
    inline auto TimeUsToChrono(TimeUs value)
    {
        AZStd::chrono::steady_clock::time_point epoch;
        auto chronoValue = AZStd::chrono::microseconds(aznumeric_cast<int64_t>(value));
        return epoch + chronoValue;
    }

    //! A utility function to convert from seconds to TimeMs
    inline TimeMs SecondsToTimeMs(const double value)
    {
        const double valueMs = value * 1000.0;
        return static_cast<TimeMs>(static_cast<int64_t>(valueMs));
    }

    //! A utility function to convert from seconds to TimeUs
    inline TimeUs SecondsToTimeUs(const double value)
    {
        const double valueMs = value * 1000000.0;
        return static_cast<TimeUs>(static_cast<int64_t>(valueMs));
    }
} // namespace AZ

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AZ::TimeMs);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AZ::TimeUs);
