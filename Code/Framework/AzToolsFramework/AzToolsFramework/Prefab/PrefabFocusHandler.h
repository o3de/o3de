/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework::Prefab
{
    class InstanceEntityMapperInterface;

    //! Handles Prefab Focus mode, determining which prefab file entity changes will target.
    class PrefabFocusHandler final
        : private PrefabFocusInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabFocusHandler, AZ::SystemAllocator, 0);

        PrefabFocusHandler();
        ~PrefabFocusHandler();

        // PrefabFocusInterface override ...
        PrefabFocusOperationResult FocusOnOwningPrefab(AZ::EntityId entityId) override;
        TemplateId GetFocusedPrefabTemplateId() override;
        InstanceOptionalReference GetFocusedPrefabInstance() override;
        bool IsOwningPrefabBeingFocused(AZ::EntityId entityId) override;

    private:
        InstanceOptionalReference m_focusedInstance;
        TemplateId m_focusedTemplateId;

        InstanceEntityMapperInterface* m_instanceEntityMapperInterface;
    };

} // namespace AzToolsFramework::Prefab
