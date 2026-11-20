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
    //Provides a generic way to set the shape of a pointing device.
    class CursorShapeInterfaceRequests
    {
    public:
        AZ_RTTI(CursorShapeInterfaceRequests, "{569EF165-AB9D-4F81-8E79-CE0E69600B8F}");
        virtual ~CursorShapeInterfaceRequests() = default;

        virtual void RegisterPointer(wl_pointer* pointer) = 0;
        virtual void UnregisterPointer(wl_pointer* pointer) = 0;

        virtual void SetCursorShape(wl_pointer* pointer, uint serial, wp_cursor_shape_device_v1_shape shape) = 0;
    };

    using CursorShapeInterface = AZ::Interface<CursorShapeInterfaceRequests>;
} // namespace AzFramework
