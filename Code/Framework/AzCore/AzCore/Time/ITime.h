/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

namespace AZ
{
    //! This is a strong typedef for representing a millisecond value since application start.
    AZ_TYPE_SAFE_INTEGRAL(TimeMs, int64_t);

    //! @class ITime
    //! @brief This is an AZ::Interface<> for managing time related operations.
    class ITime
    {
    public:
        AZ_RTTI(ITime, "{89D22C98-1450-44F1-952F-41284CC355F9}");

        ITime() = default;
        virtual ~ITime() = default;

        //! Returns the number of milliseconds since application start.
        //! @return the number of milliseconds that have elapsed since application start
        virtual TimeMs GetElapsedTimeMs() const = 0;

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
}

AZ_TYPE_SAFE_INTEGRAL_SERIALIZEBINDING(AZ::TimeMs);
