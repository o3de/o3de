/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabFocusHandler.h>

#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>
#include <AzToolsFramework/Prefab/PrefabFocusNotificationBus.h>

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

        EditorEntityContextNotificationBus::Handler::BusConnect();
        AZ::Interface<PrefabFocusInterface>::Register(this);
    }

    PrefabFocusHandler::~PrefabFocusHandler()
    {
        AZ::Interface<PrefabFocusInterface>::Unregister(this);
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnOwningPrefab(AZ::EntityId entityId)
    {
        InstanceOptionalReference focusedInstance;

        if (!entityId.IsValid())
        {
            PrefabEditorEntityOwnershipInterface* prefabEditorEntityOwnershipInterface =
                AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();

            if(!prefabEditorEntityOwnershipInterface)
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

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPathIndex([[maybe_unused]] AzFramework::EntityContextId entityContextId, int index)
    {
        if (index < 0 || index >= m_instanceFocusVector.size())
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: Invalid index on FocusOnPathIndex."));
        }

        InstanceOptionalReference focusedInstance = m_instanceFocusVector[index];

        return FocusOnPrefabInstance(focusedInstance);
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnPrefabInstance(InstanceOptionalReference focusedInstance)
    {
        if (!focusedInstance.has_value())
        {
            return AZ::Failure(AZStd::string("Prefab Focus Handler: invalid instance to focus on."));
        }

        if (!m_focusedInstance.has_value() || &m_focusedInstance->get() != &focusedInstance->get())
        {
            m_focusedInstance = focusedInstance;
            m_focusedTemplateId = focusedInstance->get().GetTemplateId();

            AZ::EntityId containerEntityId;

            if (focusedInstance->get().GetParentInstance() != AZStd::nullopt)
            {
                containerEntityId = focusedInstance->get().GetContainerEntityId();

                // Select the container entity
                AzToolsFramework::SelectEntity(containerEntityId);
            }
            else
            {
                containerEntityId = AZ::EntityId();

                // Clear the selection
                AzToolsFramework::SelectEntities({});
                
            }
            
            // Focus on the descendants of the container entity
            if (FocusModeInterface* focusModeInterface = AZ::Interface<FocusModeInterface>::Get())
            {
                focusModeInterface->SetFocusRoot(containerEntityId);
            }

            RefreshInstanceFocusList();
            PrefabFocusNotificationBus::Broadcast(&PrefabFocusNotifications::OnPrefabFocusChanged);
        }

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

    const AZ::IO::Path& PrefabFocusHandler::GetPrefabFocusPath([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return m_instanceFocusPath;
    }

    const int PrefabFocusHandler::GetPrefabFocusPathLength([[maybe_unused]] AzFramework::EntityContextId entityContextId) const
    {
        return aznumeric_cast<int>(m_instanceFocusVector.size());
    }

    void PrefabFocusHandler::OnEntityStreamLoadSuccess()
    {
        // Focus on the root prefab (AZ::EntityId() will default to it)
        FocusOnOwningPrefab(AZ::EntityId());
    }

    void PrefabFocusHandler::RefreshInstanceFocusList()
    {
        m_instanceFocusVector.clear();
        m_instanceFocusPath.clear();

        AZStd::list<InstanceOptionalReference> instanceFocusList;

        // Use a support list to easily push front while traversing the prefab hierarchy
        InstanceOptionalReference currentInstance = m_focusedInstance;
        while (currentInstance.has_value())
        {
            instanceFocusList.push_front(currentInstance);

            currentInstance = currentInstance->get().GetParentInstance();
        }

        // Populate internals using the support list
        for (auto& instance : instanceFocusList)
        {
            m_instanceFocusPath.Append(instance->get().GetContainerEntity()->get().GetName());
            m_instanceFocusVector.emplace_back(instance);
        }
    }

} // namespace AzToolsFramework::Prefab
