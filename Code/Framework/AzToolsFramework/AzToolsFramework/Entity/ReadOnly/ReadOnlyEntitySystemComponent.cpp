/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntitySystemComponent.h>

#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityNotificationBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    void ReadOnlyEntitySystemComponent::Activate()
    {
        AZ::Interface<ReadOnlyEntityInterface>::Register(this);
        EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void ReadOnlyEntitySystemComponent::Deactivate()
    {
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AZ::Interface<ReadOnlyEntityInterface>::Unregister(this);
    }

    void ReadOnlyEntitySystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ReadOnlyEntitySystemComponent, AZ::Component>()->Version(1);
        }
    }

    void ReadOnlyEntitySystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ReadOnlyEntityService"));
    }

    void ReadOnlyEntitySystemComponent::RegisterEntityAsReadOnly(AZ::EntityId entityId)
    {
        m_readOnlyEntities.insert(entityId);
        ReadOnlyEntityNotificationBus::Broadcast(&ReadOnlyEntityNotificationBus::Events::OnReadOnlyEntityStatusChanged, entityId, true);
    }

    void ReadOnlyEntitySystemComponent::RegisterEntitiesAsReadOnly(EntityIdList entityIds)
    {
        m_readOnlyEntities.insert(entityIds.begin(), entityIds.end());

        for (AZ::EntityId entityId : entityIds)
        {
            ReadOnlyEntityNotificationBus::Broadcast(
                &ReadOnlyEntityNotificationBus::Events::OnReadOnlyEntityStatusChanged, entityId, true);
        }
    }

    void ReadOnlyEntitySystemComponent::UnregisterEntityAsReadOnly(AZ::EntityId entityId)
    {
        m_readOnlyEntities.erase(entityId);
        ReadOnlyEntityNotificationBus::Broadcast(&ReadOnlyEntityNotificationBus::Events::OnReadOnlyEntityStatusChanged, entityId, false);
    }

    void ReadOnlyEntitySystemComponent::UnregisterEntitiesAsReadOnly(EntityIdList entityIds)
    {
        for (AZ::EntityId entityId : entityIds)
        {
            m_readOnlyEntities.erase(entityId);
            ReadOnlyEntityNotificationBus::Broadcast(&ReadOnlyEntityNotificationBus::Events::OnReadOnlyEntityStatusChanged, entityId, false);
        }
    }

    bool ReadOnlyEntitySystemComponent::IsReadOnly(AZ::EntityId entityId) const
    {
        return m_readOnlyEntities.contains(entityId);
    }

    void ReadOnlyEntitySystemComponent::OnContextReset()
    {
        return m_readOnlyEntities.clear();
    }

} // namespace AzToolsFramework
