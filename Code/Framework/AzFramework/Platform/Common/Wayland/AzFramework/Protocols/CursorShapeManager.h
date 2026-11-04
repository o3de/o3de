/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

#include "CursorShapeInterface.h"
#include "AzFramework/WaylandInterface.h"

struct wl_shm;
struct wp_cursor_shape_manager_v1;
struct wl_cursor;
struct wl_cursor_theme;

namespace AzFramework
{
    //This provides the implementation for both CursorShape and the default `wl_pointer.set_cursor`.
    //While we could separate the implementations, the protocols are pretty simple.
    class CursorShapeManager final
        : protected WaylandRegistryEventsBus::Handler
        , protected CursorShapeInterfaceRequests
    {
    public:
        AZ_CLASS_ALLOCATOR(CursorShapeManager, AZ::SystemAllocator);
        CursorShapeManager();
        ~CursorShapeManager() override;

        // WaylandRegistryEventsBus
        void OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version) override;
        void OnUnregister(wl_registry* registry, uint32_t id) override;
        void OnRegistryFinished() override;

        // CursorShapeInterfaceRequests
        void RegisterPointer(wl_pointer* pointer) override;
        void UnregisterPointer(wl_pointer* pointer) override;
        void SetCursorShape(wl_pointer* pointer, uint serial, wp_cursor_shape_device_v1_shape shape) override;

    private:
        wp_cursor_shape_manager_v1* m_cursorManager = nullptr;

        wl_shm* m_shm = nullptr;
        wl_cursor_theme* m_cursorTheme = nullptr;

        struct PointerMetadata
        {
            //CursorShape
            wp_cursor_shape_device_v1* m_shapeDevice = nullptr;
            //wayland-cursor
            wl_surface* m_surface = nullptr;
        };
        AZStd::unordered_map<wl_pointer*, PointerMetadata> m_pointerDevices;
    };
} // namespace AzFramework
