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
#include <AzCore/std/string/string.h>
#include <AzCore/RTTI/RTTI.h>

#include <wayland-client.h>

#include "AzFramework/WaylandInterface.h"

namespace AzFramework
{
    class SeatManagerRequests
    {
    public:
        AZ_RTTI(SeatManagerRequests, "{F2601B85-C823-454C-8053-18F44293F58A}");
        virtual ~SeatManagerRequests() = default;

        virtual uint32_t GetSeatCount() const = 0;
        virtual wl_pointer* GetSeatPointer(uint32_t playerIdx) const = 0;
        virtual wl_keyboard* GetSeatKeyboard(uint32_t playerIdx) const = 0;
        virtual wl_touch* GetSeatTouch(uint32_t playerIdx) const = 0;
    };

    using SeatManagerInterface = AZ::Interface<SeatManagerRequests>;

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

    class SeatManager final
        : protected WaylandRegistryEventsBus::Handler
        , protected SeatManagerInterface::Registrar
    {
    public:
        SeatManager();
        ~SeatManager() override;

        //WaylandRegistryEventsBus
        void OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version) override;
        void OnUnregister(wl_registry*, uint32_t id) override;

        //SeatManager
        uint32_t GetSeatCount() const;
        wl_pointer* GetSeatPointer(uint32_t playerIdx) const;
        wl_keyboard* GetSeatKeyboard(uint32_t playerIdx) const;
        wl_touch* GetSeatTouch(uint32_t playerIdx) const;

    private:
        struct WaylandSeat
        {
            explicit WaylandSeat(wl_seat* inseat)
                : m_seat(inseat)
            {
            }

            wl_seat* m_seat = nullptr;
            uint32_t m_registryId = 0;
            uint32_t m_playerIdx = 0;
            bool m_supportsPointer = false;
            bool m_supportsKeyboard = false;
            bool m_supportsTouch = false;

            AZStd::string m_name = "";
        };

        static void SeatCaps(void* data, struct wl_seat* wl_seat, uint32_t capabilities);
        static void SeatName(void* data, struct wl_seat* wl_seat, const char* name);

        static wl_seat_listener s_seatListener;

        WaylandSeat* GetSeatFromPlayerIdx(uint32_t playerIdx) const;
        uint32_t GetAvailablePlayerIdx() const;

        // Registry id -> WaylandSeat Ptr
        AZStd::unordered_map<uint32_t, WaylandSeat*> m_seats = {};
    };

} // namespace AzFramework
