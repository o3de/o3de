/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Protocols/ProtocolManager.h>
#include <AzFramework/WaylandProtocolNames.h>
#include <pointer-constraints-unstable-v1-client-protocol.h>

namespace AzFramework
{
    ProtocolManager::ProtocolManager()
    {
        WaylandRegistryEventsBus::Handler::BusConnect();
    }

    ProtocolManager::~ProtocolManager()
    {
        WaylandRegistryEventsBus::Handler::BusDisconnect();
        if (m_constraintsManager != nullptr)
        {
            zwp_pointer_constraints_v1_destroy(m_constraintsManager);
        }
        if (m_relativePointerManager != nullptr)
        {
            zwp_relative_pointer_manager_v1_destroy(m_relativePointerManager);
        }
    }

    void ProtocolManager::OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version)
    {
        if (interface == PointerConstraintsName && version >= PointerConstraintsVersion)
        {
            m_constraintsManager =
                static_cast<zwp_pointer_constraints_v1*>(wl_registry_bind(
                    registry, id, &zwp_pointer_constraints_v1_interface, PointerConstraintsVersion));
            WaylandProxyBus::MultiHandler::BusConnect(PointerConstraintsName);
            return;
        }

        if (interface == RelativePointerManagerName && version >= RelativePointerManagerVersion)
        {
            m_relativePointerManager =
                static_cast<zwp_relative_pointer_manager_v1*>(wl_registry_bind(
                    registry, id, &zwp_relative_pointer_manager_v1_interface, RelativePointerManagerVersion));
            WaylandProxyBus::MultiHandler::BusConnect(RelativePointerManagerName);
            return;
        }
    }

    void ProtocolManager::OnUnregister(wl_registry* registry, uint32_t id)
    {
        if (id == WL_GET_PROXY_ID(m_constraintsManager))
        {
            zwp_pointer_constraints_v1_destroy(m_constraintsManager);
            m_constraintsManager = nullptr;
            WaylandProxyBus::MultiHandler::BusDisconnect(PointerConstraintsName);
            return;
        }

        if (id == WL_GET_PROXY_ID(m_relativePointerManager))
        {
            zwp_relative_pointer_manager_v1_destroy(m_relativePointerManager);
            m_relativePointerManager = nullptr;
            WaylandProxyBus::MultiHandler::BusDisconnect(RelativePointerManagerName);
            return;
        }
    }

    wl_proxy* ProtocolManager::GetProxy(AZ::Crc32 interface)
    {
        if (interface == PointerConstraintsName)
        {
            return reinterpret_cast<wl_proxy*>(m_constraintsManager);
        }

        if (interface == RelativePointerManagerName)
        {
            return reinterpret_cast<wl_proxy*>(m_relativePointerManager);
        }

        return nullptr;
    }

    zwp_relative_pointer_v1* ProtocolManager::GetRelativePointer(wl_pointer* pointer)
    {
        if (m_relativePointerManager == nullptr || pointer == nullptr)
        {
            return nullptr;
        }

        return zwp_relative_pointer_manager_v1_get_relative_pointer(m_relativePointerManager, pointer);
    }
} // namespace AzFramework
