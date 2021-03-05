/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Time/ITime.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>

namespace AZ
{
    class ScheduledEvent;
    class ScheduledEventHandle;

    //! @class IEventScheduler
    //! @brief This is an AZ::Interface<> for managing scheduled events.
    //! Users should not require any direct interaction with this interface, ScheduledEvent is a self contained abstraction.
    class IEventScheduler
    {
    public:
        AZ_RTTI(IEventScheduler, "{D8146217-6F93-47EB-9037-53BBFE429666}");

        using ScheduledEventHandlePtr = AZStd::unique_ptr<ScheduledEventHandle>;

        IEventScheduler() = default;
        virtual ~IEventScheduler() = default;

        //! Adds a scheduled event and intervalMs to run it.
        //! Actual duration is not guaranteed but will not be less than the value provided.
        //! @param scheduledEvent a scheduled event to add
        //! @param durationMs an interval Ms to run the scheduled event
        //! @return pointer to the handle for this scheduled event, IEventScheduler maintains ownership
        virtual ScheduledEventHandle* Add(ScheduledEvent* scheduledEvent, TimeMs durationMs) = 0;

        AZ_DISABLE_COPY_MOVE(IEventScheduler);
    };

    // EBus wrapper for ScriptCanvas
    class IEventSchedulerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using IEventSchedulerRequestBus = AZ::EBus<IEventScheduler, IEventSchedulerRequests>;
}
