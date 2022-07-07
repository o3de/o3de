/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkEntity/NetworkEntityTracker.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    void NetworkEntityTracker::Add(NetEntityId netEntityId, AZ::Entity* entity)
    {
        ++m_addChangeDirty;
        AZ_Assert(m_entityMap.end() == m_entityMap.find(netEntityId), "Attempting to add the same entity to the entity map multiple times");
        m_entityMap[netEntityId] = entity;
        m_netEntityIdMap[entity->GetId()] = netEntityId;
    }

    void NetworkEntityTracker::RegisterNetBindComponent(AZ::Entity* entity, NetBindComponent* component)
    {
        m_netBindingMap[entity] = component;
    }

    void NetworkEntityTracker::UnregisterNetBindComponent(NetBindComponent* component)
    {
        m_netBindingMap.erase(component->GetEntity());
    }

    NetworkEntityHandle NetworkEntityTracker::Get(NetEntityId netEntityId)
    {
        AZ::Entity* entity = GetRaw(netEntityId);
        return NetworkEntityHandle(entity, this);
    }

    ConstNetworkEntityHandle NetworkEntityTracker::Get(NetEntityId netEntityId) const
    {
        AZ::Entity* entity = GetRaw(netEntityId);
        return ConstNetworkEntityHandle(entity, this);
    }

    NetEntityId NetworkEntityTracker::Get(const AZ::EntityId& entityId) const
    {
        auto found = m_netEntityIdMap.find(entityId);
        if (found != m_netEntityIdMap.end())
        {
            return found->second;
        }
        return Multiplayer::InvalidNetEntityId;
    }

    bool NetworkEntityTracker::Exists(NetEntityId netEntityId) const
    {
        return (m_entityMap.find(netEntityId) != m_entityMap.end());
    }

    AZ::Entity* NetworkEntityTracker::GetRaw(NetEntityId netEntityId) const
    {
        auto found = m_entityMap.find(netEntityId);
        if (found != m_entityMap.end())
        {
            return found->second;
        }
        return nullptr;
    }

    void NetworkEntityTracker::erase(NetEntityId netEntityId)
    {
        ++m_deleteChangeDirty;

        auto found = m_entityMap.find(netEntityId);
        if (found != m_entityMap.end())
        {
            m_netEntityIdMap.erase(found->second->GetId());            
            m_entityMap.erase(found);
        }
    }

    NetworkEntityTracker::EntityMap::iterator NetworkEntityTracker::erase(EntityMap::iterator iter)
    {
        ++m_deleteChangeDirty;
        if (iter != m_entityMap.end())
        {
            m_netEntityIdMap.erase(iter->second->GetId());
        }
        return m_entityMap.erase(iter);
    }

    AZ::Entity *NetworkEntityTracker::Move(EntityMap::iterator iter)
    {
        AZ::Entity *ptr = iter->second;
        erase(iter);
        return ptr;
    }
}
