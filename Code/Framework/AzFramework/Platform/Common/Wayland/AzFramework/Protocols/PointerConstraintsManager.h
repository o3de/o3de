/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "AzCore/Memory/SystemAllocator.h"

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

#include <pointer-constraints-unstable-v1-client-protocol.h>
#include <AzFramework/WaylandInterface.h>

namespace AzFramework
{
    class PointerConstraintsManager
    {
    public:
        AZ_RTTI(PointerConstraintsManager, "{C22DB3C9-6059-42D1-8D82-6FA2018FA078}");

        virtual ~PointerConstraintsManager() = default;

        virtual zwp_pointer_constraints_v1* GetConstraints() = 0;
    };

    using PointerConstraintsManagerInterface = AZ::Interface<PointerConstraintsManager>;

    class PointerConstraintsManagerImpl
        : public WaylandRegistryEventsBus::Handler
        , public PointerConstraintsManager
    {
    public:
        AZ_CLASS_ALLOCATOR(PointerConstraintsManagerImpl, AZ::SystemAllocator);
        PointerConstraintsManagerImpl();
        ~PointerConstraintsManagerImpl() override;

        void OnRegister(wl_registry* registry, uint32_t id, const char* interface, uint32_t version) override;
        void OnUnregister(wl_registry* registry, uint32_t id) override;

        zwp_pointer_constraints_v1* GetConstraints() override;

    private:
        zwp_pointer_constraints_v1* m_constraintsManager = nullptr;
        uint32_t m_constraintsManagerId = 0;
    };
} // namespace AzFramework