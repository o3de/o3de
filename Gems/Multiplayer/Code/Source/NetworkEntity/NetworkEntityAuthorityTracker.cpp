/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkEntity/NetworkEntityAuthorityTracker.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/NetworkEntity/INetworkEntityManager.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>

namespace Multiplayer
{
    AZ_CVAR(AZ::TimeMs, net_EntityMigrationTimeoutMs, AZ::TimeMs{ 1000 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Time to wait for a new authority to attach to an entity before we delete the entity");

    NetworkEntityAuthorityTracker::NetworkEntityAuthorityTracker(INetworkEntityManager& networkEntityManager)
        : m_networkEntityManager(networkEntityManager)
    {
        ;
    }

    bool NetworkEntityAuthorityTracker::AddEntityAuthorityManager(ConstNetworkEntityHandle entityHandle, HostId newOwner)
    {
        bool ret = false;
        auto timeoutData = m_timeoutDataMap.find(entityHandle.GetNetEntityId());
        if (timeoutData != m_timeoutDataMap.end())
        {
            AZLOG
            (
                NET_AuthTracker,
                "AuthTracker: Removing timeout for networkEntityId %u from %u, new owner is %u",
                aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()),
                aznumeric_cast<uint32_t>(timeoutData->second.m_previousOwner),
                aznumeric_cast<uint32_t>(newOwner)
            );
            m_timeoutDataMap.erase(timeoutData);
            ret = true;
        }

        auto iter = m_entityAuthorityMap.find(entityHandle.GetNetEntityId());
        if (iter != m_entityAuthorityMap.end())
        {
            AZLOG
            (
                NET_AuthTracker,
                "AuthTracker: Assigning networkEntityId %u from %u to %u",
                aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()),
                aznumeric_cast<uint32_t>(iter->second.back()),
                aznumeric_cast<uint32_t>(newOwner)
            );
        }
        else
        {
            AZLOG
            (
                NET_AuthTracker,
                "AuthTracker: Assigning networkEntityId %u to %u",
                aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()),
                aznumeric_cast<uint32_t>(newOwner)
            );
        }

        m_entityAuthorityMap[entityHandle.GetNetEntityId()].push_back(newOwner);
        return ret;
    }

    void NetworkEntityAuthorityTracker::RemoveEntityAuthorityManager(ConstNetworkEntityHandle entityHandle, HostId previousOwner)
    {
        auto mapIter = m_entityAuthorityMap.find(entityHandle.GetNetEntityId());
        if (mapIter != m_entityAuthorityMap.end())
        {
            auto& authorityStack = mapIter->second;
            for (auto stackIter = authorityStack.begin(); stackIter != authorityStack.end();)
            {
                if (*stackIter == previousOwner)
                {
                    stackIter = authorityStack.erase(stackIter);
                }
                else
                {
                    ++stackIter;
                }
            }

            AZLOG(NET_AuthTracker, "AuthTracker: Removing networkEntityId %u from %u", aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()), aznumeric_cast<uint32_t>(previousOwner));
            if (auto localEnt = entityHandle.GetEntity())
            {
                if (authorityStack.empty())
                {
                    m_entityAuthorityMap.erase(entityHandle.GetNetEntityId());
                    NetEntityRole networkRole = NetEntityRole::InvalidRole;
                    NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
                    if (netBindComponent != nullptr)
                    {
                        networkRole = netBindComponent->GetNetEntityRole();
                    }
                    if (networkRole != NetEntityRole::Autonomous)
                    {
                        AZ_Assert
                        (
                            (m_timeoutDataMap.find(entityHandle.GetNetEntityId()) == m_timeoutDataMap.end()) ||
                            (m_timeoutDataMap[entityHandle.GetNetEntityId()].m_previousOwner == previousOwner),
                            "Trying to add something twice to the timeout map, this is unexpected"
                        );
                        m_timeoutQueue.RegisterItem(aznumeric_cast<uint64_t>(entityHandle.GetNetEntityId()), net_EntityMigrationTimeoutMs);
                        TimeoutData& timeoutData = m_timeoutDataMap[entityHandle.GetNetEntityId()];
                        timeoutData.m_entityHandle = entityHandle;
                        timeoutData.m_previousOwner = previousOwner;
                    }
                    else
                    {
                        AZLOG(NET_AuthTracker, "AuthTracker: Skipping timeout for Autonomous networkEntityId %u", aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()));
                    }
                }
            }
        }
        else
        {
            AZLOG(NET_AuthTracker, "AuthTracker: Remove authority called on networkEntityId that was never added %u", aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()));
            AZ_Assert(false, "AuthTracker: Remove authority called on entity that was never added");
        }
    }

    HostId NetworkEntityAuthorityTracker::GetEntityAuthorityManager(ConstNetworkEntityHandle entityHandle) const
    {
        HostId hostId = GetEntityAuthorityManagerInternal(entityHandle);
        AZ_Assert(hostId != InvalidHostId, "Unable to determine manager for entity");
        return hostId;
    }

    bool NetworkEntityAuthorityTracker::DoesEntityHaveOwner(ConstNetworkEntityHandle entityHandle) const
    {
        return InvalidHostId != GetEntityAuthorityManagerInternal(entityHandle);
    }

    HostId NetworkEntityAuthorityTracker::GetEntityAuthorityManagerInternal(ConstNetworkEntityHandle entityHandle) const
    {
        if (auto localEnt = entityHandle.GetEntity())
        {
            NetEntityRole networkRole = NetEntityRole::InvalidRole;
            NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
            if (netBindComponent != nullptr)
            {
                networkRole = netBindComponent->GetNetEntityRole();
            }
            if (networkRole == NetEntityRole::Authority)
            {
                return m_networkEntityManager.GetHostId();
            }
            else
            {
                auto iter = m_entityAuthorityMap.find(entityHandle.GetNetEntityId());
                if (iter != m_entityAuthorityMap.end())
                {
                    if (!iter->second.empty())
                    {
                        return iter->second.back();
                    }
                }
            }
        }
        return InvalidHostId;
    }

    NetworkEntityAuthorityTracker::TimeoutData::TimeoutData(ConstNetworkEntityHandle entityHandle, HostId previousOwner)
        : m_entityHandle(entityHandle)
        , m_previousOwner(previousOwner)
    {
        ;
    }

    NetworkEntityAuthorityTracker::NetworkEntityTimeoutFunctor::NetworkEntityTimeoutFunctor
    (
        NetworkEntityAuthorityTracker& networkEntityAuthorityTracker,
        INetworkEntityManager& networkEntityManager
    )
        : m_networkEntityAuthorityTracker(networkEntityAuthorityTracker)
        , m_networkEntityManager(networkEntityManager)
    {
        ;
    }

    AzNetworking::TimeoutResult NetworkEntityAuthorityTracker::NetworkEntityTimeoutFunctor::HandleTimeout(AzNetworking::TimeoutQueue::TimeoutItem& item)
    {
        const NetEntityId netEntityId = aznumeric_cast<NetEntityId>(item.m_userData);
        auto timeoutData = m_networkEntityAuthorityTracker.m_timeoutDataMap.find(netEntityId);
        if (timeoutData != m_networkEntityAuthorityTracker.m_timeoutDataMap.end())
        {
            m_networkEntityAuthorityTracker.m_timeoutDataMap.erase(timeoutData);
            ConstNetworkEntityHandle entityHandle = m_networkEntityManager.GetEntity(netEntityId);
            if (auto entity = entityHandle.GetEntity())
            {
                NetEntityRole networkRole = NetEntityRole::InvalidRole;
                NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
                if (netBindComponent != nullptr)
                {
                    networkRole = netBindComponent->GetNetEntityRole();
                }
                if (networkRole != NetEntityRole::Authority)
                {
                    AZLOG_ERROR
                    (
                        "Timed out entity id %u during migration previous owner %u, removing it",
                        aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()),
                        aznumeric_cast<uint32_t>(timeoutData->second.m_previousOwner)
                    );
                    m_networkEntityManager.MarkForRemoval(entityHandle);
                }
            }
        }
        return AzNetworking::TimeoutResult::Delete;
    }
}
