/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_SYSTEM_REPLICAS_H
#define GM_SYSTEM_REPLICAS_H

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Containers/unordered_map.h>
#include <GridMate/Serialize/ContainerMarshal.h>
#include <GridMate/Serialize/UtilityMarshal.h>

namespace GridMate
{
    class ReplicaManager;

    namespace ReplicaInternal
    {
        struct MigrationNotificationTraits : public RpcAuthoritativeTraits
        {
        };

        struct MigrationResponseTraits : public RpcDefaultTraits
        {
            static const bool s_isReliable = true;
            static const bool s_isPostAttached = true;
            static const bool s_allowNonAuthoritativeRequests = true;
        };

        //-----------------------------------------------------------------------------
        // SessionInfo
        // Global replication settings that every peer needs to know about
        // Always owned by the replication host
        //-----------------------------------------------------------------------------
        class SessionInfo
            : public ReplicaChunk
        {
        public:
            typedef AZStd::intrusive_ptr<SessionInfo> Ptr;

            GM_CLASS_ALLOCATOR(SessionInfo);
            static const char* GetChunkName() { return "GridMateReplicaSessionInfo"; }
            static void RegisterType();

            SessionInfo(ReplicaManager* pMgr);

            bool IsReplicaMigratable() override;
            void OnReplicaActivate(const ReplicaContext& rc) override;
            void OnReplicaDeactivate(const ReplicaContext& rc) override;
            void OnReplicaChangeOwnership(const ReplicaContext& rc) override;
            bool AcceptChangeOwnership(PeerId requestor, const ReplicaContext& rc) override;

            bool IsBroadcast() override { return true; }

            bool AnnounceNewHost(const RpcContext& rc);
            bool DiscardOrphans(PeerId orphanId, const RpcContext& rc);
            bool OnPeerMigrationRequest(PeerId peerId, const RpcContext& rc);
            bool OnReportPeerState(PeerId orphan, PeerId from, const RpcContext& rc);

            bool IsInAcceptList(PeerId peerId) const;
            bool HasPendingReports(PeerId orphan) const;

            Rpc<>::BindInterface<SessionInfo, & SessionInfo::AnnounceNewHost, MigrationNotificationTraits> AnnounceNewHostRpc;
            Rpc<RpcArg<PeerId> >::BindInterface<SessionInfo, & SessionInfo::DiscardOrphans, MigrationNotificationTraits> DiscardOrphansRpc;
            Rpc<RpcArg<PeerId> >::BindInterface<SessionInfo, & SessionInfo::OnPeerMigrationRequest, MigrationNotificationTraits> RequestPeerMigration;
            Rpc<RpcArg<PeerId>, RpcArg<PeerId> >::BindInterface<SessionInfo, & SessionInfo::OnReportPeerState, MigrationResponseTraits> ReportPeerState;

            DataSet<vector<PeerId>, ContainerMarshaler<vector<PeerId> > > m_acceptedPeers;
            DataSet<AZ::u32> m_localLagAmt;
            DataSet<RepIdSeed> m_nextAvailableIdBlock;

            ReplicaManager* m_pMgr;
            ReplicaPeer* m_pHostPeer;
            PeerId m_formerHost;
            unordered_map<PeerId, vector<PeerId> > m_pendingPeerReports;
        };
        //-----------------------------------------------------------------------------

        //-----------------------------------------------------------------------------
        // PeerReplica
        // Info about the peer that everyone else needs to know
        // Owned by each peer
        //-----------------------------------------------------------------------------
        class PeerReplica
            : public ReplicaChunk
        {
        public:
            typedef AZStd::intrusive_ptr<PeerReplica> Ptr;

            GM_CLASS_ALLOCATOR(PeerReplica);
            static const char* GetChunkName() { return "GridMatePeerReplica"; }
            static void RegisterType();

            PeerReplica()
                : OnAckUpstreamSuspended("OnAckUpstreamSuspended")
                , OnAckDownstream("OnAckDownstream")
                , OnReplicaMigrated("OnReplicaMigrated")
                , m_peerId("PeerId")
            {
                SetPriority(k_replicaPriorityRealTime);
            }

            void OnReplicaActivate(const ReplicaContext& rc) override;
            void OnReplicaDeactivate(const ReplicaContext& rc) override;

            bool IsReplicaMigratable() override { return false; }
            bool IsBroadcast() override { return true; }

            bool OnAckUpstreamSuspendedFn(ReplicaId replicaId, PeerId peerId, AZ::u32 requestTime, const RpcContext& rpcContext);
            bool OnAckDownstreamFn(ReplicaId replicaId, PeerId peerId, AZ::u32 requestTime, const RpcContext& rpcContext);
            bool OnReplicaMigratedFn(ReplicaId replicaId, PeerId newOwnerId, const RpcContext& rpcContext);

            Rpc<RpcArg<ReplicaId>, RpcArg<PeerId>, RpcArg<AZ::u32> >::BindInterface<PeerReplica, & PeerReplica::OnAckUpstreamSuspendedFn, MigrationResponseTraits> OnAckUpstreamSuspended;
            Rpc<RpcArg<ReplicaId>, RpcArg<PeerId>, RpcArg<AZ::u32> >::BindInterface<PeerReplica, & PeerReplica::OnAckDownstreamFn, MigrationResponseTraits> OnAckDownstream;
            Rpc<RpcArg<ReplicaId>, RpcArg<PeerId> >::BindInterface<PeerReplica, & PeerReplica::OnReplicaMigratedFn, MigrationNotificationTraits> OnReplicaMigrated;

            DataSet<PeerId> m_peerId;
        };
    } // namespace ReplicaInternal
} // namespace GridMate

#endif // GM_SYSTEM_REPLICAS_H
