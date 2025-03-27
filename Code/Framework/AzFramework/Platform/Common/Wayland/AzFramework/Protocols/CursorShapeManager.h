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
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

#include <cursor-shape-v1-client-protocol.h>
#include <AzFramework/WaylandInterface.h>

namespace AzFramework
{
    class CursorShapeManager
    {
    public:
        AZ_RTTI(CursorShapeManager, "{569EF165-AB9D-4F81-8E79-CE0E69600B8F}");

        virtual ~CursorShapeManager() = default;

        virtual wp_cursor_shape_device_v1* GetCursorShapeDevice(wl_pointer* pointer) = 0;
    };

    using CursorShapeManagerInterface = AZ::Interface<CursorShapeManager>;

    class CursorShapeManagerImpl
        : public WaylandRegistryEventsBus::Handler
        , public CursorShapeManager
    {
    public:
        AZ_CLASS_ALLOCATOR(CursorShapeManagerImpl, AZ::SystemAllocator);
        CursorShapeManagerImpl();
        ~CursorShapeManagerImpl() override;

        void OnRegister(wl_registry* registry, uint32_t id, const char* interface, uint32_t version) override;
        void OnUnregister(wl_registry* registry, uint32_t id) override;

        wp_cursor_shape_device_v1* GetCursorShapeDevice(wl_pointer* pointer) override;

    private:
        wp_cursor_shape_manager_v1* m_cursorManager = nullptr;
        uint32_t m_cursorManagerId = 0;
    };
} // namespace AzFramework