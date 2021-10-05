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
#include <AzCore/std/time.h>
#include <AzCore/std/chrono/chrono.h>

namespace AZ
{
    //! This is a strong typedef for representing a millisecond value since application start.
    AZ_TYPE_SAFE_INTEGRAL(TimeMs, int64_t);

    //! This is a strong typedef for representing a microsecond value since application start.
    //! Using int64_t as the underlying type, this is good to represent approximately 292,471 years
    AZ_TYPE_SAFE_INTEGRAL(TimeUs, int64_t);

    //! @class ITime
    //! @brief This is an AZ::Interface<> for managing time related operations.
    //! AZ::ITime and associated types may not operate in realtime. These abstractions are to allow our application
    //! simulation to operate both slower and faster than realtime in a well defined and user controllable manner
    //! The rate at which time passes for AZ::ITime is controlled by the cvar t_scale
    //!     t_scale == 0 means simulation time should halt
    //! 0 < t_scale <  1 will cause time to pass slower than realtime, with t_scale 0.1 being roughly 1/10th realtime
    //!     t_scale == 1 will cause time to pass at roughly realtime
    //!     t_scale >  1 will cause time to pass faster than normal, with t_scale 10 being roughly 10x realtime
    class ITime
    {
    public:
        AZ_RTTI(ITime, "{89D22C98-1450-44F1-952F-41284CC355F9}");

        ITime() = default;
        virtual ~ITime() = default;

        //! Returns the number of milliseconds since application start.
        //! @return the number of milliseconds that have elapsed since application start
        virtual TimeMs GetElapsedTimeMs() const = 0;

        //! Returns the number of microseconds since application start.
        //! @return the number of microseconds that have elapsed since application start
        virtual TimeUs GetElapsedTimeUs() const = 0;

        AZ_DISABLE_COPY_MOVE(ITime);
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

    //! Converts from microseconds to seconds
    inline float TimeUsToSeconds(TimeUs value)
    {
        return static_cast<float>(value) / 1000000.0f;
    }

    //! Converts from milliseconds to AZStd::chrono::time_point
    inline auto TimeMsToChrono(TimeMs value)
    {
        auto epoch = AZStd::chrono::time_point<AZStd::chrono::high_resolution_clock>();
        auto chronoValue = AZStd::chrono::milliseconds(aznumeric_cast<int64_t>(value));
        return epoch + chronoValue;
    }

    //! Converts from microseconds to AZStd::chrono::time_point
    inline auto TimeUsToChrono(TimeUs value)
    {
        auto epoch = AZStd::chrono::time_point<AZStd::chrono::high_resolution_clock>();
        auto chronoValue = AZStd::chrono::microseconds(aznumeric_cast<int64_t>(value));
        return epoch + chronoValue;
    }
} // namespace AZ

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AZ::TimeMs);
AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AZ::TimeUs);
