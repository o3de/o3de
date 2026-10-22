/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SeatManager.h"

#include <AzFramework/WaylandProtocolNames.h>

namespace AzFramework
{
    SeatManager::SeatManager()
    {
        WaylandRegistryEventsBus::Handler::BusConnect();
    }

    SeatManager::~SeatManager()
    {
        WaylandRegistryEventsBus::Handler::BusDisconnect();
        {
            // GlobalRegistryRemove will modify the vector
            // copy it for local
            auto seats = m_seats;
            for (auto& seat : seats)
            {
                OnUnregister(nullptr, seat.first);
            }
        }
        m_seats.clear();
    }

    void SeatManager::OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version)
    {
        if (interface != WaylandSeatName || version < WaylandSeatVersion)
        {
            return;
        }

        auto seat = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, WaylandSeatVersion));
        auto info = new WaylandSeat(seat);
        info->m_registryId = id;
        info->m_playerIdx = GetAvailablePlayerIdx();
        wl_seat_add_listener(seat, &s_seatListener, info);

        m_seats.emplace(id, info);
    }

    void SeatManager::OnUnregister(wl_registry*, uint32_t id)
    {
        if (m_seats.find(id) == m_seats.end())
        {
            return;
        }

        auto seat = m_seats[id];
        SeatNotificationsBus::Event(seat->m_playerIdx, &SeatNotificationsBus::Events::ReleaseSeat);

        m_seats.erase(id);
        wl_seat_destroy(seat->m_seat);
        delete seat;
    }

    uint32_t SeatManager::GetSeatCount() const
    {
        return m_seats.size();
    }

    wl_pointer* SeatManager::GetSeatPointer(uint32_t playerIdx) const
    {
        if (auto seat = GetSeatFromPlayerIdx(playerIdx))
        {
            if (!seat->m_supportsPointer)
            {
                return nullptr;
            }
            return wl_seat_get_pointer(seat->m_seat);
        }
        return nullptr;
    }

    wl_keyboard* SeatManager::GetSeatKeyboard(uint32_t playerIdx) const
    {
        if (auto seat = GetSeatFromPlayerIdx(playerIdx))
        {
            if (!seat->m_supportsKeyboard)
            {
                return nullptr;
            }
            return wl_seat_get_keyboard(seat->m_seat);
        }
        return nullptr;
    }

    wl_touch* SeatManager::GetSeatTouch(uint32_t playerIdx) const
    {
        if (auto seat = GetSeatFromPlayerIdx(playerIdx))
        {
            if (!seat->m_supportsTouch)
            {
                return nullptr;
            }
            return wl_seat_get_touch(seat->m_seat);
        }
        return nullptr;
    }

    void SeatManager::SeatCaps(void* data, wl_seat*, uint32_t capabilities)
    {
        auto self = static_cast<WaylandSeat*>(data);

        self->m_supportsPointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
        self->m_supportsKeyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
        self->m_supportsTouch = capabilities & WL_SEAT_CAPABILITY_TOUCH;

        AZ_Info("Wayland", "Seat capabilities updated for player idx: %i name: \"%s\"", self->m_playerIdx, self->m_name.c_str());
        AZ_Info("Wayland", "Supports Pointer? %s", self->m_supportsPointer ? "yes" : "no");
        AZ_Info("Wayland", "Supports Keyboard? %s", self->m_supportsKeyboard ? "yes" : "no");
        AZ_Info("Wayland", "Supports Touch? %s", self->m_supportsTouch ? "yes" : "no");

        SeatNotificationsBus::Event(self->m_playerIdx, &SeatNotificationsBus::Events::SeatCapsChanged);
    }

    void SeatManager::SeatName(void* data, wl_seat*, const char* name)
    {
        auto self = static_cast<WaylandSeat*>(data);
        self->m_name = AZStd::string(name);
    }

    wl_seat_listener SeatManager::s_seatListener = { .capabilities = SeatCaps, .name = SeatName };

    SeatManager::WaylandSeat* SeatManager::GetSeatFromPlayerIdx(uint32_t playerIdx) const
    {
        for (auto& seat : m_seats)
        {
            if (seat.second->m_playerIdx == playerIdx)
            {
                return seat.second;
            }
        }

        return nullptr;
    }

    uint32_t SeatManager::GetAvailablePlayerIdx() const
    {
        for (uint32_t i = 0; i < UINT32_MAX; ++i)
        {
            if (GetSeatFromPlayerIdx(i) == nullptr)
            {
                return i;
            }
        }

        return UINT32_MAX;
    }
} // namespace AzFramework
