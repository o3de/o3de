/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabFocusHandler.h>

#include <AzToolsFramework/Commands/SelectionCommand.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityInterface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusUndo.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework::Prefab
{
    PrefabFocusHandler::PrefabFocusHandler()
    {
        m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
        AZ_Assert(
            m_instanceEntityMapperInterface,
            "Prefab - PrefabFocusHandler - "
            "Instance Entity Mapper Interface could not be found. "
            "Check that it is being correctly initialized.");

        EditorEntityInfoNotificationBus::Handler::BusConnect();
        EditorEntityContextNotificationBus::Handler::BusConnect();
        PrefabPublicNotificationBus::Handler::BusConnect();
        AZ::Interface<PrefabFocusInterface>::Register(this);
        AZ::Interface<PrefabFocusPublicInterface>::Register(this);
        PrefabFocusPublicRequestBus::Handler::BusConnect();
    }

    PrefabFocusHandler::~PrefabFocusHandler()
    {
        PrefabFocusPublicRequestBus::Handler::BusDisconnect();
        AZ::Interface<PrefabFocusPublicInterface>::Unregister(this);
        AZ::Interface<PrefabFocusInterface>::Unregister(this);
        PrefabPublicNotificationBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
    }

    void PrefabFocusHandler::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context); behaviorContext)
        {
            behaviorContext->EBus<PrefabFocusPublicRequestBus>("PrefabFocusPublicRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Prefab")
                ->Attribute(AZ::Script::Attributes::Module, "prefab")
                ->Event("FocusOnOwningPrefab", &PrefabFocusPublicInterface::FocusOnOwningPrefab);
        }
    }

    void PrefabFocusHandler::InitializeEditorInterfaces()
    {
        m_containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
        AZ_Assert(
            m_containerEntityInterface,
            "Prefab - PrefabFocusHandler - "
            "Container Entity Interface could not be found. "
            "Check that it is being correctly initialized.");

        m_focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
        AZ_Assert(
            m_focusModeInterface,
            "Prefab - PrefabFocusHandler - "
            "Focus Mode Interface could not be found. "
            "Check that it is being correctly initialized.");

        m_readOnlyEntityQueryInterface = AZ::Interface<ReadOnlyEntityQueryInterface>::Get();
        AZ_Assert(
            m_readOnlyEntityQueryInterface,
            "Prefab - PrefabFocusHandler - "
            "ReadOnly Entity Query Interface could not be found. "
            "Check that it is being correctly initialized.");
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnOwningPrefab(AZ::EntityId entityId)
    {
        // Initialize Undo Batch object
        ScopedUndoBatch undoBatch("Edit Prefab");

        // Clear selection
        {
            const EntityIdList selectedEntities = EntityIdList{};
            auto selectionUndo = aznew SelectionCommand(selectedEntities, "Clear Selection");
            selectionUndo->SetParent(undoBatch.GetUndoBatch());
            ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::SetSelectedEntities, selectedEntities);
        }

        // Edit Prefab
        {
            auto editUndo = aznew PrefabFocusUndo("Edit Prefab");
            editUndo->Capture(entityId);
            editUndo->SetParent(undoBatch.GetUndoBatch());
            FocusOnPrefabInstanceOwningEntityId(entityId);
        }

        return AZ::Success();
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnParentOfFocusedPrefab(
        [[maybe_unused]] AzFramework::EntityContextId entityContextId)
    {
        // If only one instance is in the hierarchy, this operation is invalid
        size_t hierarchySize = m_instanceFocusHierarchy.size();
        if (hierarchySize <= 1)
        {
            return AZ::Failure(
                AZStd::string("Prefab Focus Handler: Could not complete FocusOnParentOfFocusedPrefab operation while focusing on the root."));
        }

        // Retrieve parent of currently focused prefab.
        InstanceOptionalReference parentInstance = GetReferenceFromContainerEntityId(m_instanceFocusHierarchy[hierarchySize - 2]);

        // Use container entity of parent Instance for focus operations.
        AZ::EntityId entityId = parentInstance->get().GetContainerEntityId();

        // Initialize Undo Batch object
        ScopedUndoBatch undoBatch("Edit Prefab");

        // Clear selection
        {
            const EntityIdList selectedEntities = EntityIdList{};
            auto selectionUndo = aznew SelectionCommand(selectedEntities, "Clear Selection");
            selectionUndo->SetParent(undoBatch.GetUndoBatch());
            ToolsApplicationRequestBus::Broadcast(&ToolsApplicationRequestBus::Events::SetSelectedEntities, selectedEntities);
        }

        // Edit Prefab
        {
            auto editUndo = aznew PrefabFocusUndo("Edit Prefab");
            editUndo->Capture(entityId);
            editUndo->SetParent(undoBatch.GetUndoBatch());
            FocusOnPrefabInstanceOwningEntityId(entityId);
        }

        return AZ::Success();
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPathIndex([[maybe_unused]] AzFramework::EntityContextId entityContextId, int index)
    {
        if (index < 0 || index >= m_instanceFocusHierarchy.size())
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: Invalid index on FocusOnPathIndex."));
        }

        InstanceOptionalReference focusedInstance = GetReferenceFromContainerEntityId(m_instanceFocusHierarchy[index]);

        return FocusOnOwningPrefab(focusedInstance->get().GetContainerEntityId());
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPrefabInstanceOwningEntityId(AZ::EntityId entityId)
    {
        InstanceOptionalReference focusedInstance;

        if (!entityId.IsValid())
        {
            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();

            if (!prefabEditorEntityOwnershipInterface)
            {
                return AZ::Failure(AZStd::string("Could not focus on root prefab instance - internal error "
                                                 "(PrefabEditorEntityOwnershipInterface unavailable)."));
            }

            focusedInstance = prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        }
        else
        {
            focusedInstance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        }

        return FocusOnPrefabInstance(focusedInstance);
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPrefabInstance(InstanceOptionalReference focusedInstance)
    {
        if (!focusedInstance.has_value())
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: invalid instance to focus on."));
        }

        // Close all container entities in the old path.
        CloseInstanceContainers(m_instanceFocusHierarchy);

        AZ::EntityId previousContainerEntityId = m_focusedInstanceContainerEntityId;

        // Do not store the container for the root instance, use an invalid EntityId instead.
        m_focusedInstanceContainerEntityId = focusedInstance->get().GetParentInstance().has_value() ? focusedInstance->get().GetContainerEntityId() : AZ::EntityId();
        m_focusedTemplateId = focusedInstance->get().GetTemplateId();

        // Focus on the descendants of the container entity in the Editor, if the interface is initialized.
        if (m_focusModeInterface)
        {
            const AZ::EntityId containerEntityId =
                (focusedInstance->get().GetParentInstance() != AZStd::nullopt)
                ? focusedInstance->get().GetContainerEntityId()
                : AZ::EntityId();

            m_focusModeInterface->SetFocusRoot(containerEntityId);
        }

        // Refresh the read-only cache, if the interface is initialized.
        if (m_readOnlyEntityQueryInterface)
        {
            m_readOnlyEntityQueryInterface->RefreshReadOnlyState({ previousContainerEntityId, m_focusedInstanceContainerEntityId });
        }

        // Refresh path variables.
        RefreshInstanceFocusList();
        RefreshInstanceFocusPath();

        // Open all container entities in the new path.
        OpenInstanceContainers(m_instanceFocusHierarchy);

        PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged);

        return AZ::Success();
    }
    
    TemplateId PrefabFocusHandler::GetFocusedPrefabTemplateId([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return m_focusedTemplateId;
    }

    InstanceOptionalReference PrefabFocusHandler::GetFocusedPrefabInstance(
        [[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return GetReferenceFromContainerEntityId(m_focusedInstanceContainerEntityId);
    }

    AZ::EntityId PrefabFocusHandler::GetFocusedPrefabContainerEntityId([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        if (m_focusedInstanceContainerEntityId.IsValid())
        {
            return m_focusedInstanceContainerEntityId;
        }

        if (auto instance = GetReferenceFromContainerEntityId(m_focusedInstanceContainerEntityId); instance.has_value())
        {
            return instance->get().GetContainerEntityId();
        }

        return AZ::EntityId();
    }

    bool PrefabFocusHandler::IsOwningPrefabBeingFocused(AZ::EntityId entityId) const
    {
        if (!entityId.IsValid())
        {
            return false;
        }

        InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);
        if (!instance.has_value())
        {
            return false;
        }

        // If this is owned by the root instance, that corresponds to an invalid m_focusedInstanceContainerEntityId.
        if (!instance->get().GetParentInstance().has_value())
        {
            return !m_focusedInstanceContainerEntityId.IsValid();
        }

        return (instance->get().GetContainerEntityId() == m_focusedInstanceContainerEntityId);
    }

    bool PrefabFocusHandler::IsOwningPrefabInFocusHierarchy(AZ::EntityId entityId) const
    {
        if (!entityId.IsValid())
        {
            return false;
        }

        // If the focus is on the root, m_focusedInstanceContainerEntityId will be the invalid id.
        // In those case all entities are in the focus hierarchy and should return true.
        if (!m_focusedInstanceContainerEntityId.IsValid())
        {
            return true;
        }

        InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

        while (instance.has_value())
        {
            if (instance->get().GetContainerEntityId() == m_focusedInstanceContainerEntityId)
            {
                return true;
            }

            instance = instance->get().GetParentInstance();
        }

        return false;
    }

    const AZ::IO::Path& PrefabFocusHandler::GetPrefabFocusPath([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return m_instanceFocusPath;
    }

    const int PrefabFocusHandler::GetPrefabFocusPathLength([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return aznumeric_cast<int>(m_instanceFocusHierarchy.size());
    }

    void PrefabFocusHandler::OnContextReset()
    {
        // Clear the old focus vector
        m_instanceFocusHierarchy.clear();

        // Focus on the root prefab (AZ::EntityId() will default to it)
        FocusOnPrefabInstanceOwningEntityId(AZ::EntityId());
    }

    void PrefabFocusHandler::OnEntityInfoUpdatedName(AZ::EntityId entityId, [[maybe_unused]]const AZStd::string& name)
    {
        // Determine if the entityId is the container for any of the instances in the vector.
        auto result = AZStd::find_if(
            m_instanceFocusHierarchy.begin(), m_instanceFocusHierarchy.end(),
            [&, entityId](const AZ::EntityId& containerEntityId)
            {
                InstanceOptionalReference instance = GetReferenceFromContainerEntityId(containerEntityId);
                return (instance->get().GetContainerEntityId() == entityId);
            }
        );

        if (result != m_instanceFocusHierarchy.end())
        {
            // Refresh the path and notify changes.
            RefreshInstanceFocusPath();
            PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged);
        }
    }

    void PrefabFocusHandler::OnPrefabInstancePropagationEnd()
    {
        // Refresh the path and notify changes in case propagation updated any container names.
        RefreshInstanceFocusPath();
        PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged);
    }

    void PrefabFocusHandler::OnPrefabTemplateDirtyFlagUpdated(TemplateId templateId, [[maybe_unused]] bool status)
    {
        // Determine if the templateId matches any of the instances in the vector.
        auto result = AZStd::find_if(
            m_instanceFocusHierarchy.begin(), m_instanceFocusHierarchy.end(),
            [&, templateId](const AZ::EntityId& containerEntityId)
            {
                InstanceOptionalReference instance = GetReferenceFromContainerEntityId(containerEntityId);
                return (instance->get().GetTemplateId() == templateId);
            }
        );

        if (result != m_instanceFocusHierarchy.end())
        {
            // Refresh the path and notify changes.
            RefreshInstanceFocusPath();
            PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged);
        }
    }

    void PrefabFocusHandler::RefreshInstanceFocusList()
    {
        m_instanceFocusHierarchy.clear();

        AZStd::list<InstanceOptionalReference> instanceFocusList;

        InstanceOptionalReference currentInstance = GetReferenceFromContainerEntityId(m_focusedInstanceContainerEntityId);
        while (currentInstance.has_value())
        {
            if (currentInstance->get().GetParentInstance().has_value())
            {
                m_instanceFocusHierarchy.emplace_back(currentInstance->get().GetContainerEntityId());
            }
            else
            {
                m_instanceFocusHierarchy.emplace_back(AZ::EntityId());
            }

            currentInstance = currentInstance->get().GetParentInstance();
        }

        // Invert the vector, since we need the top instance to be at index 0.
        AZStd::reverse(m_instanceFocusHierarchy.begin(), m_instanceFocusHierarchy.end());
    }

    void PrefabFocusHandler::RefreshInstanceFocusPath()
    {
        auto prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        m_instanceFocusPath.clear();

        size_t index = 0;
        size_t maxIndex = m_instanceFocusHierarchy.size() - 1;

        for (const AZ::EntityId& containerEntityId : m_instanceFocusHierarchy)
        {
            InstanceOptionalReference instance = GetReferenceFromContainerEntityId(containerEntityId);
            if (instance.has_value())
            {
                AZStd::string prefabName;

                if (index < maxIndex)
                {
                    // Get the filename without the extension (stem).
                    prefabName = instance->get().GetTemplateSourcePath().Stem().Native();
                }
                else
                {
                    // Get the full filename.
                    prefabName = instance->get().GetTemplateSourcePath().Filename().Native();
                }

                if (prefabSystemComponentInterface->IsTemplateDirty(instance->get().GetTemplateId()))
                {
                    prefabName += "*";
                }

                m_instanceFocusPath.Append(prefabName);
            }

            ++index;
        }
    }

    void PrefabFocusHandler::OpenInstanceContainers(const AZStd::vector<AZ::EntityId>& instances) const
    {
        // If this is called outside the Editor, this interface won't be initialized.
        if (!m_containerEntityInterface)
        {
            return;
        }
        
        for (const AZ::EntityId& containerEntityId : instances)
        {
            InstanceOptionalReference instance = GetReferenceFromContainerEntityId(containerEntityId);

            if (instance.has_value())
            {
                m_containerEntityInterface->SetContainerOpen(instance->get().GetContainerEntityId(), true);
            }
        }
    }

    void PrefabFocusHandler::CloseInstanceContainers(const AZStd::vector<AZ::EntityId>& instances) const
    {
        // If this is called outside the Editor, this interface won't be initialized.
        if (!m_containerEntityInterface)
        {
            return;
        }

        for (const AZ::EntityId& containerEntityId : instances)
        {
            InstanceOptionalReference instance = GetReferenceFromContainerEntityId(containerEntityId);

            if (instance.has_value())
            {
                m_containerEntityInterface->SetContainerOpen(instance->get().GetContainerEntityId(), false);
            }
        }
    }

    InstanceOptionalReference PrefabFocusHandler::GetReferenceFromContainerEntityId(AZ::EntityId containerEntityId) const
    {
        if (!containerEntityId.IsValid())
        {
            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();

            if (!prefabEditorEntityOwnershipInterface)
            {
                return AZStd::nullopt;
            }

            return prefabEditorEntityOwnershipInterface->GetRootPrefabInstance();
        }

        return m_instanceEntityMapperInterface->FindOwningInstance(containerEntityId);
    }

} // namespace AzToolsFramework::Prefab
