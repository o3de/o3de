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

        return AZ::Success();
    }

    bool ContainerEntitySystemComponent::IsContainer(AZ::EntityId entityId) const
    {
        return m_containerSet.contains(entityId);
    }

    ContainerEntityOperationResult ContainerEntitySystemComponent::SetContainerOpenState(AZ::EntityId entityId, bool open)
    {
        if (!IsContainer(entityId))
        {
            return AZ::Failure(AZStd::string(
                "ContainerEntitySystemComponent error - cannot set open state of entity that was not registered as container."));
        }

        if(open)
        {
            m_openContainerSet.insert(entityId);
        }
        else
        {
            m_openContainerSet.erase(entityId);
        }

        ContainerEntityNotificationBus::Broadcast(&ContainerEntityNotifications::OnContainerEntityStatusChanged, entityId);

        return AZ::Success();
    }

    bool ContainerEntitySystemComponent::IsContainerOpen(AZ::EntityId entityId) const
    {
        // If the entity is not a container, it should behave as open.
        if(!m_containerSet.contains(entityId))
        {
            return true;
        }

        // If the entity is a container, return its state.
        return m_openContainerSet.contains(entityId);
    }

} // namespace AzToolsFramework
