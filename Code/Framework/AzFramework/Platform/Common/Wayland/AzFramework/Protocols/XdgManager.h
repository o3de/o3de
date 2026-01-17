/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "XdgDecorManager.h"
#include "XdgShellManager.h"

#include <AzCore/Memory/SystemAllocator.h>

#include <AzCore/EBus/EBus.h>

#include <AzFramework/WaylandInterface.h>

namespace AzFramework
{
    //XDG has a few protocols, so this class just manages them all
    class XdgManagerImpl
        : public XdgShellConnectionManager
        , public XdgDecorConnectionManager
        , public WaylandRegistryEventsBus::Handler
        , public WaylandInterfaceNotificationsBus::MultiHandler
    {
    public:
        XdgManagerImpl();

        ~XdgManagerImpl() override;

        uint32_t GetXdgWmBaseRegistryId() const override;

        xdg_wm_base* GetXdgWmBase() const override;

        zxdg_decoration_manager_v1* GetXdgDecor() const override;

        void OnRegister(wl_registry* registry, uint32_t id, const AZ::Crc32 interface, uint32_t version) override;

        void OnUnregister(wl_registry*, uint32_t id) override;

        void OnProtocolError(uint32_t registryId, uint32_t errorCode) override;

        static void XdgPing(void* data, xdg_wm_base* xdg, uint32_t serial);

        const xdg_wm_base_listener s_xdg_wm_listener = { .ping = XdgPing };

    private:
        xdg_wm_base* m_xdg = nullptr;
        zxdg_decoration_manager_v1* m_decor = nullptr;
        uint32_t m_xdgId = 0;
        uint32_t m_decorId = 0;
    };
} // namespace AzFramework