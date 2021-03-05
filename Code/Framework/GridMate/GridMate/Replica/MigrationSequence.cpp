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
#include <GridMate/Replica/MigrationSequence.h>
#include <GridMate/Replica/ReplicaStatus.h>

namespace GridMate
{
    namespace ReplicaInternal
    {
        //-----------------------------------------------------------------------------
        //-----------------------------------------------------------------------------
        MigrationSequence::MigrationSequence(Replica* replica, PeerId newOwnerId)
            : m_replica(replica)
            , m_newOwnerId(newOwnerId)
        {
            ReplicaMgrCallbackBus::Handler::BusConnect(replica->GetReplicaManager()->GetGridMate());

            m_replicaMgr = replica->GetReplicaManager();

            if (replica->IsMaster() && newOwnerId != m_replicaMgr->GetLocalPeerId())
            {
                m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_TOP), AZ::HSM::StateHandler(this, &MigrationSequence::DefaultHandler), AZ::HSM::InvalidStateId, MST_MIGRATING);
            }
            else
            {
                m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_TOP), AZ::HSM::StateHandler(this, &MigrationSequence::DefaultHandler), AZ::HSM::InvalidStateId, MST_CHANGE_ROUTING_ONLY);
            }
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_MIGRATING), AZ::HSM::StateHandler(this, &MigrationSequence::OnStateMigrating), MST_TOP, MST_FLUSH_UPSTREAM);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_FLUSH_UPSTREAM), AZ::HSM::StateHandler(this, &MigrationSequence::OnStateFlushUpstream), MST_MIGRATING);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_FLUSH_DOWNSTREAM), AZ::HSM::StateHandler(this, &MigrationSequence::OnStateFlushDownstream), MST_MIGRATING);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_CHANGE_ROUTING_FOR_MIGRATION), AZ::HSM::StateHandler(this, &MigrationSequence::OnStateChangeRouting), MST_MIGRATING);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_HANDOFF_REPLICA), AZ::HSM::StateHandler(this, &MigrationSequence::OnStateHandoffReplica), MST_MIGRATING);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_ROLLBACK), AZ::HSM::StateHandler(this, &MigrationSequence::OnStateRollback), MST_TOP);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_ABORT), AZ::HSM::StateHandler(this, &MigrationSequence::OnStateAbort), MST_TOP);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_CHANGE_ROUTING_ONLY), AZ::HSM::StateHandler(this, &MigrationSequence::OnStateChangeRoutingOnly), MST_TOP);
            m_sm.SetStateHandler(AZ_HSM_STATE_NAME(MST_IDLE), AZ::HSM::StateHandler(this, &MigrationSequence::DefaultHandler), MST_TOP);

            m_sm.Start();
        }
        //-----------------------------------------------------------------------------
        void MigrationSequence::Update()
        {
            m_sm.Dispatch(ME_UPDATE);
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::IsDone() const
        {
            return m_sm.IsInState(MST_IDLE);
        }
        //-----------------------------------------------------------------------------
        void MigrationSequence::ModifyNewOwner(PeerId newOwnerId)
        {
            AZ::HSM::Event ev;
            ev.id = ME_MODIFY_NEW_OWNER;
            ev.userData = &newOwnerId;
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::OnStateMigrating(AZ::HSM& sm, const AZ::HSM::Event& event)
        {
            switch (event.id)
            {
            case AZ::HSM::EnterEventId:
                //AZ_TracePrintf("GridMate", "Replica 0x%x entering migration state MST_MIGRATING.\n", m_replica->GetRepId());
                m_pendingAcks.clear();
                return true;
            case AZ::HSM::ExitEventId:
                return true;
            case ME_PEER_REMOVED:
                if (*static_cast<PeerId*>(event.userData) == m_newOwnerId)
                {
                    sm.Transition(MST_ABORT);
                    return true;
                }
            // else fall through
            case ME_PEER_ACK:
                m_pendingAcks.erase(*static_cast<PeerId*>(event.userData));
                return true;
            case ME_MODIFY_NEW_OWNER:
                m_newOwnerId = *static_cast<PeerId*>(event.userData);
                return true;
            default:
                break;
            }
            return false;
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::OnStateFlushUpstream(AZ::HSM& sm, const AZ::HSM::Event& event)
        {
            switch (event.id)
            {
            case AZ::HSM::EnterEventId:
                //AZ_TracePrintf("GridMate", "Replica 0x%x entering migration state MST_FLUSH_UPSTREAM.\n", m_replica->GetRepId());
                for (auto& peerReplica : m_replicaMgr->m_peerReplicas)
                {
                    if (peerReplica.second->m_peerId.Get() != m_replicaMgr->GetLocalPeerId())
                    {
                        m_pendingAcks.insert(peerReplica.second->m_peerId.Get());
                    }
                }
                AZStd::static_pointer_cast<ReplicaStatus>(m_replica->m_replicaStatus)->SetUpstreamSuspended(true);
                m_timestamp = m_replicaMgr->GetTime().m_realTime;
                AZStd::static_pointer_cast<ReplicaStatus>(m_replica->m_replicaStatus)->MigrationSuspendUpstream(m_replicaMgr->GetLocalPeerId(), m_timestamp);
                return true;
            case ME_PEER_REMOVED:
                if (*static_cast<PeerId*>(event.userData) == m_newOwnerId)
                {
                    sm.Transition(MST_ABORT);
                    return true;
                }
            // else fall through
            case ME_PEER_ACK:
                m_pendingAcks.erase(*static_cast<PeerId*>(event.userData));
                if (m_pendingAcks.empty())
                {
                    sm.Transition(MST_FLUSH_DOWNSTREAM);
                }
                return true;
            default:
                break;
            }
            return false;
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::OnStateFlushDownstream(AZ::HSM& sm, const AZ::HSM::Event& event)
        {
            switch (event.id)
            {
            case AZ::HSM::EnterEventId:
                //AZ_TracePrintf("GridMate", "Replica 0x%x entering migration state MST_FLUSH_DOWNSTREAM.\n", m_replica->GetRepId());
                return true;
            case ME_UPDATE:
                for (auto& peerReplica : m_replicaMgr->m_peerReplicas)
                {
                    if (peerReplica.second->m_peerId.Get() != m_replicaMgr->GetLocalPeerId())
                    {
                        m_pendingAcks.insert(peerReplica.second->m_peerId.Get());
                    }
                }
                m_timestamp = m_replicaMgr->GetTime().m_realTime;
                AZStd::static_pointer_cast<ReplicaStatus>(m_replica->m_replicaStatus)->MigrationRequestDownstreamAck(m_replicaMgr->GetLocalPeerId(), m_timestamp);

                // Demote the replica so no more updates are made to it
                m_replicaMgr->ChangeReplicaOwnership(m_replica, m_replica->GetMyContext(), false);

                // Move the replica to its new routing peer (which effectively disables outbound replication)
                // This will be done in the tick to force a frame delay between demoting the replica
                // and actually moving it to allow one last outbound send.
                sm.Transition(MST_CHANGE_ROUTING_FOR_MIGRATION);
                return true;
            default:
                break;
            }
            return false;
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::OnStateChangeRouting(AZ::HSM& sm, const AZ::HSM::Event& event)
        {
            switch (event.id)
            {
            case AZ::HSM::EnterEventId:
                //AZ_TracePrintf("GridMate", "Replica 0x%x entering migration state MST_CHANGE_ROUTING_FOR_MIGRATION.\n", m_replica->GetRepId());
                return true;
            case ME_UPDATE:
            {
                if (m_replica->IsSuspendDownstream()) // wait until downstream suspention command is sent to everyone
                {
                    return true;
                }
                if (UpdateReplicaRouting())
                {
                    m_replicaMgr->UpdateReplicaTargets(m_replica);
                    sm.Transition(MST_HANDOFF_REPLICA);
                }
                else
                {
                    AZ_Warning("GridMate", false, "Replica Migration: Can't find new next hop for the replica! Aborting migration.");
                    sm.Transition(MST_ROLLBACK);
                }
                return true;
            }
            case ME_PEER_REMOVED:
                if (*static_cast<PeerId*>(event.userData) == m_newOwnerId)
                {
                    sm.Transition(MST_ROLLBACK);
                    return true;
                }
                return false;
            default:
                break;
            }
            return false;
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::OnStateHandoffReplica(AZ::HSM& sm, const AZ::HSM::Event& event)
        {
            switch (event.id)
            {
            case AZ::HSM::EnterEventId:
                //AZ_TracePrintf("GridMate", "Replica 0x%x entering migration state MST_HANDOFF_REPLICA.\n", m_replica->GetRepId());
                return true;
            case ME_PEER_REMOVED:
                if (*static_cast<PeerId*>(event.userData) == m_newOwnerId)
                {
                    sm.Transition(MST_ROLLBACK);
                    return true;
                }
            case ME_PEER_ACK:
                m_pendingAcks.erase(*static_cast<PeerId*>(event.userData));
                break;
            case ME_UPDATE:
                break;
            default:
                return false;
            }

            // If we received all the necessary acks, it's time to actually
            // handoff the replica and complete the migration.
            // This is done via an OOB message to all the peers
            if (m_pendingAcks.empty())
            {
                m_replicaMgr->AnnounceReplicaMigrated(m_replica->GetRepId(), m_newOwnerId);
                sm.Transition(MST_IDLE);
            }
            return true;
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::OnStateAbort(AZ::HSM& sm, const AZ::HSM::Event& event)
        {
            switch (event.id)
            {
            case AZ::HSM::EnterEventId:
                //AZ_TracePrintf("GridMate", "Replica 0x%x entering migration state MST_ABORT.\n", m_replica->GetRepId());
                return true;
            case ME_UPDATE:
                AZStd::static_pointer_cast<ReplicaStatus>(m_replica->m_replicaStatus)->SetUpstreamSuspended(false);
                sm.Transition(MST_IDLE);
                return true;
            case ME_PEER_REMOVED:
            case ME_PEER_ACK:
                return true;
            case ME_MODIFY_NEW_OWNER:
                m_newOwnerId = *static_cast<PeerId*>(event.userData);
                sm.Transition(MST_MIGRATING);
                return true;
            default:
                break;
            }
            return false;
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::OnStateRollback(AZ::HSM& sm, const AZ::HSM::Event& event)
        {
            switch (event.id)
            {
            case AZ::HSM::EnterEventId:
                //AZ_TracePrintf("GridMate", "Replica 0x%x entering migration state MST_ROLLBACK.\n", m_replica->GetRepId());
                return true;
            case ME_UPDATE:
                m_replicaMgr->ChangeReplicaOwnership(m_replica, m_replica->GetMyContext(), true);
                AZStd::static_pointer_cast<ReplicaStatus>(m_replica->m_replicaStatus)->SetUpstreamSuspended(false);
                if (m_replica->m_upstreamHop != &m_replicaMgr->m_self)
                {
                    m_replica->m_upstreamHop->Remove(m_replica);
                    m_replicaMgr->m_self.Add(m_replica);
                }
                sm.Transition(MST_IDLE);
                return true;
            case ME_PEER_REMOVED:
            case ME_PEER_ACK:
                return true;
            default:
                break;
            }
            return false;
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::OnStateChangeRoutingOnly(AZ::HSM& sm, const AZ::HSM::Event& event)
        {
            switch (event.id)
            {
            case AZ::HSM::EnterEventId:
                //AZ_TracePrintf("GridMate", "Replica 0x%x entering migration state MST_CHANGE_ROUTING_ONLY.\n", m_replica->GetRepId());
                return true;
            case ME_UPDATE:
            {
                UpdateReplicaRouting();
                sm.Transition(MST_IDLE);
                return true;
            }
            default:
                break;
            }
            return false;
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::DefaultHandler(AZ::HSM& sm, const AZ::HSM::Event& event)
        {
            (void)sm;
            switch (event.id)
            {
            case AZ::HSM::EnterEventId:
                //AZ_TracePrintf("GridMate", "Replica 0x%x entering migration state MST_IDLE.\n", m_replica->GetRepId());
                return true;
            case ME_MODIFY_NEW_OWNER:
                m_newOwnerId = *static_cast<PeerId*>(event.userData);
                if (m_replica->IsMaster() && m_newOwnerId != m_replicaMgr->m_self.GetId())
                {
                    sm.Transition(MST_MIGRATING);
                }
                else
                {
                    sm.Transition(MST_CHANGE_ROUTING_ONLY);
                }
                return true;
            case ME_REPLICA_REMOVED:
                if (sm.GetCurrentState() != MST_IDLE)
                {
                    sm.Transition(MST_IDLE);
                }
                return true;
            default:
                break;
            }
            return true;
        }
        //-----------------------------------------------------------------------------
        void MigrationSequence::OnDeactivateReplica(ReplicaId replicaId, ReplicaManager* pMgr)
        {
            if (pMgr == m_replicaMgr && replicaId == m_replica->GetRepId())
            {
                m_sm.Dispatch(ME_REPLICA_REMOVED);
            }
        }
        //-----------------------------------------------------------------------------
        void MigrationSequence::OnPeerRemoved(PeerId peerId, ReplicaManager* pMgr)
        {
            if (pMgr == m_replicaMgr)
            {
                AZ::HSM::Event ev;
                ev.id = ME_PEER_REMOVED;
                ev.userData = &peerId;
                m_sm.Dispatch(ev);
            }
        }
        //-----------------------------------------------------------------------------
        void MigrationSequence::OnReceivedAckUpstreamSuspended(PeerId from, AZ::u32 requestTime)
        {
            if (requestTime == m_timestamp)
            {
                //AZ_TracePrintf("GridMate", "Accepted upstream suspend ack response requested at %u for 0x%x from 0x%x.\n", requestTime, m_replica->GetRepId(), from);
                AZ::HSM::Event ev;
                ev.id = ME_PEER_ACK;
                ev.userData = &from;
                m_sm.Dispatch(ev);
            }
        }
        //-----------------------------------------------------------------------------
        void MigrationSequence::OnReceivedAckDownstream(PeerId from, AZ::u32 requestTime)
        {
            if (requestTime == m_timestamp)
            {
                //AZ_TracePrintf("GridMate", "Accepted downstream ack response requested at %u for 0x%x from 0x%x.\n", requestTime, m_replica->GetRepId(), from);
                AZ::HSM::Event ev;
                ev.id = ME_PEER_ACK;
                ev.userData = &from;
                m_sm.Dispatch(ev);
            }
        }
        //-----------------------------------------------------------------------------
        bool MigrationSequence::UpdateReplicaRouting()
        {
            // If we have a direct connection to the owner, then
            // then the next hop is the owner peer, otherwise it is the host
            ReplicaPeer* nextHop = nullptr;
            if (m_newOwnerId == m_replicaMgr->GetLocalPeerId())
            {
                nextHop = &m_replicaMgr->m_self;
            }
            else
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_replicaMgr->m_mutexRemotePeers);
                for (auto& route : m_replicaMgr->m_remotePeers)
                {
                    if (route->GetId() == m_newOwnerId)
                    {
                        nextHop = route;
                        break;
                    }
                    if (route->IsSyncHost())
                    {
                        nextHop = route;
                    }
                }
            }
            if (nextHop)
            {
                if (nextHop != m_replica->m_upstreamHop)
                {
                    if (m_replica->m_upstreamHop)
                    {
                        m_replica->m_upstreamHop->Remove(m_replica);
                    }
                    nextHop->Add(m_replica);
                }
                return true;
            }
            return false;
        }
        //-----------------------------------------------------------------------------
    }   // namespace ReplicaInternal
}   // namespace GridMate
