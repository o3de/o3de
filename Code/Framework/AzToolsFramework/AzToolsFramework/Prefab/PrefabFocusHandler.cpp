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
    }

    PrefabFocusHandler::~PrefabFocusHandler()
    {
        AZ::Interface<PrefabFocusPublicInterface>::Unregister(this);
        AZ::Interface<PrefabFocusInterface>::Unregister(this);
        PrefabPublicNotificationBus::Handler::BusDisconnect();
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        EditorEntityInfoNotificationBus::Handler::BusDisconnect();
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

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPathIndex([[maybe_unused]] AzFramework::EntityContextId entityContextId, int index)
    {
        if (index < 0 || index >= m_instanceFocusHierarchy.size())
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: Invalid index on FocusOnPathIndex."));
        }

        InstanceOptionalReference focusedInstance = m_instanceFocusHierarchy[index];

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

        m_focusedInstance = focusedInstance;
        m_focusedTemplateId = focusedInstance->get().GetTemplateId();

        AZ::EntityId containerEntityId;

        if (focusedInstance->get().GetParentInstance() != AZStd::nullopt)
        {
            containerEntityId = focusedInstance->get().GetContainerEntityId();
        }
        else
        {
            containerEntityId = AZ::EntityId();
        }
            
        // Focus on the descendants of the container entity in the Editor, if the interface is initialized.
        if (m_focusModeInterface)
        {
            m_focusModeInterface->SetFocusRoot(containerEntityId);
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
        return m_focusedInstance;
    }

    AZ::EntityId PrefabFocusHandler::GetFocusedPrefabContainerEntityId([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        if (!m_focusedInstance.has_value())
        {
            // PrefabFocusHandler has not been initialized yet.
            return AZ::EntityId();
        }

        return m_focusedInstance->get().GetContainerEntityId();
    }

    bool PrefabFocusHandler::IsOwningPrefabBeingFocused(AZ::EntityId entityId) const
    {
        if (!m_focusedInstance.has_value())
        {
            // PrefabFocusHandler has not been initialized yet.
            return false;
        }

        if (!entityId.IsValid())
        {
            return false;
        }

        InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

        return instance.has_value() && (&instance->get() == &m_focusedInstance->get());
    }

    bool PrefabFocusHandler::IsOwningPrefabInFocusHierarchy(AZ::EntityId entityId) const
    {
        if (!m_focusedInstance.has_value())
        {
            // PrefabFocusHandler has not been initialized yet.
            return false;
        }

        if (!entityId.IsValid())
        {
            return false;
        }

        InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

        while (instance.has_value())
        {
            if (&instance->get() == &m_focusedInstance->get())
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
            [entityId](const InstanceOptionalReference& instance)
            {
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
            [templateId](const InstanceOptionalReference& instance)
            {
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

        InstanceOptionalReference currentInstance = m_focusedInstance;
        while (currentInstance.has_value())
        {
            m_instanceFocusHierarchy.emplace_back(currentInstance);

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

        for (const InstanceOptionalReference& instance : m_instanceFocusHierarchy)
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

            ++index;
        }
    }

    void PrefabFocusHandler::OpenInstanceContainers(const AZStd::vector<InstanceOptionalReference>& instances) const
    {
        // If this is called outside the Editor, this interface won't be initialized.
        if (!m_containerEntityInterface)
        {
            return;
        }

        for (const InstanceOptionalReference& instance : instances)
        {
            if (instance.has_value())
            {
                m_containerEntityInterface->SetContainerOpen(instance->get().GetContainerEntityId(), true);
            }
        }
    }

    void PrefabFocusHandler::CloseInstanceContainers(const AZStd::vector<InstanceOptionalReference>& instances) const
    {
        // If this is called outside the Editor, this interface won't be initialized.
        if (!m_containerEntityInterface)
        {
            return;
        }

        for (const InstanceOptionalReference& instance : instances)
        {
            if (instance.has_value())
            {
                m_containerEntityInterface->SetContainerOpen(instance->get().GetContainerEntityId(), false);
            }
        }
    }

} // namespace AzToolsFramework::Prefab
