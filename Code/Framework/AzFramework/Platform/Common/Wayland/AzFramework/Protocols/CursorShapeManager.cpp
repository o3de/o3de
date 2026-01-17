/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Protocols/CursorShapeManager.h>

namespace AzFramework
{
    CursorShapeManagerImpl::CursorShapeManagerImpl()
    {
        WaylandRegistryEventsBus::Handler::BusConnect();
    }

    CursorShapeManagerImpl::~CursorShapeManagerImpl()
    {
        WaylandRegistryEventsBus::Handler::BusDisconnect();

        if (CursorShapeManagerInterface::Get() == this)
        {
            CursorShapeManagerInterface::Unregister(this);
        }
    }

    void CursorShapeManagerImpl::OnRegister(wl_registry* registry, uint32_t id, const AZ::Crc32 interface, uint32_t version)
    {
        if (interface != AZ_CRC_CE("wp_cursor_shape_manager_v1"))
        {
            return;
        }

        m_cursorManager =
            static_cast<wp_cursor_shape_manager_v1*>(wl_registry_bind(registry, id, &wp_cursor_shape_manager_v1_interface, version));
        m_cursorManagerId = id;

        CursorShapeManagerInterface::Register(this);
    }

    void CursorShapeManagerImpl::OnUnregister(wl_registry* registry, uint32_t id)
    {
        if (id != m_cursorManagerId)
        {
            return;
        }

        wp_cursor_shape_manager_v1_destroy(m_cursorManager);
        m_cursorManager = nullptr;
        m_cursorManagerId = 0;

        if (CursorShapeManagerInterface::Get() == this)
        {
            CursorShapeManagerInterface::Unregister(this);
        }
    }

    wp_cursor_shape_device_v1* CursorShapeManagerImpl::GetCursorShapeDevice(wl_pointer* pointer)
    {
        if (m_cursorManager == nullptr)
        {
            return nullptr;
        }

        return wp_cursor_shape_manager_v1_get_pointer(m_cursorManager, pointer);
    }
} // namespace AzFramework