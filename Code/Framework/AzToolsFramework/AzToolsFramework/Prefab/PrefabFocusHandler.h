/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework
{
    class PrefabEditorEntityOwnershipInterface;
}

namespace AzToolsFramework::FocusModeFramework
{
    class FocusModeInterface;
}

namespace AzToolsFramework::Prefab
{
    class InstanceEntityMapperInterface;

    class PrefabFocusHandler final
        : private PrefabFocusInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabFocusHandler, AZ::SystemAllocator, 0);

        ~PrefabFocusHandler();

        void Initialize();

        // PrefabFocusInterface...
        PrefabFocusOperationResult FocusOnOwningPrefab(AZ::EntityId entityId) override;
        TemplateId GetFocusedPrefabTemplateId() override;
        InstanceOptionalReference GetFocusedPrefabInstance() override;
        bool IsOwningPrefabBeingFocused(AZ::EntityId entityId) override;

    private:
        InstanceOptionalReference m_focusedInstance;
        TemplateId m_focusedTemplateId;

        static FocusModeFramework::FocusModeInterface* s_focusModeInterface;
        static InstanceEntityMapperInterface* s_instanceEntityMapperInterface;
        static PrefabEditorEntityOwnershipInterface* s_prefabEditorEntityOwnershipInterface;
    };

} // namespace AzToolsFramework::Prefab
