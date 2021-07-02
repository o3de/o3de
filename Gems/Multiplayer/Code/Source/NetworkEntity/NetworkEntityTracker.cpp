/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkEntity/NetworkEntityTracker.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    void NetworkEntityTracker::Add(NetEntityId netEntityId, AZ::Entity* entity)
    {
        ++m_addChangeDirty;
        AZ_Assert(m_entityMap.end() == m_entityMap.find(netEntityId), "Attempting to add the same entity to the entity map multiple times");
        m_entityMap[netEntityId] = entity;
    }

    NetworkEntityHandle NetworkEntityTracker::Get(NetEntityId netEntityId)
    {
        AZ::Entity* entity = GetRaw(netEntityId);
        return NetworkEntityHandle(entity, netEntityId, this);
    }

    ConstNetworkEntityHandle NetworkEntityTracker::Get(NetEntityId netEntityId) const
    {
        AZ::Entity* entity = GetRaw(netEntityId);
        return ConstNetworkEntityHandle(entity, netEntityId, this);
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
        m_entityMap.erase(netEntityId);
    }

    NetworkEntityTracker::EntityMap::iterator NetworkEntityTracker::erase(EntityMap::iterator iter)
    {
        ++m_deleteChangeDirty;
        return m_entityMap.erase(iter);
    }

    AZ::Entity *NetworkEntityTracker::Move(EntityMap::iterator iter)
    {
        AZ::Entity *ptr = iter->second;
        erase(iter);
        return ptr;
    }
}
