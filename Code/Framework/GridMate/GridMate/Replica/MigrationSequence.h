/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_MIGRATION_SEQUENCE_H
#define GM_REPLICA_MIGRATION_SEQUENCE_H

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Containers/unordered_set.h>
#include <AzCore/State/HSM.h>

namespace GridMate
{
    namespace ReplicaInternal
    {
        class MigrationSequence
            : public ReplicaMgrCallbackBus::Handler
        {
        public:
            enum MigrationState
            {
                MST_TOP,
                MST_MIGRATING,
                MST_FLUSH_UPSTREAM,
                MST_FLUSH_DOWNSTREAM,
                MST_CHANGE_ROUTING_FOR_MIGRATION,
                MST_HANDOFF_REPLICA,
                MST_ROLLBACK,
                MST_ABORT,
                MST_CHANGE_ROUTING_ONLY,
                MST_IDLE,
            };

            enum MigrationEvent
            {
                ME_UPDATE,
                ME_REPLICA_REMOVED,
                ME_PEER_REMOVED,
                ME_PEER_ACK,
                ME_MODIFY_NEW_OWNER,
            };

            GM_CLASS_ALLOCATOR(MigrationSequence);

            MigrationSequence(Replica* replica, PeerId newOwnerId);

            void Update();
            bool IsDone() const;
            void ModifyNewOwner(PeerId newOwnerId);

            bool OnStateMigrating(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateFlushUpstream(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateFlushDownstream(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateChangeRouting(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateHandoffReplica(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateAbort(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateRollback(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool OnStateChangeRoutingOnly(AZ::HSM& sm, const AZ::HSM::Event& event);
            bool DefaultHandler(AZ::HSM& sm, const AZ::HSM::Event& event);

            ///////////////////////////////////////////////////////////////////
            // ReplicaMgrCallbackBus
            void OnDeactivateReplica(ReplicaId replicaId, ReplicaManager* pMgr) override;
            void OnPeerRemoved(PeerId peerId, ReplicaManager* pMgr) override;
            ///////////////////////////////////////////////////////////////////

            void OnReceivedAckUpstreamSuspended(PeerId from, AZ::u32 requestTime);
            void OnReceivedAckDownstream(PeerId from, AZ::u32 requestTime);

            bool UpdateReplicaRouting();

            Replica* m_replica;
            PeerId m_newOwnerId;
            ReplicaManager* m_replicaMgr;
            unsigned int m_timestamp;

            AZ::HSM m_sm;

            typedef unordered_set<PeerId> PeerAckTracker;
            PeerAckTracker m_pendingAcks;
        };
    }   // namespace ReplicaInternal
}   // namespace GridMate

#endif // GM_REPLICA_MIGRATION_SEQUENCE_H
