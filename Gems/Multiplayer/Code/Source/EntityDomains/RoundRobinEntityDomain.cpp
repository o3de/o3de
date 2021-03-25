/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Source/EntityDomains/RoundRobinEntityDomain.h>

namespace Multiplayer
{
    RoundRobinEntityDomain::RoundRobinEntityDomain(const RoundRobinEntityDomain& rhs)
        : m_hostId(rhs.m_hostId)
        , m_multiserverCount(rhs.m_multiserverCount)
        , m_entitiesNotInDomain(rhs.m_entitiesNotInDomain)
        , m_controllersActivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersActivated(entityHandle, entityIsMigrating); })
        , m_controllersDeactivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersDeactivated(entityHandle, entityIsMigrating); })
    {
        ;
    }

    RoundRobinEntityDomain::RoundRobinEntityDomain(HostId hostId, uint32_t multiserverCount)
        : m_hostId(hostId)
        , m_multiserverCount(multiserverCount)
        , m_controllersActivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersActivated(entityHandle, entityIsMigrating); })
        , m_controllersDeactivatedHandler([this](const ConstNetworkEntityHandle& entityHandle, EntityIsMigrating entityIsMigrating) { OnControllersDeactivated(entityHandle, entityIsMigrating); })
    {
        ;
    }

    bool RoundRobinEntityDomain::IsInDomain([[maybe_unused]] const ConstNetworkEntityHandle& entityHandle) const
    {
        //const int desiredDomain = (entityHandle.GetNetEntityId() % m_ServerShardCount) + k_FirstGameShardId;
        //const bool netIdInDomain = desiredDomain == m_ServerShardId;
        //const bool isPlayer = entityHandle->FindComponent<PlayerComponent>() != nullptr;
        //const bool isGlobalEnt = entityHandle->FindComponent<GlobalAccessComponent>() != nullptr;
        //auto entityHierarchyComponent = entityHandle->FindComponent<EntityHierarchyComponent>();
        //const bool isParented = entityHierarchyComponent ? entityHierarchyComponent->IsParented() : false;
        //return netIdInDomain || isPlayer || isGlobalEnt || isParented;
        return false;
    }

    void RoundRobinEntityDomain::ActivateTracking(const INetworkEntityManager::OwnedEntitySet& ownedEntitySet)
    {
        for (auto& entityHandle : ownedEntitySet)
        {
            OnControllersActivated(entityHandle, EntityIsMigrating::False);
        }
        GetNetworkEntityManager()->AddControllersActivatedHandler(m_controllersActivatedHandler);
        GetNetworkEntityManager()->AddControllersDeactivatedHandler(m_controllersDeactivatedHandler);
    }

    void RoundRobinEntityDomain::DebugDraw() const
    {
        // Nothing to draw
    }

    void RoundRobinEntityDomain::RetrieveEntitiesNotInDomain(EntitiesNotInDomain& outEntitiesNotInDomain) const
    {
        outEntitiesNotInDomain.insert(m_entitiesNotInDomain.begin(), m_entitiesNotInDomain.end());
    }

    void RoundRobinEntityDomain::OnControllersActivated(const ConstNetworkEntityHandle& entityHandle, [[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        if (!IsInDomain(entityHandle))
        {
            m_entitiesNotInDomain.insert(entityHandle.GetNetEntityId());
        }
    }

    void RoundRobinEntityDomain::OnControllersDeactivated(const ConstNetworkEntityHandle& entityHandle, [[maybe_unused]] EntityIsMigrating entityIsMigrating)
    {
        m_entitiesNotInDomain.erase(entityHandle.GetNetEntityId());
    }
}
