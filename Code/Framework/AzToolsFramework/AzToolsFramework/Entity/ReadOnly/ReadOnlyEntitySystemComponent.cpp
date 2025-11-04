/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntitySystemComponent.h>

#include <AzToolsFramework/Entity/ReadOnly/ReadOnlyEntityBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

namespace AzToolsFramework
{
    void ReadOnlyEntitySystemComponent::Activate()
    {
        AZ::Interface<ReadOnlyEntityQueryInterface>::Register(this);
        AZ::Interface<ReadOnlyEntityPublicInterface>::Register(this);
        EditorEntityContextNotificationBus::Handler::BusConnect();
    }

    void ReadOnlyEntitySystemComponent::Deactivate()
    {
        EditorEntityContextNotificationBus::Handler::BusDisconnect();
        AZ::Interface<ReadOnlyEntityPublicInterface>::Unregister(this);
        AZ::Interface<ReadOnlyEntityQueryInterface>::Unregister(this);
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

    bool ReadOnlyEntitySystemComponent::IsReadOnly(const AZ::EntityId& entityId)
    {
        if (!entityId.IsValid())
        {
            return false;
        }

        if (!m_readOnlystates.contains(entityId))
        {
            QueryReadOnlyStateForEntity(entityId);
        }

        return m_readOnlystates[entityId];
    }

    void ReadOnlyEntitySystemComponent::RefreshReadOnlyState(const EntityIdList& entityIds)
    {
        for (const AZ::EntityId& entityId : entityIds)
        {
            bool wasReadOnly = m_readOnlystates[entityId];
            QueryReadOnlyStateForEntity(entityId);

            if (bool isReadOnly = m_readOnlystates[entityId]; wasReadOnly != isReadOnly)
            {
                ReadOnlyEntityPublicNotificationBus::Broadcast(
                    &ReadOnlyEntityPublicNotificationBus::Events::OnReadOnlyEntityStatusChanged, entityId, isReadOnly);
            }
        }
    }

    void ReadOnlyEntitySystemComponent::RefreshReadOnlyStateForAllEntities()
    {
        for (auto& elem : m_readOnlystates)
        {
            AZ::EntityId entityId = elem.first;
            bool wasReadOnly = elem.second;
            QueryReadOnlyStateForEntity(entityId);

            if (bool isReadOnly = m_readOnlystates[entityId]; wasReadOnly != isReadOnly)
            {
                ReadOnlyEntityPublicNotificationBus::Broadcast(
                    &ReadOnlyEntityPublicNotificationBus::Events::OnReadOnlyEntityStatusChanged, entityId, isReadOnly);
            }
        }
    }

    void ReadOnlyEntitySystemComponent::OnPrepareForContextReset()
    {
        m_readOnlystates.clear();
    }

    void ReadOnlyEntitySystemComponent::QueryReadOnlyStateForEntity(const AZ::EntityId& entityId)
    {
        bool isReadOnly = false;

        ReadOnlyEntityQueryRequestBus::Broadcast(
            &ReadOnlyEntityQueryRequestBus::Events::IsReadOnly, entityId, isReadOnly);

        m_readOnlystates[entityId] = isReadOnly;
    }

} // namespace AzToolsFramework
