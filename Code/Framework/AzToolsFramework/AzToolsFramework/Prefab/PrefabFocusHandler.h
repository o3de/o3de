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
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework
{
    class ContainerEntityInterface;
    class FocusModeInterface;
    class ReadOnlyEntityPublicInterface;
    class ReadOnlyEntityQueryInterface;
}

namespace AzToolsFramework::Prefab
{
    class InstanceEntityMapperInterface;
    class InstanceToTemplateInterface;
    class InstanceUpdateExecutorInterface;
    class PrefabSystemComponentInterface;

    //! Handles Prefab Focus mode, determining which prefab file entity changes will target.
    class PrefabFocusHandler final
        : public PrefabFocusPublicRequestBus::Handler
        , private PrefabFocusInterface
        , private PrefabPublicNotificationBus::Handler
        , private EditorEntityContextNotificationBus::Handler
        , private EditorEntityInfoNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabFocusHandler, AZ::SystemAllocator);

        void RegisterPrefabFocusInterface();
        void UnregisterPrefabFocusInterface();

        static void Reflect(AZ::ReflectContext* context);

        // PrefabFocusInterface overrides ...
        void InitializeEditorInterfaces() override;

        PrefabFocusOperationResult FocusOnPrefabInstanceOwningEntityId(AZ::EntityId entityId) override;
        TemplateId GetFocusedPrefabTemplateId(AzFramework::EntityContextId entityContextId) const override;
        InstanceOptionalReference GetFocusedPrefabInstance(AzFramework::EntityContextId entityContextId) const override;
        bool IsFocusedPrefabInstanceReadOnly(AzFramework::EntityContextId entityContextId) const override;
        LinkId PrependPathFromFocusedInstanceToPatchPaths(PrefabDom& patches, AZ::EntityId entityId) const override;

        // PrefabFocusPublicInterface and PrefabFocusPublicRequestBus overrides ...
        PrefabFocusOperationResult FocusOnOwningPrefab(AZ::EntityId entityId) override;
        PrefabFocusOperationResult FocusOnParentOfFocusedPrefab(AzFramework::EntityContextId entityContextId) override;
        PrefabFocusOperationResult FocusOnPathIndex(AzFramework::EntityContextId entityContextId, int index) override;
        PrefabFocusOperationResult SetOwningPrefabInstanceOpenState(AZ::EntityId entityId, bool openState) override;
        AZ::EntityId GetFocusedPrefabContainerEntityId(AzFramework::EntityContextId entityContextId) const override;
        
        bool IsOwningPrefabBeingFocused(AZ::EntityId entityId) const override;
        bool IsOwningPrefabInFocusHierarchy(AZ::EntityId entityId) const override;
        const AZ::IO::Path& GetPrefabFocusPath(AzFramework::EntityContextId entityContextId) const override;
        const int GetPrefabFocusPathLength(AzFramework::EntityContextId entityContextId) const override;
        void SetPrefabEditScope(AzFramework::EntityContextId entityContextId, PrefabEditScope mode) override;

        // EditorEntityContextNotificationBus overrides ...
        void OnPrepareForContextReset() override;
        
        // EditorEntityInfoNotificationBus overrides ...
        void OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name) override;

        // PrefabPublicNotifications overrides ...
        void OnPrefabInstancePropagationEnd() override;
        void OnPrefabTemplateDirtyFlagUpdated(TemplateId templateId, bool status) override;
        
    private:
        InstanceClimbUpResult ClimbUpToFocusedOrRootInstanceFromEntity(AZ::EntityId entityId) const;

        PrefabFocusOperationResult FocusOnPrefabInstance(InstanceOptionalReference focusedInstance);
        void RefreshInstanceFocusPath();

        void SetInstanceContainersOpenState(const RootAliasPath& rootAliasPath, bool openState) const;
        void SetInstanceContainersOpenStateOfAllDescendantContainers(InstanceOptionalReference instance, bool openState) const;

        void SwitchToEditScope() const;

        InstanceOptionalReference GetInstanceReference(RootAliasPath rootAliasPath) const;

        //! The alias path for the instance the editor is currently focusing on, starting from the root instance.
        RootAliasPath m_rootAliasFocusPath = RootAliasPath();
        //! A path containing the filenames of the instances in the focus hierarchy, separated with a /.
        AZ::IO::Path m_filenameFocusPath;
        //! The length of the current focus path. Stored to simplify internal checks.
        int m_rootAliasFocusPathLength = 0;
        //! The current focus mode.
        PrefabEditScope m_prefabEditScope = PrefabEditScope::HIDE_NESTED_INSTANCES_CONTENT;

        InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
        InstanceUpdateExecutorInterface* m_instanceUpdateExecutorInterface = nullptr;
        InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;

        ContainerEntityInterface* m_containerEntityInterface = nullptr;
        FocusModeInterface* m_focusModeInterface = nullptr;
        ReadOnlyEntityPublicInterface* m_readOnlyEntityPublicInterface = nullptr;
        ReadOnlyEntityQueryInterface* m_readOnlyEntityQueryInterface = nullptr;
    };

} // namespace AzToolsFramework::Prefab
