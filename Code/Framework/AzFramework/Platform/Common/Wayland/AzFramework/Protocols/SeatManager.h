/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

#include <wayland-client.h>

namespace AzFramework
{
    class SeatManager
    {
    public:
        AZ_RTTI(SeatManager, "{F2601B85-C823-454C-8053-18F44293F58A}");

        virtual ~SeatManager() = default;

        virtual uint32_t GetSeatCount() const = 0;
        virtual wl_pointer* GetSeatPointer(uint32_t playerIdx) const = 0;
        virtual wl_keyboard* GetSeatKeyboard(uint32_t playerIdx) const = 0;
        virtual wl_touch* GetSeatTouch(uint32_t playerIdx) const = 0;
    };

    using SeatManagerInterface = AZ::Interface<SeatManager>;

    class SeatNotifications
    {
    public:
        AZ_RTTI(SeatNotifications, "{6BBCA05F-51BE-4244-92FB-A54938339A38}");

        virtual ~SeatNotifications() = default;

        virtual void SeatCapsChanged() = 0;
        virtual void ReleaseSeat() = 0;
    };

    class SeatNotificationsBusTraits : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        // PlayerIdx
        using BusIdType = uint32_t;
    };

    using SeatNotificationsBus = AZ::EBus<SeatNotifications, SeatNotificationsBusTraits>;

} // namespace AzFramework