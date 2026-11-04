/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CursorShapeManager.h"
#include <AzCore/Console/IConsole.h>
#include <AzFramework/WaylandProtocolNames.h>
#include <AzFramework/WaylandConnectionManager.h>

#include <wayland-cursor.h>
#include <cursor-shape-v1-client-protocol.h>

namespace AzFramework
{
    AZ_CVAR(
       int,
       wl_cursorSize,
       12,
       nullptr,
       AZ::ConsoleFunctorFlags::Null,
       "WAYLAND ONLY: The cursor size used when cursor shape isn't supported by the compositor. Needs to be set at startup.");

    static AZStd::unordered_map<wp_cursor_shape_device_v1_shape, const char*> CursorShapeToString = {
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT, "left_ptr"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CONTEXT_MENU, "context-menu"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_HELP, "help"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER, "pointer"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_PROGRESS, "progress"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_WAIT, "wait"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CELL, "cell"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_CROSSHAIR, "crosshair"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT, "text"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_VERTICAL_TEXT, "vertical-text"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ALIAS, "alias"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_COPY, "copy"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_MOVE, "move"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NO_DROP, "no-drop"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NOT_ALLOWED, "not-allowed"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRAB, "grab"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_GRABBING, "grabbing"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_E_RESIZE, "e-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_N_RESIZE, "n-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NE_RESIZE, "ne-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NW_RESIZE, "nw-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_S_RESIZE, "s-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SE_RESIZE, "se-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_SW_RESIZE, "sw-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_W_RESIZE, "w-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_EW_RESIZE, "ew-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NS_RESIZE, "ns-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NESW_RESIZE, "nesw-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_NWSE_RESIZE, "nwse-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_COL_RESIZE, "col-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ROW_RESIZE, "row-resize"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ALL_SCROLL, "all-scroll"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ZOOM_IN, "zoom-in"},
        {WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_ZOOM_OUT, "zoom-out"}
    };
    
    CursorShapeManager::CursorShapeManager()
    {
        WaylandRegistryEventsBus::Handler::BusConnect();
        CursorShapeInterface::Register(this);
    }

    CursorShapeManager::~CursorShapeManager()
    {
        CursorShapeInterface::Unregister(this);
        WaylandRegistryEventsBus::Handler::BusDisconnect();
        if (m_cursorManager != nullptr)
        {
            wp_cursor_shape_manager_v1_destroy(m_cursorManager);
        }

        if (m_cursorTheme != nullptr)
        {
            wl_cursor_theme_destroy(m_cursorTheme);
        }
    }

    void CursorShapeManager::OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version)
    {
        if (interface == CursorShapeManagerName && version >= CursorShapeVersion)
        {
            m_cursorManager =
                static_cast<wp_cursor_shape_manager_v1*>(wl_registry_bind(
                    registry,
                    id,
                    &wp_cursor_shape_manager_v1_interface,
                    CursorShapeVersion));
            return;
        }

        if (interface == AZ_CRC_CE("wl_shm"))
        {
            m_shm = static_cast<wl_shm*>(wl_registry_bind(registry, id, &wl_shm_interface, 1));
            return;
        }
    }

    void CursorShapeManager::OnUnregister(wl_registry* registry, uint32_t id)
    {
        if (id == WL_GET_PROXY_ID(m_cursorManager))
        {
            wp_cursor_shape_manager_v1_destroy(m_cursorManager);
            m_cursorManager = nullptr;
            return;
        }

        if (id == WL_GET_PROXY_ID(m_shm))
        {
            wl_shm_destroy(m_shm);
            m_shm = nullptr;
        }
    }

    void CursorShapeManager::OnRegistryFinished()
    {
        if (m_cursorManager != nullptr)
        {
            //If we have cursor shape then we shouldn't waste resources fetching the cursor theme ourselves.
            return;
        }

        AZ_Assert(m_shm != nullptr, "Missing shm object, this is required for wayland-pointer.");

        m_cursorTheme = wl_cursor_theme_load(nullptr, wl_cursorSize, m_shm);
        AZ_Assert(m_cursorTheme != nullptr, "Failed to load cursor theme.");
    }

    void CursorShapeManager::RegisterPointer(wl_pointer* pointer)
    {
        AZ_Assert(pointer != nullptr, "Pointer cannot be null");
        auto& metadata = m_pointerDevices[pointer];

        if (m_cursorManager != nullptr)
        {
            metadata.m_shapeDevice = wp_cursor_shape_manager_v1_get_pointer(m_cursorManager, pointer);
        }
        else
        {
            auto connectionManager = AzFramework::WaylandConnectionManagerInterface::Get();
            AZ_Assert(connectionManager != nullptr, "Failed to get connection manager.");
            auto compositor = connectionManager->GetWaylandCompositor();
            AZ_Assert(compositor != nullptr, "Failed to get compositor.");

            metadata.m_surface = wl_compositor_create_surface(compositor);
        }
    }

    void CursorShapeManager::UnregisterPointer(wl_pointer* pointer)
    {
        AZ_Assert(pointer != nullptr, "Pointer cannot be null");
        AZ_Assert(m_pointerDevices.find(pointer) != m_pointerDevices.end(), "Pointer is not registered");

        auto& metadata = m_pointerDevices[pointer];
        if (metadata.m_shapeDevice != nullptr)
        {
            wp_cursor_shape_device_v1_destroy(metadata.m_shapeDevice);
            metadata.m_shapeDevice = nullptr;
        }
        else
        {
            wl_surface_destroy(metadata.m_surface);
        }

        m_pointerDevices.erase(pointer);
    }

    void CursorShapeManager::SetCursorShape(wl_pointer* pointer, uint serial, wp_cursor_shape_device_v1_shape shape)
    {
        AZ_Assert(pointer != nullptr, "Pointer cannot be null");
        AZ_Assert(m_pointerDevices.find(pointer) != m_pointerDevices.end(), "Pointer isn't registered");

        auto& metadata = m_pointerDevices[pointer];
        if (metadata.m_shapeDevice != nullptr)
        {
            wp_cursor_shape_device_v1_set_shape(metadata.m_shapeDevice, serial, shape);
        }
        else
        {
            wl_cursor* cursor = wl_cursor_theme_get_cursor(m_cursorTheme, CursorShapeToString[shape]);
            AZ_ErrorOnce("CursorShapeManager", cursor != nullptr, "Failed to get cursor shape %d", shape);
            if (cursor == nullptr)
            {
                return;
            }

            wl_cursor_image* image = cursor->images[0];
            AZ_Assert(image != nullptr, "Failed to get cursor image");

            wl_surface_attach(metadata.m_surface, wl_cursor_image_get_buffer(image), 0, 0);
            wl_surface_damage(metadata.m_surface, 0, 0, image->width, image->height);
            wl_pointer_set_cursor(pointer, serial, metadata.m_surface, image->hotspot_x, image->hotspot_y);

            wl_surface_commit(metadata.m_surface);
        }
    }
} // namespace AzFramework
