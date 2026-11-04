/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include "CursorShapeInterface.h"
#include "OutputManager.h"
#include "RelativePointerInterface.h"

struct zwp_pointer_constraints_v1;
struct zwp_relative_pointer_manager_v1;

namespace AzFramework
{
    //Handles registering simple protocols.
    class ProtocolManager final
        : protected WaylandRegistryEventsBus::Handler
        , protected WaylandProxyBus::MultiHandler
        , protected RelativePointerInterface::Registrar
    {
    public:
        AZ_CLASS_ALLOCATOR(ProtocolManager, AZ::SystemAllocator);
        ProtocolManager();
        ~ProtocolManager() override;

        // WaylandRegistryEventsBus
        void OnRegister(wl_registry* registry, uint32_t id, AZ::Crc32 interface, uint32_t version) override;
        void OnUnregister(wl_registry* registry, uint32_t id) override;

        // WaylandProxyBus
        wl_proxy* GetProxy(AZ::Crc32 interface) override;

        //RelativePointerInterface
        zwp_relative_pointer_v1* GetRelativePointer(wl_pointer* pointer) override;

    private:
        zwp_pointer_constraints_v1* m_constraintsManager = nullptr;
        zwp_relative_pointer_manager_v1* m_relativePointerManager = nullptr;
    };
} // namespace AzFramework
