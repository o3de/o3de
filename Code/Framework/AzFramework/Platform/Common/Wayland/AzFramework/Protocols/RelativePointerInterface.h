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

#include <relative-pointer-unstable-v1-client-protocol.h>
#include <AzFramework/WaylandInterface.h>

namespace AzFramework
{
    class RelativePointerInterfaceRequests
    {
    public:
        AZ_RTTI(RelativePointerInterfaceRequests, "{AA4CD0C4-0140-406C-B2A7-A39AEFD88346}");
        virtual ~RelativePointerInterfaceRequests() = default;

        virtual zwp_relative_pointer_v1* GetRelativePointer(wl_pointer* pointer) = 0;
    };

    using RelativePointerInterface = AZ::Interface<RelativePointerInterfaceRequests>;
} // namespace AzFramework
