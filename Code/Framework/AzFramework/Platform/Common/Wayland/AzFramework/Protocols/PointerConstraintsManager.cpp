/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Protocols/PointerConstraintsManager.h>

namespace AzFramework
{
    PointerConstraintsManagerImpl::PointerConstraintsManagerImpl()
    {
        WaylandRegistryEventsBus::Handler::BusConnect();
    }

    PointerConstraintsManagerImpl::~PointerConstraintsManagerImpl()
    {
        WaylandRegistryEventsBus::Handler::BusDisconnect();
    }

    void PointerConstraintsManagerImpl::OnRegister(wl_registry* registry, uint32_t id, const AZ::Crc32 interface, uint32_t version)
    {
        if (interface != AZ_CRC_CE("zwp_pointer_constraints_v1"))
        {
            return;
        }

        m_constraintsManager =
            static_cast<zwp_pointer_constraints_v1*>(wl_registry_bind(registry, id, &zwp_pointer_constraints_v1_interface, version));
        m_constraintsManagerId = id;

        PointerConstraintsManagerInterface::Register(this);
    }

    void PointerConstraintsManagerImpl::OnUnregister(wl_registry* registry, uint32_t id)
    {
        if (m_constraintsManagerId != id)
        {
            return;
        }

        zwp_pointer_constraints_v1_destroy(m_constraintsManager);
        m_constraintsManager = nullptr;
        m_constraintsManagerId = 0;

        if (PointerConstraintsManagerInterface::Get() == this)
        {
            PointerConstraintsManagerInterface::Unregister(this);
        }
    }

    zwp_pointer_constraints_v1* PointerConstraintsManagerImpl::GetConstraints()
    {
        return m_constraintsManager;
    }
} // namespace AzFramework