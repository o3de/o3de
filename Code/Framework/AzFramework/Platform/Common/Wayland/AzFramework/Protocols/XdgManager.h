/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/EBus/EBus.h>
#include <AzFramework/WaylandInterface.h>

struct xdg_wm_base;
struct zxdg_decoration_manager_v1;

namespace AzFramework
{
    //XDG has a few protocols, so this class just manages them all
    class XdgManagerImpl
        : protected WaylandRegistryEventsBus::Handler
        , protected WaylandInterfaceNotificationsBus::MultiHandler
        , protected WaylandProxyBus::MultiHandler
    {
    public:
        XdgManagerImpl();
        ~XdgManagerImpl() override;

        //WaylandRegistryEventsBus
        void OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version) override;
        void OnUnregister(wl_registry*, uint32_t id) override;

        //WaylandInterfaceNotificationsBus
        void OnProtocolError(uint32_t registryId, uint32_t errorCode) override;

        //WaylandProxyBus
        wl_proxy* GetProxy(AZ::Crc32 interface) override;

        static void XdgPing(void* data, xdg_wm_base* xdg, uint32_t serial);

    private:
        xdg_wm_base* m_xdg = nullptr;
        zxdg_decoration_manager_v1* m_decor = nullptr;
    };
} // namespace AzFramework
