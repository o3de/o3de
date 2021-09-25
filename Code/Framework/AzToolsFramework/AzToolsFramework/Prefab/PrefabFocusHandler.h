/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework::Prefab
{
    class InstanceEntityMapperInterface;

    //! Handles Prefab Focus mode, determining which prefab file entity changes will target.
    class PrefabFocusHandler final
        : private PrefabFocusInterface
        , private EditorEntityContextNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabFocusHandler, AZ::SystemAllocator, 0);

        PrefabFocusHandler();
        ~PrefabFocusHandler();

        // PrefabFocusInterface overrides ...
        PrefabFocusOperationResult FocusOnOwningPrefab(AZ::EntityId entityId) override;
        PrefabFocusOperationResult FocusOnPathIndex(int index) override;
        TemplateId GetFocusedPrefabTemplateId() const override;
        InstanceOptionalReference GetFocusedPrefabInstance() const override;
        bool IsOwningPrefabBeingFocused(AZ::EntityId entityId) const override;
        const AZ::IO::Path& GetPrefabFocusPath() const override;
        const int GetPrefabFocusPathLength() const override;

        // EditorEntityContextNotificationBus overrides ...
        void OnEntityStreamLoadSuccess() override;

    private:
        PrefabFocusOperationResult FocusOnPrefabInstance(InstanceOptionalReference focusedInstance);
        void RefreshInstanceFocusList();

        InstanceOptionalReference m_focusedInstance;
        TemplateId m_focusedTemplateId;
        AZStd::vector<InstanceOptionalReference> m_instanceFocusVector;
        AZ::IO::Path m_instanceFocusPath;

        InstanceEntityMapperInterface* m_instanceEntityMapperInterface;
    };

} // namespace AzToolsFramework::Prefab
