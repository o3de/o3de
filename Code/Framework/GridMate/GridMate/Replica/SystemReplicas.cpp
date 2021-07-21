/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Replica/MigrationSequence.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/SystemReplicas.h>

namespace GridMate
{
    namespace ReplicaInternal
    {
        /*
        * Custom descriptor to override allocation because
        * session info is an integral part of replica manager
        */
        class SessionInfoDesc
            : public ReplicaChunkDescriptor
        {
        public:
            SessionInfoDesc()
                : ReplicaChunkDescriptor(SessionInfo::GetChunkName(), sizeof(SessionInfo))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext& mc) override
            {
                AZ_Assert(!mc.m_rm->m_sessionInfo->GetReplica() ||
                    !mc.m_rm->m_sessionInfo->GetReplica()->IsActive(), "We should not have more than one sessionInfo replica!!!");
                return mc.m_rm->m_sessionInfo.get();
            }

            void DiscardCtorStream(UnmarshalContext&) override {}
            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override { delete chunkInstance; }
            void MarshalCtorData(ReplicaChunkBase*, WriteBuffer&) override {}
        };

        //-----------------------------------------------------------------------------
        // SessionInfo
        //-----------------------------------------------------------------------------
        void SessionInfo::RegisterType()
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<SessionInfo, SessionInfoDesc>();
        }
        //-----------------------------------------------------------------------------
        SessionInfo::SessionInfo(ReplicaManager* pMgr)
            : AnnounceNewHostRpc("AnnounceNewHostRpc")
            , DiscardOrphansRpc("DiscardOrphansRpc")
            , RequestPeerMigration("RequestPeerMigration")
            , ReportPeerState("ReportPeerState")
            , m_acceptedPeers("AcceptedPeers")
            , m_localLagAmt("LocalLag")
            , m_nextAvailableIdBlock("NextAvailableIdBlock")
            , m_pMgr(pMgr)
            , m_pHostPeer(nullptr)
            , m_formerHost(0)
        {
            m_localLagAmt.Set(0);
            m_acceptedPeers.SetMaxIdleTime(0.f);
            SetPriority(k_replicaPriorityRealTime);
        }
        //-----------------------------------------------------------------------------
        bool SessionInfo::IsReplicaMigratable()
        {
            return true;
        }
        //-----------------------------------------------------------------------------
        void SessionInfo::OnReplicaActivate(const ReplicaContext& rc)
        {
            rc.m_rm->m_sessionInfo = this;
            m_pHostPeer = rc.m_peer;
            m_pHostPeer->MakeSyncHost(true);

            // on activation of this replica, create our PeerInfo replica
            Replica* peerReplica = Replica::CreateReplica("PeerInfo");
            CreateAndAttachReplicaChunk<PeerReplica>(peerReplica);
            rc.m_rm->AddPrimary(peerReplica);
        }
        //-----------------------------------------------------------------------------
        void SessionInfo::OnReplicaDeactivate(const ReplicaContext& rc)
        {
            (void)rc;
        }
        //-----------------------------------------------------------------------------
        void SessionInfo::OnReplicaChangeOwnership(const ReplicaContext& rc)
        {
            if (m_pHostPeer)
            {
                m_pHostPeer->MakeSyncHost(false);
                m_formerHost = m_pHostPeer->GetId();
            }
            m_pHostPeer = rc.m_peer;
            m_pHostPeer->MakeSyncHost(true);
            //AZ_TracePrintf("GridMate", "SystemReplica migrated from peerId=0x%x to peerId=0x%x.\n", m_formerHost, m_pHostPeer->GetId());
        }
        //-----------------------------------------------------------------------------
        bool SessionInfo::AcceptChangeOwnership(PeerId requestor, const ReplicaContext& rc)
        {
            // nobody can request ownership transfer of the system replica
            // ReplicaMgr does this manually during host migration.
            (void)requestor;
            (void)rc;
            return false;
        }
        //-----------------------------------------------------------------------------
        bool SessionInfo::AnnounceNewHost(const RpcContext& rc)
        {
            (void)rc;
            EBUS_EVENT_ID(m_pMgr->GetGridMate(), ReplicaMgrCallbackBus, OnNewHost, m_pMgr->IsSyncHost(), m_pMgr);
            return true;
        }
        //-----------------------------------------------------------------------------
        bool SessionInfo::DiscardOrphans(PeerId orphanId, const RpcContext& rc)
        {
            (void)rc;
            m_pMgr->DiscardOrphans(orphanId);
            return true;
        }
        //-----------------------------------------------------------------------------
        bool SessionInfo::OnPeerMigrationRequest(PeerId peerId, const RpcContext& rc)
        {
            (void)rc;
            if (m_pMgr->IsSyncHost())
            {
                AZ_Assert(IsPrimary(), "The host should always own sessionInfo!!!");
                AZ_Assert(m_pendingPeerReports.find(peerId) == m_pendingPeerReports.end(), "We are already waiting for reports for peer 0x%8x!", peerId);

                vector<PeerId> peers;
                for (AZStd::size_t i = 0; i < m_acceptedPeers.Get().size(); ++i)
                {
                    // we need to wait for replies from all currently accepted peers except for ourselves.
                    if (m_acceptedPeers.Get()[i] != m_pMgr->GetLocalPeerId())
                    {
                        peers.push_back(m_acceptedPeers.Get()[i]);
                    }
                }

                if (!peers.empty())
                {
                    auto ret = m_pendingPeerReports.insert_key(peerId);
                    ret.first->second = AZStd::move(peers);
                }
            }
            else
            {
                AZ_Assert(IsProxy(), "Only the host should own sessionInfo!!!");
                ReportPeerState(peerId, m_pMgr->GetLocalPeerId());
            }
            return true;
        }
        //-----------------------------------------------------------------------------
        bool SessionInfo::OnReportPeerState(PeerId orphan, PeerId from, const RpcContext& rc)
        {
            (void)rc;
            auto it = m_pendingPeerReports.find(orphan);
            if (it != m_pendingPeerReports.end())
            {
                for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
                {
                    if (*it2 == from)
                    {
                        it->second.erase(it2);

                        if (it->second.empty())
                        {
                            m_pendingPeerReports.erase(it);
                            GetReplicaManager()->OnPendingReportsReceived(orphan);
                        }
                        break;
                    }
                }
            }
            return false;
        }
        //-----------------------------------------------------------------------------
        bool SessionInfo::IsInAcceptList(PeerId peerId) const
        {
            return AZStd::find(m_acceptedPeers.Get().begin(), m_acceptedPeers.Get().end(), peerId) != m_acceptedPeers.Get().end();
        }
        //-----------------------------------------------------------------------------
        bool SessionInfo::HasPendingReports(PeerId orphan) const
        {
            auto it = m_pendingPeerReports.find(orphan);
            return it != m_pendingPeerReports.end() && !it->second.empty();
        }
        //-----------------------------------------------------------------------------

        //-----------------------------------------------------------------------------
        // PeerReplica
        //-----------------------------------------------------------------------------
        void PeerReplica::RegisterType()
        {
            ReplicaChunkDescriptorTable::Get().RegisterChunkType<PeerReplica>();
        }
        //-----------------------------------------------------------------------------
        void PeerReplica::OnReplicaActivate(const ReplicaContext& rc)
        {
            if (IsPrimary())
            {
                m_peerId.Set(rc.m_rm->GetLocalPeerId());
            }
            rc.m_rm->OnPeerReplicaActivated(this);
        }
        //-----------------------------------------------------------------------------
        void PeerReplica::OnReplicaDeactivate(const ReplicaContext& rc)
        {
            (void)rc;
            rc.m_rm->OnPeerReplicaDeactivated(this);
        }
        //-----------------------------------------------------------------------------
        bool PeerReplica::OnAckUpstreamSuspendedFn(ReplicaId replicaId, PeerId peerId, AZ::u32 requestTime, const RpcContext& rpcContext)
        {
            (void)rpcContext;
            //AZ_TracePrintf("GridMate", "Received upstream suspend ack response requested at %u for 0x%x from 0x%x.\n", requestTime, replicaId, peerId);
            GetReplicaManager()->OnAckUpstreamSuspended(replicaId, peerId, requestTime);
            return false;
        }
        //-----------------------------------------------------------------------------
        bool PeerReplica::OnAckDownstreamFn(ReplicaId replicaId, PeerId peerId, AZ::u32 requestTime, const RpcContext& rpcContext)
        {
            (void)rpcContext;
            //AZ_TracePrintf("GridMate", "Received downstream ack response requested at %u for 0x%x from 0x%x.\n", requestTime, replicaId, peerId);
            GetReplicaManager()->OnAckDownstream(replicaId, peerId, requestTime);
            return false;
        }
        //-----------------------------------------------------------------------------
        bool PeerReplica::OnReplicaMigratedFn(ReplicaId replicaId, PeerId newOwnerId, const RpcContext& rpcContext)
        {
            (void)rpcContext;
            (void)replicaId;
            ReplicaManager* manager = GetReplicaManager();

            if (IsProxy())
            {
                ReplicaContext rc = GetReplica()->GetMyContext();
                ReplicaPtr replica = manager->FindReplica(replicaId);
                if (replica)
                {
                    manager->MigrateReplica(replica, newOwnerId);
                    if (newOwnerId == manager->GetLocalPeerId())
                    {
                        // replica is migrating to our local peer
                        rc.m_peer = &manager->m_self;
                        manager->OnReplicaMigrated(replica, true, rc);
                    }
                }
            }
            return true;
        }
    }   // namespace ReplicaInternal
}   // namespace GridMate
