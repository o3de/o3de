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
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework
{
    class ContainerEntityInterface;
    class FocusModeInterface;
}

namespace AzToolsFramework::Prefab
{
    class InstanceEntityMapperInterface;

    //! Handles Prefab Focus mode, determining which prefab file entity changes will target.
    class PrefabFocusHandler final
        : private PrefabFocusInterface
        , private PrefabFocusPublicInterface
        , private EditorEntityContextNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabFocusHandler, AZ::SystemAllocator, 0);

        PrefabFocusHandler();
        ~PrefabFocusHandler();

        void Initialize();

        // PrefabFocusInterface overrides ...
        PrefabFocusOperationResult FocusOnPrefabInstanceOwningEntityId(AZ::EntityId entityId) override;
        TemplateId GetFocusedPrefabTemplateId(AzFramework::EntityContextId entityContextId) const override;
        InstanceOptionalReference GetFocusedPrefabInstance(AzFramework::EntityContextId entityContextId) const override;

        // PrefabFocusPublicInterface overrides ...
        PrefabFocusOperationResult FocusOnOwningPrefab(AZ::EntityId entityId) override;
        PrefabFocusOperationResult FocusOnPathIndex(AzFramework::EntityContextId entityContextId, int index) override;
        AZ::EntityId GetFocusedPrefabContainerEntityId(AzFramework::EntityContextId entityContextId) const override;
        bool IsOwningPrefabBeingFocused(AZ::EntityId entityId) const override;
        const AZ::IO::Path& GetPrefabFocusPath(AzFramework::EntityContextId entityContextId) const override;
        const int GetPrefabFocusPathLength(AzFramework::EntityContextId entityContextId) const override;

        // EditorEntityContextNotificationBus overrides ...
        void OnEntityStreamLoadSuccess() override;

    private:
        PrefabFocusOperationResult FocusOnPrefabInstance(InstanceOptionalReference focusedInstance);
        void RefreshInstanceFocusList();

        void OpenInstanceContainers(const AZStd::vector<InstanceOptionalReference>& instances) const;
        void CloseInstanceContainers(const AZStd::vector<InstanceOptionalReference>& instances) const;

        InstanceOptionalReference m_focusedInstance;
        TemplateId m_focusedTemplateId;
        AZStd::vector<InstanceOptionalReference> m_instanceFocusVector;
        AZ::IO::Path m_instanceFocusPath;

        ContainerEntityInterface* m_containerEntityInterface = nullptr;
        FocusModeInterface* m_focusModeInterface = nullptr;
        InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;

        bool m_isInitialized = false;
    };

} // namespace AzToolsFramework::Prefab
