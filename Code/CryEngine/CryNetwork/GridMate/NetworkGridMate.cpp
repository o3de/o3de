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
#include "CryNetwork_precompiled.h"

#include "ILevelSystem.h"

#include "NetworkGridMate.h"
#include "NetworkGridmateDebug.h"

#include "Replicas/EntityReplica.h"
#include "Replicas/EntityScriptReplicaChunk.h"
#include "Compatibility/GridMateNetSerialize.h"
#include "Compatibility/GridMateRMI.h"

#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/BasicHostChunkDescriptor.h>

#include "NetworkGridMateEntityEventBus.h"


#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define NETWORKGRIDMATE_CPP_SECTION_1 1
#define NETWORKGRIDMATE_CPP_SECTION_2 2
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION NETWORKGRIDMATE_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(GridMate/NetworkGridMate_cpp)
#endif

namespace GridMate
{
    //-----------------------------------------------------------------------------
    Network* Network::s_instance = nullptr;

    int Network::s_StatsIntervalMS          = 1000;     // 1 second by default.
    int Network::s_DumpStatsEnabled         = 0;

    FILE* Network::s_DumpStatsFile          = nullptr;

    //-----------------------------------------------------------------------------
    Network::Network()
        : m_localChannelId(kOfflineChannelId)
        , m_gridMate(nullptr)
        , m_session(nullptr)
        , m_levelLoadState(LevelLoadState_None)
        , m_allowMinimalUpdate(false)
    {
        s_instance = this;
        m_legacySerializeProvider = this;

        m_postFrameTasks.reserve(32);
    }

    //-----------------------------------------------------------------------------
    Network::~Network()
    {
#if GRIDMATE_DEBUG_ENABLED
        Debug::UnregisterCVars();
#endif

        if (GetLevelSystem())
        {
            GetLevelSystem()->RemoveListener(this);
        }

        m_activeEntityReplicaMap.clear();
        m_newProxyEntities.clear();

        ShutdownGridMate();

        if (s_DumpStatsFile)
        {
            fclose(s_DumpStatsFile);
            s_DumpStatsFile = nullptr;
        }

        s_instance = nullptr;
    }

    //-----------------------------------------------------------------------------
    bool Network::Init([[maybe_unused]] int ncpu)
    {
#if GRIDMATE_DEBUG_ENABLED
        Debug::RegisterCVars();
#endif

        StartGridMate();
        MarkAsLocalOnly();

        return true;
    }

    //-----------------------------------------------------------------------------
    Network& Network::Get()
    {
        GM_ASSERT_TRACE(s_instance, "Network interface has not yet been created.");
        return *s_instance;
    }

    //-----------------------------------------------------------------------------
    void Network::Release()
    {
        delete this;
    }

    //-----------------------------------------------------------------------------
    bool Network::AllowEntityCreation() const
    {
        return true;
    }

    //-----------------------------------------------------------------------------
    bool Network::IsInMinimalUpdate() const
    {
        return m_allowMinimalUpdate;
    }

    //-----------------------------------------------------------------------------
    void Network::SyncWithGame(ENetworkGameSync syncType)
    {
        FUNCTION_PROFILER(GetISystem(), PROFILE_NETWORK);

        switch (syncType)
        {
        case eNGS_FrameStart:
        {
            UpdateGridMate(syncType);
        }
        break;

        case eNGS_FrameEnd:
        {
            FlushPostFrameTasks();

            UpdateGridMate(syncType);

            UpdateNetworkStatistics();

            DebugDraw();
        }
        break;

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Inherited from CryNetwork, this mechanism is required for safe updating during loading.
        // During such time, the network is pumped via the NetworkStallerTicker thread, and this flag
        // basically describes when it's safe for network messages to be distributed to the game.
        case eNGS_AllowMinimalUpdate:
        {
            m_allowMinimalUpdate = true;
            m_levelLoadState = LevelLoadState_Loading;
        }
        break;

        case eNGS_DenyMinimalUpdate:
        {
            m_allowMinimalUpdate = false;
            m_levelLoadState = LevelLoadState_Loaded;
        }
        break;

        case eNGS_MinimalUpdateForLoading:
        {
            if (m_allowMinimalUpdate)
            {
                UpdateGridMate(syncType);
            }
        }
        break;
            /////////////////////////////////////////////////////////////////////////////////////////////
        }
    }

    //-----------------------------------------------------------------------------
    void Network::FlushPostFrameTasks()
    {
        BindNewEntitiesToNetwork();

        for (const Task& task : m_postFrameTasks)
        {
            task();
        }

        RMI::FlushQueue();

        m_postFrameTasks.clear();
    }

    //-----------------------------------------------------------------------------
    void Network::UpdateGridMate(ENetworkGameSync syncType)
    {
        if (m_gridMate && m_mutexUpdatingGridMate.try_lock() )
        {
            FRAME_PROFILER("GridMate Update", GetISystem(), PROFILE_NETWORK);

            GridMate::ReplicaManager* replicaManager = GetCurrentSession() ? GetCurrentSession()->GetReplicaMgr() : nullptr;
            if (replicaManager)
            {
                switch (syncType)
                {
                    case eNGS_MinimalUpdateForLoading:
                    case eNGS_FrameStart:
                    {
                        if (replicaManager)
                        {
                            replicaManager->Unmarshal();
                            replicaManager->UpdateFromReplicas();
                        }

                        // When called from the network stall ticker thread, marshalling should be performed as well.
                        if (syncType != eNGS_MinimalUpdateForLoading)
                        {
                            break;
                        }
                    }

                    case eNGS_FrameEnd:
                    {
                        if (replicaManager)
                        {
                            replicaManager->UpdateReplicas();
                            replicaManager->Marshal();
                        }
                        break;
                    }
                    default: break;
                }
            }

            m_gridMate->Update();
            m_mutexUpdatingGridMate.unlock();
        }
    }

    //-----------------------------------------------------------------------------
    ChannelId Network::GetChannelIdForSessionMember(GridMate::GridMember* member) const
    {
        return member ? ChannelId(member->GetIdCompact()) : kInvalidChannelId;
    }

    //-----------------------------------------------------------------------------
    void Network::ChangedAspects(EntityId entityId, NetworkAspectType aspectBits)
    {
        if (aspectBits == 0)
        {
            return; // nothing to do
        }

#ifndef _RELEASE
        for (size_t i = NetSerialize::kNumAspectSlots; i < NUM_ASPECTS; ++i)
        {
            if (BIT64(i) & aspectBits)
            {
                GM_ASSERT_TRACE(0, "Any aspects >= %u can not be serialized through this layer, until support for > 32 data sets is enabled.", static_cast<uint32>(NetSerialize::kNumAspectSlots));
                break;
            }
        }
#endif

        EntityReplica* replica = FindEntityReplica(entityId);
        if (replica)
        {
            if (replica->IsMaster() || replica->IsAspectDelegatedToThisClient())
            {
                NetworkAspectType oldDirtyAspects = replica->GetDirtyAspects();
                replica->MarkAspectsDirty(aspectBits);

                if (replica->IsAspectDelegatedToThisClient())
                {
                    // Only add the task if these are the first aspects being dirtied.
                    if (oldDirtyAspects == 0)
                    {
                        m_postFrameTasks.push_back(
                            [=]()
                            {
                                EntityReplica* rep = FindEntityReplica(entityId);
                                if (rep)
                                {
                                    rep->UploadClientDelegatedAspects();
                                }
                            }
                            );
                    }
                }
            }
        }
        else
        {
            GM_DEBUG_TRACE("Failed to mark aspects dirty because replica for "
                "entity id %u could not be found.", entityId);
        }
    }

    //-----------------------------------------------------------------------------
    ChannelId Network::GetLocalChannelId() const
    {
        return m_localChannelId;
    }

    //-----------------------------------------------------------------------------
    ChannelId Network::GetServerChannelId() const
    {
        if (m_session)
        {
            return GetChannelIdForSessionMember(m_session->GetHost());
        }

        return m_localChannelId;
    }

    //-----------------------------------------------------------------------------
    EntityId Network::LocalEntityIdToServerEntityId(EntityId localId) const
    {
        if (!gEnv->bServer)
        {
            // \todo - Optimize. Keep a local->server id map locally. We already have server->local
            // via m_activeEntityReplicaMap.
            for (auto& replicaEntry : m_activeEntityReplicaMap)
            {
                if (replicaEntry.second->GetLocalEntityId() == localId)
                {
                    return replicaEntry.first;
                }
            }

            return kInvalidEntityId;
        }

        return localId;
    }

    //-----------------------------------------------------------------------------
    EntityId Network::ServerEntityIdToLocalEntityId(EntityId serverId, bool allowForcedEstablishment /*= false*/) const
    {
        EntityId localId = kInvalidEntityId;

        if (gEnv->bServer)
        {
            localId = serverId;
        }
        else
        {
            auto foundAt = m_activeEntityReplicaMap.find(serverId);
            if (foundAt != m_activeEntityReplicaMap.end())
            {
                EntityReplicaPtr replica = foundAt->second;

                localId = replica->GetLocalEntityId();
            }
            else if (allowForcedEstablishment)
            {
                AZ_Assert(AllowEntityCreation(), "Entity creation is not allowed during level loads! Forcing creation is going to cause problems!");

                // If we're deserializing this entity Id via the 'eid' policy, but the local entity is not
                // yet established, expedite establishment. This is to ensure we can properly map/decode
                // the entity Id mid-serialization.
                auto newProxy = m_newProxyEntities.find(serverId);
                if (newProxy != m_newProxyEntities.end())
                {
                    EntityReplicaPtr replica = newProxy->second;
                    localId = replica->HandleNewlyReceivedNow();
                }
            }
        }

        return localId;
    }

    //-----------------------------------------------------------------------------
    void Network::InvokeActorRMI(EntityId entityId, uint8 actorExtensionId, ChannelId targetChannelFilter, IActorRMIRep& rep)
    {
        RMI::InvokeActor(entityId, actorExtensionId, targetChannelFilter, rep);
    }

    //-----------------------------------------------------------------------------
    void Network::InvokeScriptRMI(ISerializable* serializable, bool isServerRMI, ChannelId toChannelId, ChannelId avoidChannelId)
    {
        RMI::InvokeScript(serializable, isServerRMI, toChannelId, avoidChannelId);
    }

    //-----------------------------------------------------------------------------
    void Network::RegisterActorRMI(IActorRMIRep* rep)
    {
        RMI::RegisterActorRMI(rep);
    }

    //-----------------------------------------------------------------------------
    void Network::UnregisterActorRMI(IActorRMIRep* rep)
    {
        RMI::UnregisterActorRMI(rep);
    }

    //-----------------------------------------------------------------------------
    void Network::SetDelegatableAspectMask(NetworkAspectType aspectBits)
    {
        NetSerialize::SetDelegatableAspects(aspectBits);
    }

    //-----------------------------------------------------------------------------
    void Network::SetObjectDelegatedAspectMask(EntityId entityId, NetworkAspectType aspects, bool set)
    {
        m_postFrameTasks.push_back(
            [=]()
            {
                if (EntityReplica* entityReplica = FindEntityReplica(entityId))
                {
                    NetworkAspectType mask = entityReplica->GetClientDelegatedAspectMask();

                    if (set)
                    {
                        mask |= aspects;
                    }
                    else
                    {
                        mask &= ~aspects;
                    }

                    entityReplica->SetClientDelegatedAspectMask(mask);
                }
                else
                {
                    GM_DEBUG_TRACE("Failed to update aspect delegation mask because replica"
                        "for entity id %u could not be found.", entityId);
                }
            }
            );
    }

    //-----------------------------------------------------------------------------
    void Network::DelegateAuthorityToClient(EntityId entityId, ChannelId clientChannelId)
    {
        GridMate::EntityReplica* replica = FindEntityReplica(entityId);
        if (replica)
        {
            replica->RPCDelegateAuthorityToOwner(clientChannelId);
        }
    }

    void Network::ShutdownGridMate()
    {
        if (m_gridMate)
        {
            GM_DEBUG_TRACE("Shutting down GridMate network.");

            m_postFrameTasks.clear();
            RMI::EmptyQueue();

            GridMateDestroy(m_gridMate);
            m_gridMate = nullptr;

            if (m_sessionEvents.IsConnected())
            {
                m_sessionEvents.Disconnect();
            }

            if (m_systemEvents.IsConnected())
            {
                m_systemEvents.Disconnect();
            }
        }
    }

    //-----------------------------------------------------------------------------
    EntityReplica* Network::FindEntityReplica(EntityId id) const
    {
        if (!gEnv->bServer)
        {
            // Replicas are mapped by server-side entity Id, and we map back and forth
            // to reconcile across server and clients.
            // Upon deserializing via 'eid' policy, server-side Ids are converted back
            // to local.
            id = LocalEntityIdToServerEntityId(id);
        }

        auto replicaIter = m_activeEntityReplicaMap.find(id);

        if (replicaIter != m_activeEntityReplicaMap.end())
        {
            return replicaIter->second.get();
        }

        return nullptr;
    }

    //-----------------------------------------------------------------------------
    void Network::StartGridMate()
    {
        if (nullptr != m_gridMate)
        {
            return;
        }

        GridMateDesc desc;
        m_gridMate = GridMateCreate(desc);

        // Monitor session events.
        GM_ASSERT_TRACE(!m_sessionEvents.IsConnected(), "Session events bus should not be connected yet.");
        m_sessionEvents.Connect(m_gridMate);

        // Monitor internal system events.
        GM_ASSERT_TRACE(!m_systemEvents.IsConnected(), "System events bus should not be connected yet.");
        m_systemEvents.Connect();

        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(EntityReplica::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<EntityReplica, EntityReplica::EntityReplicaDesc>();
        }

        if (!ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(EntityScriptReplicaChunk::GetChunkName())))
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<EntityScriptReplicaChunk>();
        }

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION NETWORKGRIDMATE_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(GridMate/NetworkGridMate_cpp)
#endif
    }

    //-----------------------------------------------------------------------------
    void Network::OnLoadingComplete([[maybe_unused]] ILevel* level)
    {
    }

    //-----------------------------------------------------------------------------
    void Network::OnUnloadComplete([[maybe_unused]] ILevel* level)
    {
        m_levelLoadState = LevelLoadState_None;
        m_activeEntityReplicaMap.clear();
        m_newServerEntities.clear();
    }

    //-----------------------------------------------------------------------------
    CTimeValue Network::GetSessionTime()
    {
        CTimeValue t = gEnv->pTimer->GetFrameStartTime();
        if (m_session)
        {
            t.SetMilliSeconds(m_session->GetTime());
        }
        return t;
    }
    //-----------------------------------------------------------------------------
    void Network::UpdateNetworkStatistics()
    {
        static float s_lastUpdate = 0.f;

        const float time = gEnv->pTimer->GetCurrTime(ITimer::ETIMER_UI);
        if (time >= s_lastUpdate + (s_StatsIntervalMS * 0.001f))
        {
            FUNCTION_PROFILER(GetISystem(), PROFILE_NETWORK);

            s_lastUpdate = time;

            if (m_session)
            {
                Carrier* carrier = m_session->GetCarrier();

                for (unsigned int i = 0; i < m_session->GetNumberOfMembers(); ++i)
                {
                    GridMember* member = m_session->GetMemberByIndex(i);

                    if (member == m_session->GetMyMember())
                    {
                        continue;
                    }

                    const ConnectionID connId = member->GetConnectionId();

                    if (connId != InvalidConnectionID)
                    {
                        TrafficControl::Statistics stats;
                        carrier->QueryStatistics(connId, &stats);

                        CarrierStatistics& memberStats =
                            m_statisticsPerChannel[ GetChannelIdForSessionMember(member) ];

                        memberStats.m_rtt = stats.m_rtt;
                        memberStats.m_packetLossRate = stats.m_packetLoss;
                        memberStats.m_totalReceivedBytes = stats.m_dataReceived;
                        memberStats.m_totalSentBytes = stats.m_dataSend;
                        memberStats.m_packetsLost = stats.m_packetLost;
                        memberStats.m_packetsReceived = stats.m_packetReceived;
                        memberStats.m_packetsSent = stats.m_packetSend;
                    }
                }
            }

            #if GRIDMATE_DEBUG_ENABLED
            if (s_DumpStatsEnabled > 0)
            {
                DumpNetworkStatistics();
                m_gameStatistics = GameStatistics();
            }
            #endif // GRIDMATE_DEBUG_ENABLED
        }
    }

    //-----------------------------------------------------------------------------
    void Network::ClearNetworkStatistics()
    {
        m_gameStatistics = GameStatistics();
        m_statisticsPerChannel.clear();
    }

    //-----------------------------------------------------------------------------
    GameStatistics& Network::GetGameStatistics()
    {
        return m_gameStatistics;
    }

    //-----------------------------------------------------------------------------
    CarrierStatistics Network::GetCarrierStatistics()
    {
        if (!m_statisticsPerChannel.empty())
        {
            return m_statisticsPerChannel.begin()->second;
        }

        return CarrierStatistics();
    }


    //-----------------------------------------------------------------------------
    void Network::BindNewEntitiesToNetwork()
    {
        m_newServerEntities.clear();

        for (EntityReplicaMap::iterator iterNewProxy = m_newProxyEntities.begin(); iterNewProxy != m_newProxyEntities.end(); )
        {
            EntityReplicaPtr entityChunk = iterNewProxy->second;
            entityChunk->HandleNewlyReceivedNow();

            if ((entityChunk->GetFlags() & EntityReplica::kFlag_NewlyReceived) == 0)
            {
                iterNewProxy = m_newProxyEntities.erase(iterNewProxy);
            }
            else
            {
                ++iterNewProxy;
            }
        }
    }

    //-----------------------------------------------------------------------------
    void Network::GetBandwidthStatistics(SBandwidthStats* const pStats)
    {
        pStats->m_numChannels = m_statisticsPerChannel.size();

        if (!m_statisticsPerChannel.empty())
        {
            const auto& carrierStats = m_statisticsPerChannel.begin()->second;
            pStats->m_1secAvg.m_totalPacketsDropped = carrierStats.m_packetsLost;
            pStats->m_1secAvg.m_totalPacketsRecvd = carrierStats.m_packetsReceived;
            pStats->m_1secAvg.m_totalPacketsSent = carrierStats.m_packetsSent;
            pStats->m_1secAvg.m_totalBandwidthRecvd = carrierStats.m_totalReceivedBytes;
            pStats->m_1secAvg.m_totalBandwidthSent = carrierStats.m_totalSentBytes;
        }
    }

    //-----------------------------------------------------------------------------
    void Network::GetPerformanceStatistics([[maybe_unused]] SNetworkPerformance* pSizer)
    {
        // Network Cpu stats.
    }

    //-----------------------------------------------------------------------------
    void Network::GetProfilingStatistics(SNetworkProfilingStats* const pStats)
    {
        pStats->m_maxBoundObjects = uint(~0);
        pStats->m_numBoundObjects = m_activeEntityReplicaMap.size();
        // pStats->m_ProfileInfoStats Part of NET_PROFILE macros
    }
    //! Called when need serializer for the legacy aspect
    void Network::AcquireSerializer(WriteBuffer& wb, NetSerialize::AcquireSerializeCallback callback)
    {
        NetSerialize::EntityNetSerializerCollectState serializerImpl(wb);
        CSimpleSerialize<NetSerialize::EntityNetSerializerCollectState> serializer(serializerImpl);
        callback(&serializer);
    }
    //! Called when need deserializer for the legacy aspect
    void Network::AcquireDeserializer(ReadBuffer& rb, NetSerialize::AcquireSerializeCallback callback)
    {
        NetSerialize::EntityNetSerializerDispatchState serializerImpl(rb);
        CSimpleSerialize<NetSerialize::EntityNetSerializerDispatchState> serializer(serializerImpl);
        callback(&serializer);
    }
    //-----------------------------------------------------------------------------
    void Network::MarkAsConnectedServer()
    {
        CryLog("Marked as hosting server.");

        gEnv->bServer = true;
        gEnv->bMultiplayer = true;
    }
    //-----------------------------------------------------------------------------
    void Network::MarkAsConnectedClient()
    {
        CryLog("Marked as connected client.");

        gEnv->bServer = false;
        gEnv->bMultiplayer = true;
    }
    //-----------------------------------------------------------------------------
    void Network::MarkAsLocalOnly()
    {
        CryLog("Marked as local only.");

        gEnv->bServer = true;
        gEnv->bMultiplayer = false;
    }
} // namespace GridMate
