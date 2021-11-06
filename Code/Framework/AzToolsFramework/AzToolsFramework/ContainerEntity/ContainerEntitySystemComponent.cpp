/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ContainerEntity/ContainerEntitySystemComponent.h>

#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    void ContainerEntitySystemComponent::Activate()
    {
        AZ::Interface<ContainerEntityInterface>::Register(this);
        EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void ContainerEntitySystemComponent::Deactivate()
    {
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AZ::Interface<ContainerEntityInterface>::Unregister(this);
    }

    void ContainerEntitySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ContainerEntitySystemComponent, AZ::Component>()->Version(1);
        }
    }

    void ContainerEntitySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ContainerEntityService"));
    }

    ContainerEntityOperationResult ContainerEntitySystemComponent::RegisterEntityAsContainer(AZ::EntityId entityId)
    {
        if (IsContainer(entityId))
        {
            return AZ::Failure(AZStd::string(
                "ContainerEntitySystemComponent error - trying to register entity as container twice."));
        }

        m_containers.insert(entityId);

        return AZ::Success();
    }

    ContainerEntityOperationResult ContainerEntitySystemComponent::UnregisterEntityAsContainer(AZ::EntityId entityId)
    {
        if (!IsContainer(entityId))
        {
            return AZ::Failure(AZStd::string(
                "ContainerEntitySystemComponent error - trying to unregister entity that is not a container."));
        }

        m_containers.erase(entityId);

        return AZ::Success();
    }

    bool ContainerEntitySystemComponent::IsContainer(AZ::EntityId entityId) const
    {
        return m_containers.contains(entityId);
    }

    ContainerEntityOperationResult ContainerEntitySystemComponent::SetContainerOpen(AZ::EntityId entityId, bool open)
    {
        if (!IsContainer(entityId))
        {
            return AZ::Failure(AZStd::string(
                "ContainerEntitySystemComponent error - cannot set open state of entity that was not registered as container."));
        }

        if(open)
        {
            m_openContainers.insert(entityId);
        }
        else
        {
            m_openContainers.erase(entityId);
        }

        ContainerEntityNotificationBus::Broadcast(&ContainerEntityNotificationBus::Events::OnContainerEntityStatusChanged, entityId, open);

        return AZ::Success();
    }

    bool ContainerEntitySystemComponent::IsContainerOpen(AZ::EntityId entityId) const
    {
        // Non-container entities behave the same as open containers. This saves the caller an additional check.
        if(!m_containers.contains(entityId))
        {
            return true;
        }

        // If the entity is a container, return its state.
        return m_openContainers.contains(entityId);
    }

    AZ::EntityId ContainerEntitySystemComponent::FindHighestSelectableEntity(AZ::EntityId entityId) const
    {
        if (!entityId.IsValid())
        {
            return entityId;
        }

        // Return the highest closed container, or the entity if none is found.
        AZ::EntityId highestSelectableEntityId = entityId;

        // Skip the queried entity, as we only want to check its ancestors.
        AZ::TransformBus::EventResult(entityId, entityId, &AZ::TransformBus::Events::GetParentId);

        // Go up the hierarchy until you hit the root
        while (entityId.IsValid())
        {
            if (!IsContainerOpen(entityId))
            {
                // If one of the ancestors is a container and it's closed, keep track of its id.
                // We only return of the higher closed container in the hierarchy.
                highestSelectableEntityId = entityId;
            }

            AZ::TransformBus::EventResult(entityId, entityId, &AZ::TransformBus::Events::GetParentId);
        }

        return highestSelectableEntityId;
    }

    void ContainerEntitySystemComponent::OnEntityStreamLoadSuccess()
    {
        // We don't yet support multiple entity contexts, so just use the default.
        auto editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

        Clear(editorEntityContextId);
    }

    ContainerEntityOperationResult ContainerEntitySystemComponent::Clear(AzFramework::EntityContextId entityContextId)
    {
        // We don't yet support multiple entity contexts, so only clear the default.
        auto editorEntityContextId = AzFramework::EntityContextId::CreateNull();
        EditorEntityContextRequestBus::BroadcastResult(editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

        if (entityContextId != editorEntityContextId)
        {
            return AZ::Failure(AZStd::string(
                "Error in ContainerEntitySystemComponent::Clear - cannot clear non-default Entity Context!"));
        }

        if (!m_containers.empty())
        {
            return AZ::Failure(AZStd::string(
                "Error in ContainerEntitySystemComponent::Clear - cannot clear container states if entities are still registered!"));
        }

        m_openContainers.clear();

        return AZ::Success();
    }

    bool ContainerEntitySystemComponent::IsUnderClosedContainerEntity(AZ::EntityId entityId) const
    {
        if (!entityId.IsValid())
        {
            return false;
        }

        // Skip the queried entity, as we only want to check its ancestors.
        AZ::TransformBus::EventResult(entityId, entityId, &AZ::TransformBus::Events::GetParentId);

        // Go up the hierarchy until you hit the root.
        while (entityId.IsValid())
        {
            if (!IsContainerOpen(entityId))
            {
                // One of the ancestors is a container and it's closed.
                return true;
            }

            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(parentId, entityId, &AZ::TransformBus::Events::GetParentId);

            if (parentId == entityId)
            {
                // In some circumstances, querying a root level entity with GetParentId will return
                // the entity itself instead of an invalid entityId. 
                break;
            }

            entityId = parentId;
        }

        // All ancestors are either regular entities or open containers.
        return false;
    }

} // namespace AzToolsFramework
