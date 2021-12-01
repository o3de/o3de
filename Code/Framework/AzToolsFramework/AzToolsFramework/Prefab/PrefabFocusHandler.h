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
#include <AzToolsFramework/FocusMode/FocusModeInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicNotificationBus.h>
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
        : public PrefabFocusPublicRequestBus::Handler
        , private PrefabFocusInterface
        , private PrefabPublicNotificationBus::Handler
        , private EditorEntityContextNotificationBus::Handler
        , private EditorEntityInfoNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(PrefabFocusHandler, AZ::SystemAllocator, 0);

        PrefabFocusHandler();
        ~PrefabFocusHandler();

        static void Reflect(AZ::ReflectContext* context);

        // PrefabFocusInterface overrides ...
        void InitializeEditorInterfaces() override;
        PrefabFocusOperationResult FocusOnPrefabInstanceOwningEntityId(AZ::EntityId entityId) override;
        TemplateId GetFocusedPrefabTemplateId(AzFramework::EntityContextId entityContextId) const override;
        InstanceOptionalReference GetFocusedPrefabInstance(AzFramework::EntityContextId entityContextId) const override;

        // PrefabFocusPublicInterface and PrefabFocusPublicRequestBus overrides ...
        PrefabFocusOperationResult FocusOnOwningPrefab(AZ::EntityId entityId) override;
        PrefabFocusOperationResult FocusOnParentOfFocusedPrefab(AzFramework::EntityContextId entityContextId) override;
        PrefabFocusOperationResult FocusOnPathIndex(AzFramework::EntityContextId entityContextId, int index) override;
        AZ::EntityId GetFocusedPrefabContainerEntityId(AzFramework::EntityContextId entityContextId) const override;
        bool IsOwningPrefabBeingFocused(AZ::EntityId entityId) const override;
        bool IsOwningPrefabInFocusHierarchy(AZ::EntityId entityId) const override;
        const AZ::IO::Path& GetPrefabFocusPath(AzFramework::EntityContextId entityContextId) const override;
        const int GetPrefabFocusPathLength(AzFramework::EntityContextId entityContextId) const override;

        // EditorEntityContextNotificationBus overrides ...
        void OnContextReset() override;
        
        // EditorEntityInfoNotificationBus overrides ...
        void OnEntityInfoUpdatedName(AZ::EntityId entityId, const AZStd::string& name) override;

        // PrefabPublicNotifications overrides ...
        void OnPrefabInstancePropagationEnd() override;
        void OnPrefabTemplateDirtyFlagUpdated(TemplateId templateId, bool status) override;
        
    private:
        PrefabFocusOperationResult FocusOnPrefabInstance(InstanceOptionalReference focusedInstance);
        void RefreshInstanceFocusList();
        void RefreshInstanceFocusPath();

        void OpenInstanceContainers(const AZStd::vector<AZ::EntityId>& instances) const;
        void CloseInstanceContainers(const AZStd::vector<AZ::EntityId>& instances) const;

        InstanceOptionalReference GetReferenceFromContainerEntityId(AZ::EntityId containerEntityId) const;

        //! The EntityId of the prefab container entity for the instance the editor is currently focusing on.
        AZ::EntityId m_focusedInstanceContainerEntityId = AZ::EntityId();
        //! The templateId of the focused instance.
        TemplateId m_focusedTemplateId;
        //! The list of instances going from the root (index 0) to the focused instance,
        //! referenced by their prefab container's EntityId.
        AZStd::vector<AZ::EntityId> m_instanceFocusHierarchy;
        //! A path containing the filenames of the instances in the focus hierarchy, separated with a /.
        AZ::IO::Path m_instanceFocusPath;

        ContainerEntityInterface* m_containerEntityInterface = nullptr;
        FocusModeInterface* m_focusModeInterface = nullptr;
        InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
    };

} // namespace AzToolsFramework::Prefab
