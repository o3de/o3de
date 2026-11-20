/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Protocols/XdgManager.h>
#include <AzFramework/WaylandConnectionManager.h>
#include <AzFramework/WaylandProtocolNames.h>

#include <xdg-shell-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>

namespace AzFramework
{
    static xdg_wm_base_listener s_xdgWmListener = {XdgManagerImpl::XdgPing};

    XdgManagerImpl::XdgManagerImpl()
    {
        WaylandRegistryEventsBus::Handler::BusConnect();
    }

    XdgManagerImpl::~XdgManagerImpl()
    {
        WaylandRegistryEventsBus::Handler::BusDisconnect();

        if (m_xdg != nullptr)
        {
            xdg_wm_base_destroy(m_xdg);
        }
        if (m_decor != nullptr)
        {
            zxdg_decoration_manager_v1_destroy(m_decor);
        }
    }

    void XdgManagerImpl::OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version)
    {
        if (interface == XdgWmBaseName && version >= XdgWmBaseVersion)
        {
            m_xdg = static_cast<xdg_wm_base*>(wl_registry_bind(registry, id, &xdg_wm_base_interface, XdgWmBaseVersion));
            xdg_wm_base_add_listener(m_xdg, &s_xdgWmListener, this);
            WaylandInterfaceNotificationsBus::MultiHandler::BusConnect(id);
            WaylandProxyBus::MultiHandler::BusConnect(XdgWmBaseName);
            return;
        }
        if (interface == XdgDecorationManagerName && version >= XdgDecorationManagerVersion)
        {
            m_decor =
                static_cast<zxdg_decoration_manager_v1*>(wl_registry_bind(
                    registry, id, &zxdg_decoration_manager_v1_interface, XdgDecorationManagerVersion));
            WaylandInterfaceNotificationsBus::MultiHandler::BusConnect(id);
            WaylandProxyBus::MultiHandler::BusConnect(XdgDecorationManagerName);
            return;
        }
    }

    void XdgManagerImpl::OnUnregister(wl_registry*, uint32_t id)
    {
        if (id == WL_GET_PROXY_ID(m_xdg))
        {
            WaylandInterfaceNotificationsBus::MultiHandler::BusDisconnect(id);
            WaylandProxyBus::MultiHandler::BusDisconnect(XdgWmBaseName);
            xdg_wm_base_destroy(m_xdg);
            m_xdg = nullptr;
        }
        else if (id == WL_GET_PROXY_ID(m_decor))
        {
            WaylandInterfaceNotificationsBus::MultiHandler::BusDisconnect(id);
            WaylandProxyBus::MultiHandler::BusDisconnect(XdgDecorationManagerName);
            zxdg_decoration_manager_v1_destroy(m_decor);
            m_decor = nullptr;
        }
    }

    void XdgManagerImpl::OnProtocolError(uint32_t registryId, uint32_t errorCode)
    {
        auto conn = WaylandConnectionManagerInterface::Get();
        if (!conn)
        {
            return;
        }
        if (registryId == WL_GET_PROXY_ID(m_xdg))
        {
            switch (errorCode)
            {
            case XDG_WM_BASE_ERROR_ROLE:
                AZ_Error("XDG", false, "Given surface has other role.");
                break;
            case XDG_WM_BASE_ERROR_DEFUNCT_SURFACES:
                AZ_Error("XDG", false, "xdg_wm_base was destroyed before children");
                break;
            case XDG_WM_BASE_ERROR_NOT_THE_TOPMOST_POPUP:
                AZ_Error("XDG", false, "Tried to map or destroy a non-topmost popup");
                break;
            case XDG_WM_BASE_ERROR_INVALID_POPUP_PARENT:
                AZ_Error("XDG", false, "Invalid popup parent surface.");
                break;
            case XDG_WM_BASE_ERROR_INVALID_SURFACE_STATE:
                AZ_Error("XDG", false, "Invalid surface state");
                break;
            case XDG_WM_BASE_ERROR_INVALID_POSITIONER:
                AZ_Error("XDG", false, "Invalid positioner.");
                break;
            case XDG_WM_BASE_ERROR_UNRESPONSIVE:
                AZ_Error("XDG", false, "Didn't ping in time");
                break;
            default:
                AZ_Error("XDG", false, "Unknown error");
                break;
            }
        }
        else if (registryId == WL_GET_PROXY_ID(m_decor))
        {
            switch (errorCode)
            {
            case ZXDG_TOPLEVEL_DECORATION_V1_ERROR_UNCONFIGURED_BUFFER:
                AZ_Error("XDG Decor", false, "TopLevel has a buffer attached before configure.");
                break;
            case ZXDG_TOPLEVEL_DECORATION_V1_ERROR_ALREADY_CONSTRUCTED:
                AZ_Error("XDG Decor", false, "TopLevel already has a decoration object.");
                break;
            case ZXDG_TOPLEVEL_DECORATION_V1_ERROR_ORPHANED:
                AZ_Error("XDG Decor", false, "TopLevel destroyed before the decoration object.");
                break;
            default:
                AZ_Error("XDG Decor", false, "Unknown error");
                break;
            }
        }
    }

    wl_proxy* XdgManagerImpl::GetProxy(AZ::Crc32 interface)
    {
        if (interface == XdgWmBaseName)
        {
            return reinterpret_cast<wl_proxy*>(m_xdg);
        }
        if (interface == XdgDecorationManagerName)
        {
            return reinterpret_cast<wl_proxy*>(m_decor);
        }
        return nullptr;
    }

    void XdgManagerImpl::XdgPing(void*, xdg_wm_base* xdg, uint32_t serial)
    {
        // You ping, I pong :)
        xdg_wm_base_pong(xdg, serial);
    }
} // namespace AzFramework
