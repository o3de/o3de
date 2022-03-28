/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkEntity/NetworkEntityAuthorityTracker.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/NetworkEntity/INetworkEntityManager.h>
#include <Multiplayer/EntityDomains/IEntityDomain.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/EBus/IEventScheduler.h>
#include <AzNetworking/Utilities/NetworkCommon.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>

namespace Multiplayer
{
    AZ_CVAR(AZ::TimeMs, net_DefaultEntityMigrationTimeoutMs, AZ::TimeMs{ 1000 }, nullptr, AZ::ConsoleFunctorFlags::Null, "Time to wait for a new authority to attach to an entity before we delete the entity");

    NetworkEntityAuthorityTracker::NetworkEntityAuthorityTracker(INetworkEntityManager& networkEntityManager)
        : m_networkEntityManager(networkEntityManager)
        , m_timeoutTimeMs(net_DefaultEntityMigrationTimeoutMs)
    {
        ;
    }

<<<<<<< HEAD
=======
    void NetworkEntityAuthorityTracker::SetTimeoutTimeMs(AZ::TimeMs timeoutTimeMs)
    {
        m_timeoutTimeMs = timeoutTimeMs;
    }

>>>>>>> development
    bool NetworkEntityAuthorityTracker::AddEntityAuthorityManager(ConstNetworkEntityHandle entityHandle, const HostId& newOwner)
    {
        bool ret = false;
        auto timeoutData = m_timedOutNetEntityIds.find(entityHandle.GetNetEntityId());
        if (timeoutData != m_timedOutNetEntityIds.end())
        {
            AZLOG
            (
                NET_AuthTracker,
<<<<<<< HEAD
                "AuthTracker: Removing timeout for networkEntityId %u from %s, new owner is %s",
                aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()),
                timeoutData->second.m_previousOwner.GetString().c_str(),
=======
                "AuthTracker: Removing timeout for networkEntityId %llu, new owner is %s",
                aznumeric_cast<AZ::u64>(entityHandle.GetNetEntityId()),
>>>>>>> development
                newOwner.GetString().c_str()
            );
            m_timedOutNetEntityIds.erase(timeoutData);
            ret = true;
        }

<<<<<<< HEAD
        auto iter = m_entityAuthorityMap.find(entityHandle.GetNetEntityId());
        if (iter != m_entityAuthorityMap.end())
        {
            AZLOG
            (
                NET_AuthTracker,
                "AuthTracker: Assigning networkEntityId %u from %s to %s",
                aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()),
                iter->second.back().GetString().c_str(),
                newOwner.GetString().c_str()
            );
        }
        else
        {
            AZLOG
            (
                NET_AuthTracker,
                "AuthTracker: Assigning networkEntityId %u to %s",
                aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()),
                newOwner.GetString().c_str()
            );
        }
=======
        AZLOG
        (
            NET_AuthTracker,
            "AuthTracker: Assigning networkEntityId %llu to %s",
            aznumeric_cast<AZ::u64>(entityHandle.GetNetEntityId()),
            newOwner.GetString().c_str()
        );
>>>>>>> development

        m_entityAuthorityMap[entityHandle.GetNetEntityId()].push_back(newOwner);
        return ret;
    }

    void NetworkEntityAuthorityTracker::RemoveEntityAuthorityManager(ConstNetworkEntityHandle entityHandle, const HostId& previousOwner)
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

<<<<<<< HEAD
            AZLOG(NET_AuthTracker, "AuthTracker: Removing networkEntityId %u from %s", aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()), previousOwner.GetString().c_str());
=======
            AZLOG(NET_AuthTracker, "AuthTracker: Removing networkEntityId %llu from %s", aznumeric_cast<AZ::u64>(entityHandle.GetNetEntityId()), previousOwner.GetString().c_str());
>>>>>>> development
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
                            m_timedOutNetEntityIds.find(entityHandle.GetNetEntityId()) == m_timedOutNetEntityIds.end(),
                            "Trying to add something twice to the timeout map, this is unexpected"
                        );
                        m_timedOutNetEntityIds.insert(entityHandle.GetNetEntityId());
                        AZ::Interface<AZ::IEventScheduler>::Get()->AddCallback([this, netEntityId = entityHandle.GetNetEntityId()]
                            {
                                auto timeoutData = m_timedOutNetEntityIds.find(netEntityId);
                                if (timeoutData != m_timedOutNetEntityIds.end())
                                {
                                    m_timedOutNetEntityIds.erase(timeoutData);
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
                                            m_networkEntityManager.GetEntityDomain()->HandleLossOfAuthoritativeReplicator(entityHandle);
                                        }
                                    }
                                }
                            },
                            AZ::Name("Entity authority removal functor"),
                            m_timeoutTimeMs
                        );
                    }
                    else
                    {
                        AZLOG(NET_AuthTracker, "AuthTracker: Skipping timeout for Autonomous networkEntityId %llu", aznumeric_cast<AZ::u64>(entityHandle.GetNetEntityId()));
                    }
                }
            }
        }
        else
        {
            AZLOG(NET_AuthTracker, "AuthTracker: Remove authority called on networkEntityId that was never added %llu", aznumeric_cast<AZ::u64>(entityHandle.GetNetEntityId()));
            AZ_Assert(false, "AuthTracker: Remove authority called on entity that was never added");
        }
    }

    HostId NetworkEntityAuthorityTracker::GetEntityAuthorityManager(ConstNetworkEntityHandle entityHandle) const
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

<<<<<<< HEAD
    NetworkEntityAuthorityTracker::TimeoutData::TimeoutData(ConstNetworkEntityHandle entityHandle, const HostId& previousOwner)
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
                        "Timed out entity id %u during migration previous owner %s, removing it",
                        aznumeric_cast<uint32_t>(entityHandle.GetNetEntityId()),
                        timeoutData->second.m_previousOwner.GetString().c_str()
                    );
                    m_networkEntityManager.MarkForRemoval(entityHandle);
                }
            }
        }
        return AzNetworking::TimeoutResult::Delete;
=======
    bool NetworkEntityAuthorityTracker::DoesEntityHaveOwner(ConstNetworkEntityHandle entityHandle) const
    {
        return InvalidHostId != GetEntityAuthorityManager(entityHandle);
>>>>>>> development
    }
}
