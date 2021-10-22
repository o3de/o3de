/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Carrier/Carrier.h>

#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/SystemReplicas.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaStatus.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Replica/MigrationSequence.h>
#include <GridMate/Replica/Tasks/ReplicaMarshalTasks.h>
#include <GridMate/Replica/Tasks/ReplicaUpdateTasks.h>
#include <GridMate/Replica/ReplicaDrillerEvents.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Serialize/CompressionMarshal.h>

#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/MathUtils.h>

namespace GridMate
{
    bool                    ReplicaManager::k_enableBackPressure = false;

    namespace ReplicaDebug
    {
        bool g_SendDbgHeartbeat = false; // send heartbeats from this station
        bool g_TrackDbgHeartbeat = false; // track heartbeats received on this station
        int g_MaxTicksPerHeartbeat = 50; // max allowed ticks without heartbeats
    };
    //-----------------------------------------------------------------------------
    template<typename iter_type>
    iter_type find(iter_type first, iter_type last, ReplicaId repId)
    {
        for (; first != last; ++first)
        {
            if ((*first)->GetRepId() == repId)
            {
                break;
            }
        }
        return first;
    }
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // ReplicaPeer
    //-----------------------------------------------------------------------------
    ReplicaPeer::ReplicaPeer(ReplicaManager* manager, ConnectionID connId, RemotePeerMode mode)
        : m_flags(0)
        , m_peerId(InvalidReplicaPeerId)
        , m_connId(connId)
        , m_mode(mode)
        , m_reliableOutBuffer(EndianType::IgnoreEndian)
        , m_unreliableOutBuffer(EndianType::IgnoreEndian)
        , m_zoneMask(ZoneMask_All)
        , m_rm(manager)
        , m_lastReceiveTicks(0)
        , m_avgSendRateBurst(0.f)
        , m_sentBytes(0)
        , m_sendBytesAllowed(0)
    {
        AZ_Assert(m_rm, "No replica manager specified");
        if (m_rm->IsInitialized())
        {
            SetEndianType(manager->GetGridMate()->GetDefaultEndianType());
        }
        ResetBuffer();
    }
    //-----------------------------------------------------------------------------
    void ReplicaPeer::SetEndianType(EndianType endianType)
    {
        m_reliableOutBuffer.SetEndianType(endianType);
        m_unreliableOutBuffer.SetEndianType(endianType);
    }
    //-----------------------------------------------------------------------------
    void ReplicaPeer::Add(Replica* pObj)
    {
        auto mapIt = m_objectsMap.insert_key(pObj->GetRepId());
        if (mapIt.second)
        {
            mapIt.first->second.m_replica = pObj;
            m_objectsTimeSort.insert(mapIt.first->second);
            pObj->m_upstreamHop = this;
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaPeer::Remove(Replica* pObj)
    {
        auto mapIt = m_objectsMap.find(pObj->GetRepId());
        if (mapIt != m_objectsMap.end())
        {
            m_objectsTimeSort.erase(mapIt->second);
            m_objectsMap.erase(mapIt);

            if (IsOrphan() && m_objectsTimeSort.empty())
            {
                m_rm->OnPeerReadyToRemove(this);
            }
            return;
        }

        if (pObj->m_upstreamHop == this)
        {
            pObj->m_upstreamHop = nullptr;
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaPeer::Accept()
    {
        m_flags |= PeerFlags::Peer_New | PeerFlags::Peer_Accepted;
        m_rm->OnPeerAccepted(this);
    }
    //-----------------------------------------------------------------------------
    void ReplicaPeer::SendBuffer(Carrier* carrier, unsigned char commChannel, const AZ::u32 replicaManagerTimer)
    {
        AZ_Assert(carrier, "No available net layer!");

        bool hasReliableData = m_reliableOutBuffer.Size() > m_reliableTimestamp.GetOffsetAfterMarker();
        bool hasUnreliableData = m_unreliableOutBuffer.Size() > m_unreliableTimestamp.GetOffsetAfterMarker();

        if (ReplicaDebug::g_SendDbgHeartbeat)
        {
            if (hasReliableData)
            {
                m_reliableOutBuffer.Write(Cmd_Heartbeat);
            }
            else
            {
                m_unreliableOutBuffer.Write(Cmd_Heartbeat);
                hasUnreliableData = true;
            }
        }

        if (hasReliableData)
        {
            m_reliableTimestamp = replicaManagerTimer;
#ifdef REPLICA_MSG_CRC_CHECK
            m_reliableMsgCrc = AZ::Crc32(m_reliableOutBuffer.Get() + m_reliableMsgCrc.GetOffsetAfterMarker(), m_reliableOutBuffer.Size() - m_reliableMsgCrc.GetOffsetAfterMarker());
#endif

            auto callback = AZStd::make_unique<PeerAckCallbacks>((m_reliableCallbacks));
            carrier->SendWithCallback(m_reliableOutBuffer.Get(), static_cast<unsigned int>(m_reliableOutBuffer.Size()), AZStd::move(callback), GetConnectionId(), Carrier::SEND_RELIABLE, Carrier::PRIORITY_NORMAL, commChannel);

            EBUS_EVENT(Debug::ReplicaDrillerBus, OnSend, GetId(), m_reliableOutBuffer.Get(), m_reliableOutBuffer.Size(), true);
        }

        if (hasUnreliableData)
        {
            m_unreliableTimestamp = replicaManagerTimer;
#ifdef REPLICA_MSG_CRC_CHECK
            m_unreliableMsgCrc = AZ::Crc32(m_unreliableOutBuffer.Get() + m_unreliableMsgCrc.GetOffsetAfterMarker(), m_unreliableOutBuffer.Size() - m_unreliableMsgCrc.GetOffsetAfterMarker());
#endif
            if (m_unreliableOutBuffer.Size() > carrier->GetMessageMTU())
            {
                AZ_TracePrintf("GridMate", "SendBuffer [0x%p]: Unreliable replica update exceeds MTU (size=%u, MTU=%u), forcing reliable for this send.\n", GetConnectionId(), static_cast<unsigned int>(m_unreliableOutBuffer.Size()), carrier->GetMessageMTU());
            }

            auto callback = AZStd::make_unique<PeerAckCallbacks>((m_unreliableCallbacks));
            carrier->SendWithCallback(m_unreliableOutBuffer.Get(), static_cast<unsigned int>(m_unreliableOutBuffer.Size()), AZStd::move(callback), GetConnectionId(), Carrier::SEND_UNRELIABLE, Carrier::PRIORITY_NORMAL, commChannel);

            EBUS_EVENT(Debug::ReplicaDrillerBus, OnSend, GetId(), m_unreliableOutBuffer.Get(), m_unreliableOutBuffer.Size(), false);
        }
        // prepare for next cycle
        ResetBuffer();
    }
    //-----------------------------------------------------------------------------
    void ReplicaPeer::ResetBuffer()
    {
        m_reliableOutBuffer.Clear();
        m_unreliableOutBuffer.Clear();

#ifdef REPLICA_MSG_CRC_CHECK
        m_reliableMsgCrc = m_reliableOutBuffer.InsertMarker<AZ::Crc32>();
        m_unreliableMsgCrc = m_unreliableOutBuffer.InsertMarker<AZ::Crc32>();
#endif
        m_reliableTimestamp = m_reliableOutBuffer.InsertMarker<AZ::u32>();
        m_unreliableTimestamp = m_unreliableOutBuffer.InsertMarker<AZ::u32>();
    }
    //-----------------------------------------------------------------------------
    ReplicaPtr ReplicaPeer::GetReplica(ReplicaId repId)
    {
        auto i = m_objectsMap.find(repId);
        if (i == m_objectsMap.end())
        {
            return nullptr;
        }

        return i->second.m_replica;
    }
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // RepIdMgrClient
    //-----------------------------------------------------------------------------
    void ReplicaManager::RepIdMgrClient::AddBlock(RepIdSeed seed)
    {
        auto ret = m_idBlocks.insert_key(seed);
        if (ret.second)
        {
            ret.first->second = static_cast<ReplicaId>(seed);
            m_nAvailableIds += GM_REPIDS_PER_BLOCK;
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::RepIdMgrClient::RemoveBlock(RepIdSeed seed)
    {
        auto entry = m_idBlocks.find(seed);
        if (entry != m_idBlocks.end())
        {
            size_t idsRemoved = entry->first + GM_REPIDS_PER_BLOCK - entry->second;
            m_idBlocks.erase(entry);
            m_nAvailableIds -= idsRemoved;
        }
    }
    //-----------------------------------------------------------------------------
    ReplicaId ReplicaManager::RepIdMgrClient::Alloc()
    {
        AZ_Assert(m_nAvailableIds > 0, "We ran out of available replica ids!");
        for (auto it = m_idBlocks.begin(); it != m_idBlocks.end(); ++it)
        {
            if (it->second < it->first + GM_REPIDS_PER_BLOCK)
            {
                m_nAvailableIds--;
                return it->second++;
            }
        }
        AZ_Assert(false, "We are supposed to have available replica ids but we couldn't find any empty slots!");
        return InvalidReplicaId;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::RepIdMgrClient::Dealloc(ReplicaId id)
    {
        // We don't support reusing replica ids
        (void) id;
    }
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // ReplicaManager
    //-----------------------------------------------------------------------------
    ReplicaManager::ReplicaManager()
        : m_flags(0)
        , m_self(this, InvalidConnectionID, Mode_Client)
        , m_marshalingTasks(&m_tasksAllocator)
        , m_updateTasks(&m_tasksAllocator)
        , m_peerUpdateTasks(&m_tasksAllocator)
        , m_latchedCarrierTime(0)
        , m_autoBroadcast(true)
    { }
    //-----------------------------------------------------------------------------
    void ReplicaManager::Init(const ReplicaMgrDesc& desc)
    {
        AZ_Assert((m_flags & Rm_Initialized) == 0, "ReplicaManager already initialized.");
        m_cfg = desc;

        m_self.SetEndianType(GetGridMate()->GetDefaultEndianType());

        AZ_Assert(m_cfg.m_carrier, "Carrier must be valid!");
        if (m_cfg.m_targetSendTimeMS < m_cfg.m_carrier->GetMaxSendRate())
        {
            //AZ_TracePrintf("GridMate", "targetSendTimeMS is too high for underlying carrier!\n");
            m_cfg.m_targetSendTimeMS = m_cfg.m_carrier->GetMaxSendRate();
        }
        m_lastCheckTime = AZStd::chrono::system_clock::now();
        m_nextSendTime = m_lastCheckTime + AZStd::chrono::milliseconds(m_cfg.m_targetSendTimeMS);
        m_receiveBuffer.resize(m_cfg.m_carrier->GetMessageMTU());

        m_currentFrameTime.m_localTime = m_currentFrameTime.m_realTime = m_latchedCarrierTime = m_cfg.m_carrier->GetTime();

        AZ_Assert(m_cfg.m_myPeerId, "myPeerId has to be a non-zero globally unique id, it is used to identify this peer on the network.");
        m_self.m_peerId = m_cfg.m_myPeerId;

        m_sessionInfo = CreateReplicaChunk<ReplicaInternal::SessionInfo>(this);
        m_flags = Rm_Initialized;

        if (IsUsingFixedTimeStep())
        {
            m_fixedTimeStep.SetTargetUpdateRate(m_cfg.m_targetFixedTimeStepsPerSecond);
        }

        {
            AZ::PoolAllocator::Descriptor allocDesc;

#if !defined(AZ_DEBUG_BUILD)
            allocDesc.m_allocationRecords = false;
            allocDesc.m_stackRecordLevels = 0;
#endif
            allocDesc.m_pageSize = 256;
            allocDesc.m_maxAllocationSize = 64;
            allocDesc.m_numStaticPages = 1024;
            allocDesc.m_pageAllocator = &AZ::AllocatorInstance<GridMateAllocatorMP>::Get();

            m_tasksAllocator.Create(allocDesc);
        }

        if ((m_cfg.m_roles & ReplicaMgrDesc::Role_SyncHost) != 0)
        {
            Promote();
        }

        CarrierEventBus::Handler::BusConnect(m_cfg.m_carrier->GetGridMate());
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::Shutdown()
    {
        AZ_Assert((m_flags & Rm_Processing) == 0, "Cannot shut down ReplicaManager while still processing!");
        if (m_flags & Rm_Terminating)
        {
            //AZ_TracePrintf("GridMate", "ReplicaManager is already shutting down!\n");
            return;
        }
        m_flags = Rm_Terminating;

        for (auto& migration : m_activeMigrations)
        {
            delete migration.second;
        }
        m_activeMigrations.clear();

        ReplicaContext rc(this, GetTime());
        m_sessionInfo.reset();

        // remove all peers
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);
            ReplicaPtr pObj;

            for (ReplicaPeerList::iterator it = m_remotePeers.begin(); it != m_remotePeers.end(); ++it)
            {
                ReplicaPeer* peer = *it;
                for (auto& replicaObject : peer->m_objectsTimeSort)
                {
                    UnregisterReplica(replicaObject.m_replica, rc);
                }
                delete peer;
            }

            m_remotePeers.clear();
        }
        
        // remove self
        for (auto& replicaObject : m_self.m_objectsTimeSort)
        {
            UnregisterReplica(replicaObject.m_replica, rc);
        }
        m_self.m_objectsTimeSort.clear();
        m_self.m_objectsMap.clear();

        CarrierEventBus::Handler::BusDisconnect(m_cfg.m_carrier->GetGridMate());

        AZ_Assert(m_replicas.empty(), "There shouldn't be registered replicas left since we have cleared out all the peers including ourselves!");
        m_cfg.m_carrier = nullptr; /// We assume the transport is dead.

        m_marshalingTasks.Clear();
        m_updateTasks.Clear();
        m_peerUpdateTasks.Clear();
        m_tasksAllocator.Destroy();
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::Promote()
    {
        AZ_Assert(IsInitialized(), "ReplicaMgr has not been initialized!");
        AZ_Assert(!HasValidHost(), "There is already a valid host on the network!");

        m_self.MakeSyncHost(true);

        //AZ_TracePrintf("GridMate", "Local peer id=0x%x promoted to sync host.\n", m_self.GetId());

        // if we don't have sessionInfo then either we just got started as the host
        // or we never fully connected to the host. Either way, init ourselves as host
        if (!m_sessionInfo->GetReplica())
        {
            Replica* replica = Replica::CreateReplica("ReplicaSessionInfo");
            replica->AttachReplicaChunk(m_sessionInfo);
            replica->SetMigratable(true);

            // register global session info
            // this one is kind of special so don't go through AddPrimary()
            ReplicaContext rc(this, GetTime(), &m_self);
            replica->m_createTime = rc.m_realTime;
            replica->SetRepId(RepId_SessionInfo);
            m_sessionInfo->m_nextAvailableIdBlock.Set(Max_Reserved_Cmd_Or_Id);

            // give ourselves a block of ids if we don't have any
            if (m_localIdBlocks.Available() == 0)
            {
                m_localIdBlocks.AddBlock(ReserveIdBlock(m_self.GetId()));
            }

            m_self.Add(m_sessionInfo->GetReplica());
            replica->InitReplica(this);
            RegisterReplica(replica, true, rc);
        }
        else
        {
            // take over ownership of the session info
            AZ_Assert(!m_sessionInfo->IsPrimary(), "We just became host but we were already the owner of sessionInfo!");
            AZ_Assert(m_sessionInfo->m_pHostPeer->IsOrphan(), "We can't be promoted if we are still connected to the host!");
            m_sessionInfo->m_pHostPeer->Remove(m_sessionInfo->GetReplica());
            m_self.Add(m_sessionInfo->GetReplica());
            OnReplicaMigrated(m_sessionInfo->GetReplica(), true, ReplicaContext(this, GetTime(), &m_self));
        }

        // initialize accepted peer list to currently known peers
        vector<PeerId> acceptedPeers;
        acceptedPeers.push_back(m_self.GetId());
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);
            for (ReplicaPeerList::iterator it = m_remotePeers.begin(); it != m_remotePeers.end(); ++it)
            {
                ReplicaPeer* peer = *it;
                if (peer->GetId() && !peer->IsOrphan())
                {
                    acceptedPeers.push_back(peer->GetId());
                }
            }
        }

        m_sessionInfo->m_acceptedPeers.Set(acceptedPeers);

        // reconcile peers
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);
            for (ReplicaPeerList::iterator it = m_remotePeers.begin(); it != m_remotePeers.end(); ++it)
            {
                ReplicaPeer* peer = *it;

                // Queue request to start migration process on orphan peers.
                // At the very least we need to know their peer id, otherwise there
                // is no way to synchonize migration state
                if (peer->IsOrphan() && peer->GetId())
                {
                    m_sessionInfo->RequestPeerMigration(peer->GetId());
                }
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::RegisterUserContext(int key, void* data)
    {
        m_userContexts.insert(AZStd::make_pair(key, data));
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::UnregisterUserContext(int key)
    {
        m_userContexts.erase(key);
    }
    //-----------------------------------------------------------------------------
    void* ReplicaManager::GetUserContext(int key)
    {
        UserContextMapType::iterator iter = m_userContexts.find(key);
        if (iter != m_userContexts.end())
        {
            return iter->second;
        }
        return nullptr;
    }
    //-----------------------------------------------------------------------------
    RepIdSeed ReplicaManager::ReserveIdBlock(PeerId requestor)
    {
        (void) requestor;
        AZ_Assert(IsSyncHost(), "Cannot reserve id blocks on a non-synchost.");
        RepIdSeed idBlock = m_sessionInfo->m_nextAvailableIdBlock.Get();
        AZ_Assert(idBlock + GM_REPIDS_PER_BLOCK > idBlock, "Replica host ran out of id blocks!");
        m_sessionInfo->m_nextAvailableIdBlock.Set(idBlock + GM_REPIDS_PER_BLOCK);
        return idBlock;
    }
    //-----------------------------------------------------------------------------
    size_t ReplicaManager::ReleaseIdBlock(PeerId requestor)
    {
        (void) requestor;
        AZ_Assert(IsSyncHost(), "Cannot reserve id blocks on a non-synchost.");
        // We don't support reusing id blocks
        return 0;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::AddPeer(ConnectionID connId, RemotePeerMode peerMode)
    {
        AZ_Assert(IsSyncHost() || peerMode == Mode_Peer, "Mode_Client can only be added to hosts!");
        ReplicaPeer* pPeer = aznew ReplicaPeer(this, connId, peerMode);

        m_mutexRemotePeers.lock();
        m_remotePeers.push_back(pPeer);
        m_mutexRemotePeers.unlock();

        // immediate introduce ourselves to this peer (only if we are not the host)
        if (!IsSyncHost())
        {
            SendGreetings(pPeer);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::DiscardOrphans(PeerId orphanId)
    {
        if (IsSyncHost())
        {
            return;
        }
        
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);

            for (ReplicaPeerList::iterator iPeer = m_remotePeers.begin(); iPeer != m_remotePeers.end(); ++iPeer)
            {
                ReplicaPeer* peer = *iPeer;
                if (peer->GetId() == orphanId)
                {
                    ReplicaContext rc(this, GetTime(), peer);
                    for (auto iObj = peer->m_objectsTimeSort.begin(); iObj != peer->m_objectsTimeSort.end(); ++iObj)
                    {
                        RemoveReplicaFromDownstream(iObj->m_replica, rc);
                    }

                    OnPeerReadyToRemove(peer);
                    break;
                }
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::RemovePeer(ConnectionID connId)
    {
        // We are shutting down, don't do anything
        if (m_flags & Rm_Terminating)
        {
            return;
        }
        
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);

        for (ReplicaPeerList::iterator iPeer = m_remotePeers.begin(); iPeer != m_remotePeers.end(); ++iPeer)
        {
            ReplicaPeer* peer = *iPeer;
            if (peer->GetConnectionId() == connId)
            {
                peer->m_connId = InvalidConnectionID;

                // immediately remove all non-migratable replicas
                // then orphan the peer and wait for the host to take further action
                ReplicaContext rc(this, GetTime(), peer);
                for (auto iObj = peer->m_objectsTimeSort.begin(); iObj != peer->m_objectsTimeSort.end(); )
                {
                    ReplicaPtr pObj = iObj->m_replica;
                    if (pObj && !pObj->IsMigratable())
                    {
                        iObj = peer->m_objectsTimeSort.erase(iObj);
                        peer->m_objectsMap.erase(pObj->GetRepId());
                        RemoveReplicaFromDownstream(pObj, rc);
                    }
                    else
                    {
                        ++iObj;
                    }
                }

                if (IsSyncHost() && peer->GetId())
                {
                    // remove from the peer acceptance list
                    vector<PeerId> newList;
                    for (auto it = m_sessionInfo->m_acceptedPeers.Get().begin(); it != m_sessionInfo->m_acceptedPeers.Get().end(); ++it)
                    {
                        PeerId peerId = *it;
                        if (peerId != peer->GetId())
                        {
                            newList.push_back(peerId);
                        }
                    }
                    m_sessionInfo->m_acceptedPeers.Set(newList);

                    // stop waiting for any pending reports from this peer
                    // since it will never respond
                    for (auto it = m_sessionInfo->m_pendingPeerReports.begin(); it != m_sessionInfo->m_pendingPeerReports.end(); )
                    {
                        vector<PeerId>& pending = it->second;
                        for (auto it2 = pending.begin(); it2 != pending.end(); ++it2)
                        {
                            if (*it2 == peer->GetId())
                            {
                                pending.erase(it2);
                                break;
                            }
                        }

                        // removed all pending reports
                        if (pending.empty())
                        {
                            AZ_Assert(peer->GetId() != it->first, "Peer was waiting for pending report from itself");
                            it = m_sessionInfo->m_pendingPeerReports.erase(it);
                            OnPendingReportsReceived(it->first);
                        }
                        else
                        {
                            ++it;
                        }
                    }

                    // Request to start migration process for this peer
                    // At the very least we need to know their peer id, otherwise there
                    // is no way to synchonize migration state.
                    m_sessionInfo->RequestPeerMigration(peer->GetId());
                }

                // not waiting for reports for this peer -> can start migration
                if (IsSyncHost() && !m_sessionInfo->HasPendingReports(peer->GetId()))
                {
                    OnMigratePeer(peer);
                }
                return;
            }
        }
    }
    //-----------------------------------------------------------------------------
    ReplicaPeer* ReplicaManager::FindPeer(PeerId peerId)
    {
        if (peerId == GetLocalPeerId())
        {
            return &m_self;
        }

        // TODO: make peer list hash map
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);

        for (ReplicaPeer* peer : m_remotePeers)
        {
            if (peer->GetId() == peerId)
            {
                return peer;
            }
        }

        return nullptr;
    }
    //-----------------------------------------------------------------------------
    bool ReplicaManager::AcceptPeer(ReplicaPeer* peer)
    {
        // trivial case
        if (peer->m_flags & PeerFlags::Peer_Accepted)
        {
            return true;
        }

        // if we have peer's id and he hasn't been accepted yet then see if we should accept him
        if (peer->GetId())
        {
            // if we are the host then accept him right away
            if (IsSyncHost())
            {
                // if we are the host this is when we send our greetings
                // and add the peer to the accept list
                SendGreetings(peer);
                vector<PeerId> acceptedPeers = m_sessionInfo->m_acceptedPeers.Get();
                acceptedPeers.push_back(peer->GetId());
                m_sessionInfo->m_acceptedPeers.Set(acceptedPeers);
                peer->Accept();

                // else we need to see if the host told us to accept him
            }
            else
            {
                if ((m_sessionInfo && m_sessionInfo->IsInAcceptList(peer->GetId())) || peer->IsSyncHost())
                {
                    peer->Accept();
                }
            }
        }
        return !!(peer->m_flags & PeerFlags::Peer_Accepted);
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnPeerReplicaActivated(ReplicaInternal::PeerReplica::Ptr peerReplica)
    {
        m_peerReplicas.insert(AZStd::make_pair(peerReplica->m_peerId.Get(), peerReplica));
        if (peerReplica->IsProxy())
        {
            EBUS_EVENT_ID(GetGridMate(), ReplicaMgrCallbackBus, OnNewPeer, peerReplica->m_peerId.Get(), this);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnPeerReplicaDeactivated(ReplicaInternal::PeerReplica::Ptr peerReplica)
    {
        if (peerReplica->IsProxy())
        {
            EBUS_EVENT_ID(GetGridMate(), ReplicaMgrCallbackBus, OnPeerRemoved, peerReplica->m_peerId.Get(), this);
        }
        m_peerReplicas.erase(peerReplica->m_peerId.Get());
    }
    //-----------------------------------------------------------------------------
    TimeContext ReplicaManager::GetTime() const
    {
        // Return the time context cached at the beginning of the frame (during Unmarshal)
        return m_currentFrameTime;
    }
    AZ::u32 ReplicaManager::GetTimeForNetworkTimestamp() const
    {
        if (IsUsingFixedTimeStep())
        {
            return static_cast<AZ::u32>(m_fixedTimeStep.GetCurrentTime());
        }
        else
        {
            return m_latchedCarrierTime;
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::SetSendTimeInterval(unsigned int sendTimeMs)
    {
        m_cfg.m_targetSendTimeMS = sendTimeMs;
        m_nextSendTime = m_lastCheckTime + AZStd::chrono::milliseconds(m_cfg.m_targetSendTimeMS);
    }
    //-----------------------------------------------------------------------------
    unsigned int ReplicaManager::GetSendTimeInterval() const
    {
        return m_cfg.m_targetSendTimeMS;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::SetSendLimit(unsigned int sendLimitBytesPerSec)
    {
        m_cfg.m_targetSendLimitBytesPerSec = sendLimitBytesPerSec;
    }
    //-----------------------------------------------------------------------------
    unsigned int ReplicaManager::GetSendLimit() const
    {
        return m_cfg.m_targetSendLimitBytesPerSec;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::SetSendLimitBurstRange(float rangeSec)
    {
        m_cfg.m_targetSendLimitBurst = AZStd::GetMax(1.f, rangeSec);
    }
    //-----------------------------------------------------------------------------
    float ReplicaManager::GetSendLimitBurstRange() const
    {
        return m_cfg.m_targetSendLimitBurst;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::SetAutoBroadcast(bool isEnabled)
    {
        m_autoBroadcast = isEnabled;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::SetLocalLagAmt(unsigned int ms)
    {
        if (!IsSyncHost())
        {
            AZ_Error("ReplicaManager", false, "SetLocalLagAmt() can only be called on the replication host!");
        }
        else
        {
            m_sessionInfo->m_localLagAmt.Set(ms);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::UnregisterReplica(const ReplicaPtr& pObj, const ReplicaContext& rc)
    {
        pObj->ClearPendingRPCs();
        pObj->Deactivate(rc);

        if (pObj->IsDirty())
        {
            RemoveFromDirtyList(*pObj);
        }

        m_replicas.erase(pObj->GetRepId());
        pObj->m_flags &= Replica::Rep_Traits;
        pObj->SetRepId(Invalid_Cmd_Or_Id);
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::RemoveReplicaFromDownstream(const ReplicaPtr& pObj, const ReplicaContext& rc)
    {
        pObj->Deactivate(rc);

        if (pObj->IsDirty())
        {
            RemoveFromDirtyList(*pObj);
        }

        m_replicas.erase(pObj->GetRepId());

        // Flags are reset and id is cleared after this task executes
        m_marshalingTasks.Add<ReplicaMarshalZombieTask>(pObj);
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::MigrateReplica(ReplicaPtr replica, PeerId newOwnerId)
    {
        auto iter = m_activeMigrations.insert_key(replica->GetRepId());
        if (iter.second)
        {
            iter.first->second = aznew ReplicaInternal::MigrationSequence(replica.get(), newOwnerId);
        }
        else
        {
            iter.first->second->ModifyNewOwner(newOwnerId);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::AnnounceReplicaMigrated(ReplicaId replicaId, PeerId newOwnerId)
    {
        auto it = m_peerReplicas.find(GetLocalPeerId());
        if (it != m_peerReplicas.end())
        {
            it->second->OnReplicaMigrated(replicaId, newOwnerId);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnReplicaMigrated(ReplicaPtr replica, bool isOwner, const ReplicaContext& rc)
    {
        ChangeReplicaOwnership(replica, rc, isOwner);

        if (isOwner)
        {
            ReplicaStatus::Ptr replicaStatus = AZStd::static_pointer_cast<ReplicaStatus>(replica->m_replicaStatus);
            replicaStatus->SetUpstreamSuspended(false);
            replica->m_flags |= Replica::Rep_ChangedOwner;
            replicaStatus->m_ownerSeq.Set(replicaStatus->m_ownerSeq.Get() + 1);
            OnReplicaChanged(replica);
        }
        else if (IsSyncHost())
        {
            replica->m_flags |= Replica::Rep_ChangedOwner;
            OnReplicaChanged(replica);
        }

        UpdateReplicaTargets(replica);
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::AckUpstreamSuspended(ReplicaId replicaId, PeerId sendTo, AZ::u32 requestTime)
    {
        auto it = m_peerReplicas.find(sendTo);
        if (it != m_peerReplicas.end())
        {
            it->second->OnAckUpstreamSuspended(replicaId, GetLocalPeerId(), requestTime);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnAckUpstreamSuspended(ReplicaId replicaId, PeerId from, AZ::u32 requestTime)
    {
        auto iter = m_activeMigrations.find(replicaId);
        if (iter != m_activeMigrations.end())
        {
            iter->second->OnReceivedAckUpstreamSuspended(from, requestTime);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::AckDownstream(ReplicaId replicaId, PeerId sendTo, AZ::u32 requestTime)
    {
        auto it = m_peerReplicas.find(sendTo);
        if (it != m_peerReplicas.end())
        {
            it->second->OnAckDownstream(replicaId, GetLocalPeerId(), requestTime);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnAckDownstream(ReplicaId replicaId, PeerId from, AZ::u32 requestTime)
    {
        auto iter = m_activeMigrations.find(replicaId);
        if (iter != m_activeMigrations.end())
        {
            iter->second->OnReceivedAckDownstream(from, requestTime);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::UpdateFromReplicas()
    {
        AZ_PROFILE_FUNCTION(GridMate);

        if (!IsInitialized())
        {
            return;
        }
        m_flags |= Rm_Processing;
        m_latchedCarrierTime = m_cfg.m_carrier->GetTime();

        if (IsUsingFixedTimeStep())
        {
            m_fixedTimeStep.UpdateFixedTimeStep();
        }
        m_updateTasks.Run(this);
        m_peerUpdateTasks.Run(this);

        m_flags &= ~Rm_Processing;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::UpdateReplicas()
    {
        AZ_PROFILE_FUNCTION(GridMate);

        if (!IsInitialized())
        {
            return;
        }
        m_flags |= Rm_Processing;

        ReplicaContext rc(this, GetTime());
        for (auto& replicaObject : m_self.m_objectsTimeSort)
        {
            if (replicaObject.m_replica->IsPrimary())
            {
                replicaObject.m_replica->UpdateReplica(rc);
            }
        }

        {
            for (auto itMigration = m_activeMigrations.begin(); itMigration != m_activeMigrations.end(); )
            {
                itMigration->second->Update();
                if (itMigration->second->IsDone())
                {
                    delete itMigration->second;
                    itMigration = m_activeMigrations.erase(itMigration);
                }
                else
                {
                    ++itMigration;
                }
            }
        }

        const unsigned int tombstoneExpirationMs = 5000;
        unsigned int curNetTime = GetTime().m_realTime;
        for (TombstoneRecords::iterator iTombstone = m_tombstones.begin(); iTombstone != m_tombstones.end(); )
        {
            if (iTombstone->second + tombstoneExpirationMs < curNetTime)
            {
                iTombstone = m_tombstones.erase(iTombstone);
            }
            else
            {
                ++iTombstone;
            }
        }

        m_flags &= ~Rm_Processing;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::Marshal()
    {
        AZ_PROFILE_FUNCTION(GridMate);

        if (!IsReady())
        {
            return;
        }

        AZStd::chrono::system_clock::time_point now(AZStd::chrono::system_clock::now());
        AZStd::chrono::milliseconds dt(now - m_lastCheckTime);
        m_lastCheckTime = now;

        if (now + dt < m_nextSendTime)
        {
            return; // we'll probably come back before the train leaves the station...
        }
        m_nextSendTime += AZStd::chrono::milliseconds(m_cfg.m_targetSendTimeMS);

        m_flags |= Rm_Processing;

        while (!m_dirtyReplicas.empty())
        {
            auto& replica = m_dirtyReplicas.front();
            if (!replica.HasMarshalingTask())
            {
                m_marshalingTasks.Add<ReplicaMarshalTask>(&replica);
            }
            RemoveFromDirtyList(replica);
        }
        m_marshalingTasks.Run(this);

        // send collected updates to peers
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);
            for (ReplicaPeerList::iterator iPeer = m_remotePeers.begin(); iPeer != m_remotePeers.end(); ++iPeer)
            {
                ReplicaPeer* pRemote = *iPeer;
                if (!pRemote->IsOrphan())
                {
                    pRemote->SendBuffer(m_cfg.m_carrier, m_cfg.m_commChannel, GetTimeForNetworkTimestamp());
                }
                pRemote->SetNew(false);
            }
        }

        m_flags &= ~Rm_Processing;
    }
    //-----------------------------------------------------------------------------
    ReplicaPtr ReplicaManager::FindReplica(ReplicaId replicaId)
    {
        ReplicaMap::iterator iObj = m_replicas.find(replicaId);
        if (iObj != m_replicas.end())
        {
            AZ_Assert(iObj->second, "Detected NULL replica pointer in replica map! (id=0x%x)", replicaId);
            return iObj->second;
        }
        return ReplicaPtr(nullptr);
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::_Unmarshal(ReadBuffer& rb, ReplicaPeer* pFrom)
    {
        AZ_Assert(pFrom, "Invalid peer pointer!");

        UnmarshalContext mc(ReplicaContext(this, GetTime(), pFrom));

#ifdef REPLICA_MSG_CRC_CHECK
        AZ::u32 msgCrc;
        if (!rb.Read(msgCrc))
        {
            return;
        }
        AZ::u32 actualCrc = AZ::Crc32(rb.GetCurrent(), rb.Left());
        (void) actualCrc;
        AZ_Assert(msgCrc == actualCrc, "Replica message crc mismatch!");
#endif
        if (!rb.Read(mc.m_timestamp))
        {
            return;
        }

        /*
         * The tail end of the buffer might have a few extra unused bits that no replica can fit into.
         */
        while (!rb.IsEmptyIgnoreTrailingBits())
        {
            // This is used later to report the buffer information to driller.
            const char* cmdBufferBegin = rb.GetCurrent();

            CmdId cmdhdr;
            if (!rb.Read(cmdhdr))
            {
                return;
            }

            mc.m_hasCtorData = false;
            switch (cmdhdr)
            {
            case Cmd_Greetings:
            {
                AZ_Assert(!pFrom->GetId(), "We should only receive one greetings msg from each peer!");
                if (pFrom->GetId())
                {
                    return;
                }

                PeerId newPeerId;
                if (!rb.Read(newPeerId))
                {
                    return;
                }

                {
                    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);
                    for (ReplicaPeer* peer : m_remotePeers)
                    {
                        if (peer->GetId() == newPeerId)
                        {
                            AZ_Assert(false, "Peer Id 0x%x is already claimed by another peer!", newPeerId);
                            return;
                        }
                    }
                }

                pFrom->m_peerId = newPeerId;

                bool peerIsHost;
                if (!rb.Read(peerIsHost))
                {
                    return;
                }

                if (peerIsHost)
                {
                    AZ_Assert(!IsSyncHost() && !HasValidHost(), "We already have a host!");
                    if (IsSyncHost() || HasValidHost())
                    {
                        return;
                    }
                    pFrom->MakeSyncHost(true);

                    RepIdSeed firstSeed;
                    if (!rb.Read(firstSeed))
                    {
                        return;
                    }

                    m_localIdBlocks.AddBlock(firstSeed);
                }
                break;
            }
            case Cmd_Heartbeat:
                pFrom->m_lastReceiveTicks = 0;
                break;
            case Cmd_NewOwner:
                if (GetSecurityOptions().m_enableStrictSourceValidation && !pFrom->IsSyncHost())
                {
                    AZ_Assert(false, "Cmd_NewOwner discarded due to ReplicationSecurityOptions::m_enableStrictSourceValidation! Discarding rest of message!");
                    return;
                }
            case Cmd_NewProxy:
            {
                mc.m_hasCtorData = true;

                bool isSyncStage;
                if (!rb.Read(isSyncStage))
                {
                    return;
                }

                bool isMigratable;
                if (!rb.Read(isMigratable))
                {
                    return;
                }

                AZ::u32 createTime;
                if (!rb.Read(createTime))
                {
                    return;
                }

                AZ::u32 ownerSeq;
                if (!rb.Read(ownerSeq))
                {
                    return;
                }

                ReplicaId repId;
                if (!rb.Read(repId))
                {
                    return;
                }

                PackedSize chunkSize;
                if (!rb.Read(chunkSize))
                {
                    return;
                }

                ReadBuffer replicaPayload = rb.ReadInnerBuffer(chunkSize);
                if (replicaPayload.IsValid())
                {
                    mc.m_iBuf = &replicaPayload;
                    mc.m_peer = pFrom;
                }
                else
                {
                    return;
                }

                // Find out if we already know about this replica
                ReplicaPtr pReplica = FindReplica(repId);
                if (pReplica)
                {
                    if (AZStd::static_pointer_cast<ReplicaStatus>(pReplica->m_replicaStatus)->m_ownerSeq.Get() > ownerSeq)
                    {
                        //AZ_TracePrintf("Gridmate", "Received replica, id 0x%x, with old ownerSeq. Discarding %d bytes\n", (int)repId, (int)chunkSize);
                        break;
                    }

                    if (cmdhdr == Cmd_NewOwner)
                    {
                        // find who originally owned the replica and transfer to the new owner
                        if (pReplica->m_upstreamHop != pFrom)
                        {
                            if (pReplica->m_upstreamHop)
                            {
                                pReplica->m_upstreamHop->Remove(pReplica.get());
                            }
                            pFrom->Add(pReplica.get());
                        }
                        OnReplicaMigrated(pReplica, false, mc);
                    }

                    if (GetSecurityOptions().m_enableStrictSourceValidation && pFrom != pReplica->m_upstreamHop)
                    {
                        AZ_Assert(false, "Cmd_NewProxy discarded due to ReplicationSecurityOptions::m_enableStrictSourceValidation! Discarding rest of message!");
                        return;
                    }

                    EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveReplicaBegin, pReplica.get(), cmdBufferBegin, rb.GetCurrent() - cmdBufferBegin);
                    pReplica->Unmarshal(mc);
                    OnReplicaUnmarshaled(pReplica);
                    EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveReplicaEnd, pReplica.get());
                }
                else
                {
                    // if failed to find replica or creating new proxy
                    pReplica = aznew Replica("");
                    pReplica->m_createTime = createTime;
                    pReplica->SetRepId(repId);
                    pFrom->Add(pReplica.get());
                    pReplica->InitReplica(this);
                    pReplica->SetSyncStage(isSyncStage);
                    pReplica->SetMigratable(isMigratable);

                    EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveReplicaBegin, pReplica.get(), cmdBufferBegin, rb.GetCurrent() - cmdBufferBegin);
                    pReplica->Unmarshal(mc);
                    ReplicaContext rc(this, GetTime(), pFrom);
                    RegisterReplica(pReplica, false, rc);
                    OnReplicaUnmarshaled(pReplica);
                    EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveReplicaEnd, pReplica.get());
                }
                break;
            }
            case Cmd_DestroyProxy:
            {
                ReplicaId repId;
                if (!rb.Read(repId))
                {
                    return;
                }

                if (GetSecurityOptions().m_enableStrictSourceValidation && !pFrom->IsSyncHost())
                {
                    ReplicaPtr pReplica = FindReplica(repId);
                    if (pReplica && pFrom != pReplica->m_upstreamHop)
                    {
                        AZ_Assert(false, "Cmd_DestroyProxy discarded due to ReplicationSecurityOptions::m_enableStrictSourceValidation! Discarding rest of message!");
                        return;
                    }
                }

                OnDestroyProxy(repId);
                break;
            }
            default:
            {
                if (cmdhdr < Cmd_Count)
                {
                    AZ_Assert(0, "Received invalid ReplicaId or Cmd 0x%x!", cmdhdr);
                    return;
                }

                ReplicaId repId = static_cast<ReplicaId>(cmdhdr);
                ReplicaPtr pObj = FindReplica(repId);

                PackedSize chunkSize;
                if (!rb.Read(chunkSize))
                {
                    return;
                }

                ReadBuffer replicaPayload = rb.ReadInnerBuffer(chunkSize);
                if (replicaPayload.IsValid())
                {
                    mc.m_iBuf = &replicaPayload;
                }
                else
                {
                    return;
                }

                if (!pObj)
                {
                    // we don't know about this replica, maybe it has already been deleted
                    if (m_tombstones.find(repId) != m_tombstones.end())
                    {
                        AZ_TracePrintf("GridMate", "Received updates for tombstoned replica(id=0x%x).\n", repId);
                    }
                    else
                    {
                        AZ_TracePrintf("GridMate", "Received updates for unknown replica(id=0x%x). Maybe it has already been deleted?\n", repId);
                    }

                    break;
                }
                if (pObj->m_upstreamHop)
                {
                    EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveReplicaBegin, pObj.get(), cmdBufferBegin, rb.GetCurrent() - cmdBufferBegin);
                    mc.m_peer = pFrom;
                    pObj->Unmarshal(mc);
                    OnReplicaUnmarshaled(pObj);
                    EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveReplicaEnd, pObj.get());
                }
                else
                {
                    //AZ_TracePrintf("GridMate", "Skipping unmarshal for 0x%x because we can't identify the correct peer.\n", pObj->GetRepId());      
                }

                break;     // TODO: should we process incoming data for orphaned replicas?
            }
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::Unmarshal()
    {
        AZ_PROFILE_FUNCTION(GridMate);

        if (!IsInitialized())
        {
            return;
        }

        unsigned int lagAmount = m_sessionInfo ? m_sessionInfo->m_localLagAmt.Get() : 0;
        m_currentFrameTime.m_realTime = m_cfg.m_carrier->GetTime();
        m_currentFrameTime.m_localTime = m_currentFrameTime.m_realTime < lagAmount ? m_currentFrameTime.m_realTime : m_currentFrameTime.m_realTime - lagAmount;

        AZ_Assert(m_cfg.m_carrier, "No available net layer!");
        m_flags |= Rm_Processing;

        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);
            for (ReplicaPeerList::const_iterator iPeer = m_remotePeers.begin(); iPeer != m_remotePeers.end(); ++iPeer)
            {
                ReplicaPeer* peer = *iPeer;
                AZ_Assert(peer, "Invalid peer pointer!");

                ConnectionID conn = peer->GetConnectionId();
                if (conn == InvalidConnectionID)
                {
                    continue;
                }

                // we are only allowed to receive anything else if this peer has been approved by the host
                for (bool keepReceiving = !peer->GetId() || AcceptPeer(peer); keepReceiving; keepReceiving = AcceptPeer(peer))
                {
                    Carrier::ReceiveResult result = m_cfg.m_carrier->Receive(m_receiveBuffer.data(), static_cast<unsigned int>(m_receiveBuffer.size()), conn, m_cfg.m_commChannel);
                    if (result.m_state == Carrier::ReceiveResult::UNSUFFICIENT_BUFFER_SIZE)
                    {
                        m_receiveBuffer.resize(result.m_numBytes);
                        result = m_cfg.m_carrier->Receive(m_receiveBuffer.data(), static_cast<unsigned int>(m_receiveBuffer.size()), conn, m_cfg.m_commChannel);

                        AZ_Assert(result.m_state != Carrier::ReceiveResult::UNSUFFICIENT_BUFFER_SIZE, "Carrier::ReceiveResult::UNSUFFICIENT_BUFFER_SIZE detected!, result.m_numBytes = %u, buffer size = %u", result.m_numBytes, static_cast<unsigned int>(m_receiveBuffer.size()));
                    }

                    if (result.m_state == Carrier::ReceiveResult::NO_MESSAGE_TO_RECEIVE)
                    {
                        break;
                    }

                    ReadBuffer rb(GetGridMate()->GetDefaultEndianType(), m_receiveBuffer.data(), result.m_numBytes);
                    EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceive, peer->GetId(), rb.Get(), rb.Size().GetSizeInBytesRoundUp());
                    _Unmarshal(rb, peer);
                    AZ_Assert(rb.IsEmptyIgnoreTrailingBits(), "We did not process the whole message!");
                }

                if (ReplicaDebug::g_TrackDbgHeartbeat)
                {
                    ++peer->m_lastReceiveTicks;
                    if (peer->m_lastReceiveTicks > ReplicaDebug::g_MaxTicksPerHeartbeat)
                    {
                        m_cfg.m_carrier->DebugStatusReport(conn, m_cfg.m_commChannel);
                        AZ_Assert(0, "No updates for %d ticks!", ReplicaDebug::g_MaxTicksPerHeartbeat);
                        peer->m_lastReceiveTicks = 0;
                    }
                }
            }
        }

        m_flags &= ~Rm_Processing;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::RegisterReplica(const ReplicaPtr& pReplica, bool isPrimary, ReplicaContext& rc)
    {
        AZ_Assert((pReplica->m_flags & ~Replica::Rep_Traits) == 0, "This replica is not clean, flags=0x%x, rpcs=%d!", pReplica->m_flags);
        AZ_Assert(pReplica->GetRepId() != InvalidReplicaId, "You should set the replica ID before you register it!");
        pReplica->SetPrimary(isPrimary);
        pReplica->SetNew();
        auto it = m_replicas.insert(AZStd::make_pair(pReplica->GetRepId(), pReplica)); // register with lookup table
        (void)it;
        AZ_Assert(it.second, "Inserting duplicate id into map");
        pReplica->Activate(rc);
        UpdateReplicaTargets(pReplica);
        OnReplicaChanged(pReplica);
    }
    //-----------------------------------------------------------------------------
    ReplicaId ReplicaManager::AddPrimary(const ReplicaPtr& pPrimary)
    {
        AZ_Assert(IsReady(), "ReplicaManager is not ready!");
        AZ_Assert(pPrimary, "Attempting to register NULL replica!");
        ReplicaId newId = m_localIdBlocks.Alloc();
        ReplicaContext rc(this, GetTime(), &m_self);
        pPrimary->m_createTime = rc.m_realTime;
        pPrimary->SetRepId(newId);
        m_self.Add(pPrimary.get());
        AZStd::static_pointer_cast<ReplicaStatus>(pPrimary->m_replicaStatus)->m_ownerSeq.Set(1);
        pPrimary->InitReplica(this);
        RegisterReplica(pPrimary, true, rc);

        //AZ_TracePrintf("GridMate", "Peer 0x%x: Added replica primary 0x%x.\n",
        //               GetLocalPeerId(), pPrimary->GetRepId());

        return newId;
    }
    //-----------------------------------------------------------------------------
    bool ReplicaManager::Destroy(Replica* requestor)
    {
        AZ_Assert(requestor, "Invalid requestor!");
        auto iObj = m_self.m_objectsMap.find(requestor->GetRepId());
        if (iObj != m_self.m_objectsMap.end())
        {
            ReplicaPtr pReplica = iObj->second.m_replica;
            AZ_Assert(pReplica == requestor, "Replica pointer mismatch!");
            ReplicaContext rc(this, GetTime(), requestor->m_upstreamHop);
            RemoveReplicaFromDownstream(pReplica, rc);
            m_tombstones.insert(AZStd::make_pair(requestor->GetRepId(), GetTime().m_realTime));
            m_self.m_objectsTimeSort.erase(iObj->second);
            m_self.m_objectsMap.erase(pReplica->GetRepId());
            return true;
        }
        return false;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::GetReplicaContext(const Replica* requestor, ReplicaContext& rc)
    {
        AZ_Assert(requestor, "Invalid requestor!");
        ReplicaPtr pReplica = FindReplica(requestor->GetRepId());
        if (pReplica && pReplica == requestor)
        {
            rc = ReplicaContext(this, GetTime(), requestor->m_upstreamHop);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::SendGreetings(ReplicaPeer* peer)
    {
        peer->GetReliableOutBuffer().Write(Cmd_Greetings);
        peer->GetReliableOutBuffer().Write(GetLocalPeerId());
        peer->GetReliableOutBuffer().Write(IsSyncHost());
        if (IsSyncHost())
        {
            peer->GetReliableOutBuffer().Write(ReserveIdBlock(peer->m_peerId));
        }
        peer->SendBuffer(m_cfg.m_carrier, m_cfg.m_commChannel, GetTimeForNetworkTimestamp());
    }
    //-----------------------------------------------------------------------------
    IGridMate* ReplicaManager::GetGridMate() const
    {
        AZ_Assert(m_cfg.m_carrier, "ReplicaManager has an invalid carrier!");
        return m_cfg.m_carrier->GetGridMate();
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::EnqueueUpdateTask(ReplicaPtr replica)
    {
        if (!replica->HasUpdateTask())
        {
            m_updateTasks.Add<ReplicaUpdateTask>(replica);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::UpdateReplicaTargets(ReplicaPtr replica)
    {
        if (ShouldBroadcastReplica(replica.get()))
        {
            replica->m_targets.clear();
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers); 
                for (auto target : m_remotePeers)
                {
                    if (target->IsOrphan() || !(target->m_flags & PeerFlags::Peer_Accepted))
                    {
                        continue;
                    }

                    ReplicaPeer* source = replica->IsPrimary() ? &m_self : replica->m_upstreamHop;

                    if (replica->IsPrimary()
                        || (IsSyncHost() && source->GetId() != target->GetId() && !(source->GetMode() == Mode_Peer && target->GetMode() == Mode_Peer)))
                    {
                        ReplicaTarget::AddReplicaTarget(target, replica.get());
                    }
                }
            }
        }
        else
        {
            // Replica might've changed owner -> we need to update its targets accordingly
            ReplicaPeer* source = replica->IsPrimary() ? &m_self : replica->m_upstreamHop;

            for (auto it = replica->m_targets.begin(); it != replica->m_targets.end(); )
            {
                auto& target = *(it++);

                if (replica->IsProxy()
                    && (!IsSyncHost()  
                        || source->GetId() == target.GetPeer()->GetId()  // target points to the owner (no need to send replica to its owner)
                        || (source->GetMode() == Mode_Peer && target.GetPeer()->GetMode() == Mode_Peer))) // Clients should never forward proxies
                {
                    target.Destroy();
                }
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnPeerAccepted(ReplicaPeer* peer)
    {
        AZ_Assert(peer, "OnPeerAccepted: Invalid peer");

        for (auto& pObj : m_self.m_objectsTimeSort)
        {
            OnReplicaChanged(pObj.m_replica);
            if (ShouldBroadcastReplica(pObj.m_replica.get()))
            {
                ReplicaTarget::AddReplicaTarget(peer, pObj.m_replica.get());
            }
        }

        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);
            for (auto source : m_remotePeers)
            {
                if (!IsSyncHost() ||
                    source->GetId() == peer->GetId()
                    || (source->GetMode() == Mode_Peer && peer->GetMode() == Mode_Peer))
                {
                    continue;
                }

                for (auto& pObj : source->m_objectsTimeSort)
                {
                    OnReplicaChanged(pObj.m_replica);
                    if (ShouldBroadcastReplica(pObj.m_replica.get()))
                    {
                        ReplicaTarget::AddReplicaTarget(peer, pObj.m_replica.get());
                    }
                }
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnPeerReadyToRemove(ReplicaPeer* peer)
    {
        AZ_Assert(peer, "OnPeerReadyToRemove: Invalid peer");

        peer->m_objectsTimeSort.clear();
        peer->m_objectsMap.clear();
        peer->m_targets.clear();

        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);

            for (auto i = m_remotePeers.begin(); i != m_remotePeers.end(); ++i)
            {
                if (*i == peer)
                {
                    m_remotePeers.erase(i);
                    break;
                }
            }
        }

        m_peerUpdateTasks.Add<ReplicaDestroyPeerTask>(peer);
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnRPCQueued(ReplicaPtr replica)
    {
        OnReplicaChanged(replica);
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnReplicaChanged(ReplicaPtr replica)
    {
        AZ_Assert(replica, "OnReplicaChanged: Invalid replica");
        if (!replica->IsDirty() && replica->IsActive())
        {
            m_dirtyReplicas.push_back(*replica);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnReplicaUnmarshaled(ReplicaPtr replica)
    {
        if (IsSyncHost())
        {
            OnReplicaChanged(replica);
        }

        EnqueueUpdateTask(replica);
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::RemoveFromDirtyList(Replica& replica)
    {
        m_dirtyReplicas.erase(replica);
        replica.m_dirtyHook.m_next = replica.m_dirtyHook.m_prev = nullptr;
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::CancelTasks(ReplicaPtr replica)
    {
        for (auto& i : replica->m_marshalingTasks)
        {
            i->Cancel();
        }

        for (auto& i : replica->m_updateTasks)
        {
            i->Cancel();
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnDestroyProxy(ReplicaId repId)
    {
        ReplicaPtr pObj = FindReplica(repId);
        if (pObj)
        {
            if (pObj->m_upstreamHop->m_objectsMap.find(pObj->GetRepId()) == pObj->m_upstreamHop->m_objectsMap.end())
            {
                AZ_TracePrintf("GridMate", "Received non-authoritative request to destroy replica!");
                return;
            }

            for (auto& i : pObj->m_updateTasks)
            {
                i->Cancel();
            }
            m_updateTasks.Add<ReplicaUpdateDestroyedProxyTask>(pObj);
            pObj->m_upstreamHop->Remove(pObj.get());
        }
        m_tombstones.insert(AZStd::make_pair(repId, GetTime().m_realTime));
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnPendingReportsReceived(PeerId peerId) // this is only called for host, as others are not waiting for peer reports
    {
        // Got all pending reports for given peer
        // Should either remove it or start migration
        AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutexRemotePeers);
        
        for (auto& p : m_remotePeers)
        {
            if (p->GetId() == peerId)
            {
                if (p->m_objectsTimeSort.empty())
                {
                    OnPeerReadyToRemove(p);
                }
                else
                {
                    OnMigratePeer(p);
                }
                break;
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnMigratePeer(ReplicaPeer* peer)
    {
        if (peer->m_objectsTimeSort.empty()) // peer has no replicas -> can remove immediately and bailout
        {
            OnPeerReadyToRemove(peer);
            return;
        }

        for (auto& r : peer->m_objectsTimeSort)
        {
            if (!r.m_replica->HasUpdateTask())
            {
                m_updateTasks.Add<ReplicaUpdateTask>(r.m_replica);
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::ChangeReplicaOwnership(ReplicaPtr replica, const ReplicaContext& rc, bool isPrimary)
    {
        bool wasPrimary = replica->IsPrimary();
        if (wasPrimary != isPrimary) // wasPrimary == isPrimary can happen when host's replica is moved to client, and client confirms with Cmd_NewOwner
        {
            replica->SetPrimary(isPrimary);
            replica->OnChangeOwnership(rc);
            EBUS_EVENT(Debug::ReplicaDrillerBus, OnReplicaChangeOwnership, replica.get(), wasPrimary);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::OnReplicaPriorityUpdated(Replica* replica)
    {
        for (auto task : replica->m_marshalingTasks)
        {
            m_marshalingTasks.UpdatePriority(task);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaManager::SetSecurityOptions(const ReplicationSecurityOptions& options)
    {
        m_securityOptions = options;
    }
    //-----------------------------------------------------------------------------
    ReplicationSecurityOptions ReplicaManager::GetSecurityOptions() const
    {
        return m_securityOptions;
    }
    //-----------------------------------------------------------------------------
    bool ReplicaManager::ShouldBroadcastReplica(Replica* replica) const
    {
        return m_autoBroadcast || replica->IsBroadcast();
    }
    void ReplicaManager::UpdateConnectionRate(AZ::u32 bytesPerSecond, ConnectionID id)
    {
        if (m_connByCongestionState.empty())
        {
            return;     //No connections to update
        }
        bool updateRate = false;
        AZ::u32 minRateBytesPerSecond = m_connByCongestionState.front().m_rate;
        //const AZ::u32 old = minRateBytesPerSecond;  //For debugging

        auto connIt = AZStd::find(m_connByCongestionState.begin(), m_connByCongestionState.end(), id);

        if ( connIt == m_connByCongestionState.end())
        {
            return; //Already disconnected
        }

        //If rate changed
        if (connIt->m_rate != bytesPerSecond)
        {
            //Note this could invalidate the heap ordering, but since we only need a weak guarantee that 
            //the top is the lowest we can delay re-running make-heap until 1) a different connection 
            //takes the top/min spot or 2) a connection is removed/added (very rare) 
            connIt->m_rate = bytesPerSecond;

            //If new min or old min increased, rebuild the heap and send an update
            if (bytesPerSecond < minRateBytesPerSecond
                || (id == m_connByCongestionState.front().m_connection && bytesPerSecond > minRateBytesPerSecond))
            {
                updateRate = true;
                minRateBytesPerSecond = bytesPerSecond;
                AZStd::make_heap(m_connByCongestionState.begin(), m_connByCongestionState.end());
            }
        }

        if (updateRate)
        {
            //AZ_DbgIf(minRateBytesPerSecond < old)
            //{
            //    AZ_TracePrintf("GridMate", "%p updateConnection %p %u -> %u \n",
            //        this, id, old, minRateBytesPerSecond);
            //}

            SetSendLimit(minRateBytesPerSecond);     //Respond to congestion control update; also disables bursting
        }
    }
    //-----------------------------------------------------------------------------
} // namespace GridMate
