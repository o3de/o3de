/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Time/ITime.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/TypeSafeIntegral.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class Name;
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

        //! Adds a scheduled event to run in durationMs.
        //! Actual duration is not guaranteed but will not be less than the value provided.
        //! @param scheduledEvent a scheduled event to add
        //! @param durationMs a millisecond interval to run the scheduled event
        //! @return pointer to the handle for this scheduled event, IEventScheduler maintains ownership
        virtual ScheduledEventHandle* AddEvent(ScheduledEvent* scheduledEvent, TimeMs durationMs) = 0;

        //! Schedules a callback to run in durationMs.
        //! Actual duration is not guaranteed but will not be less than the value provided.
        //! @param callback a callback to invoke after durationMs
        //! @param eventName a text descriptor of the callback
        //! @param durationMs a millisecond interval to run the scheduled callback
        virtual void AddCallback(const AZStd::function<void()>& callback, const Name& eventName, TimeMs durationMs) = 0;

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
