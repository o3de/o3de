/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ContainerEntity/ContainerEntitySystemComponent.h>
#include <AzToolsFramework/ContainerEntity/ContainerEntityNotificationBus.h>

namespace AzToolsFramework
{
    void ContainerEntitySystemComponent::Init()
    {
    }

    void ContainerEntitySystemComponent::Activate()
    {
        AZ::Interface<ContainerEntityInterface>::Register(this);
    }

    void ContainerEntitySystemComponent::Deactivate()
    {
        AZ::Interface<ContainerEntityInterface>::Unregister(this);
    }

    void ContainerEntitySystemComponent::Reflect([[maybe_unused]] AZ::ReflectContext* context)
    {
    }

    void ContainerEntitySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ContainerEntityService"));
    }

    void ContainerEntitySystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void ContainerEntitySystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }

    ContainerEntityOperationResult ContainerEntitySystemComponent::RegisterEntityAsContainer(AZ::EntityId entityId)
    {
        if (IsContainer(entityId))
        {
            return AZ::Failure(AZStd::string(
                "ContainerEntitySystemComponent error - trying to register entity as container twice."));
        }

        m_containerSet.insert(entityId);
        // Containers start closed by default
        m_closedContainerSet.insert(entityId);

        return AZ::Success();
    }

    ContainerEntityOperationResult ContainerEntitySystemComponent::UnregisterEntityAsContainer(AZ::EntityId entityId)
    {
        if (!IsContainer(entityId))
        {
            return AZ::Failure(AZStd::string(
                "ContainerEntitySystemComponent error - trying to unregister entity that is not a container."));
        }

        m_containerSet.erase(entityId);
        // Unregistering a container also clears its state
        m_closedContainerSet.erase(entityId);

        return AZ::Success();
    }

    bool ContainerEntitySystemComponent::IsContainer(AZ::EntityId entityId) const
    {
        return m_containerSet.contains(entityId);
    }

    ContainerEntityOperationResult ContainerEntitySystemComponent::SetContainerClosedState(AZ::EntityId entityId, bool closed)
    {
        if (!IsContainer(entityId))
        {
            return AZ::Failure(AZStd::string(
                "ContainerEntitySystemComponent error - cannot set closed state of entity that was not registered as container."));
        }

        if(closed)
        {
            m_closedContainerSet.insert(entityId);
        }
        else
        {
            m_closedContainerSet.erase(entityId);
        }

        ContainerEntityNotificationBus::Broadcast(&ContainerEntityNotifications::OnContainerEntityStatusChanged, entityId);

        return AZ::Success();
    }

    bool ContainerEntitySystemComponent::IsContainerClosed(AZ::EntityId entityId) const
    {
        return m_closedContainerSet.contains(entityId);
    }

} // namespace AzToolsFramework
