/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Protocols/RelativePointerManager.h>

namespace AzFramework
{
    RelativePointerManagerImpl::RelativePointerManagerImpl()
    {
        WaylandRegistryEventsBus::Handler::BusConnect();
    }

    RelativePointerManagerImpl::~RelativePointerManagerImpl()
    {
        WaylandRegistryEventsBus::Handler::BusDisconnect();
    }

    void RelativePointerManagerImpl::OnRegister(wl_registry* registry, uint32_t id, const AZ::Crc32 interface, uint32_t version)
    {
        if (interface != AZ_CRC_CE("zwp_relative_pointer_manager_v1"))
        {
            return;
        }

        m_relativePointerManager = static_cast<zwp_relative_pointer_manager_v1*>(
            wl_registry_bind(registry, id, &zwp_relative_pointer_manager_v1_interface, version));
        m_relativePointerManagerId = id;

        RelativePointerManagerInterface::Register(this);
    }

    void RelativePointerManagerImpl::OnUnregister(wl_registry* registry, uint32_t id)
    {
        if (m_relativePointerManagerId != id)
        {
            return;
        }

        zwp_relative_pointer_manager_v1_destroy(m_relativePointerManager);
        m_relativePointerManager = nullptr;
        m_relativePointerManagerId = 0;

        if (RelativePointerManagerInterface::Get() == this)
        {
            RelativePointerManagerInterface::Unregister(this);
        }
    }

    zwp_relative_pointer_v1* RelativePointerManagerImpl::GetRelativePointer(wl_pointer* pointer)
    {
        if (m_relativePointerManager == nullptr)
        {
            return nullptr;
        }

        return zwp_relative_pointer_manager_v1_get_relative_pointer(m_relativePointerManager, pointer);
    }
} // namespace AzFramework
