/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Replica/Tasks/ReplicaUpdateTasks.h>
#include <GridMate/Replica/SystemReplicas.h>
#include <GridMate/Replica/ReplicaMgr.h>

namespace GridMate
{
    //-----------------------------------------------------------------------------
    ReplicaUpdateTaskBase::ReplicaUpdateTaskBase(ReplicaPtr replica)
    {
        m_replica = replica;
        AZ_Assert(m_replica, "Invalid replica given");
        m_replica->RegisterUpdateTask(this);
    }
    //-----------------------------------------------------------------------------
    ReplicaUpdateTaskBase::~ReplicaUpdateTaskBase()
    {
        if (m_replica)
        {
            m_replica->UnregisterUpdateTask(this);
        }
    }
    //-----------------------------------------------------------------------------
    bool ReplicaUpdateTaskBase::ProcessRPCs(const ReplicaContext& rc)
    {
        return m_replica->ProcessRPCs(rc);
    }
    //-----------------------------------------------------------------------------
    bool ReplicaUpdateTaskBase::TryMigrate(ReplicaManager* rm, const ReplicaContext& rc)
    {
        // is already migrating?
        if (rm->m_activeMigrations.find(m_replica->GetRepId()) != rm->m_activeMigrations.end())
        {
            return false;
        }

        ReplicaPeer* peer = m_replica->m_upstreamHop;
        AZ_Assert(peer, "TryMigrate: Invalid peer");

        bool shouldMigrate = rm->IsSyncHost() && peer->IsOrphan() && !rm->m_sessionInfo->HasPendingReports(peer->GetId());
        if (shouldMigrate)
        {
            rm->MigrateReplica(m_replica, rm->m_self.GetId());
            rm->OnReplicaMigrated(m_replica, true, ReplicaContext(rm, rc, &rm->m_self));
        }

        return shouldMigrate;
    }
    //-----------------------------------------------------------------------------
    void ReplicaUpdateTaskBase::DestroyReplica(ReplicaManager* rm, const ReplicaContext& rc)
    {
        // Unregister from replica as it will be destroyed
        m_replica->UnregisterUpdateTask(this);

        rm->RemoveReplicaFromDownstream(m_replica, rc);
        m_replica = nullptr;
    }
    //-----------------------------------------------------------------------------
    ReplicaUpdateTask::ReplicaUpdateTask(ReplicaPtr replica)
        : ReplicaUpdateTaskBase(replica)
    {
    }
    //-----------------------------------------------------------------------------
    ReplicaTask::TaskStatus ReplicaUpdateTask::Run(const RunContext& context)
    {
        if (!m_replica->IsUpdateFromReplicaEnabled())
        {
            return TaskStatus::Repeat;
        }

        ReplicaContext rc(context.m_replicaManager, context.m_replicaManager->GetTime());

        bool isDone = ProcessRPCs(rc);

        if (m_replica->IsProxy())
        {
            m_replica->UpdateFromReplica(rc);
            TryMigrate(context.m_replicaManager, rc);
        }

        return isDone ? TaskStatus::Done : TaskStatus::Repeat;
    }
    //-----------------------------------------------------------------------------
    ReplicaUpdateDestroyedProxyTask::ReplicaUpdateDestroyedProxyTask(ReplicaPtr replica)
        : ReplicaUpdateTaskBase(replica)
    {
    }
    //-----------------------------------------------------------------------------
    ReplicaTask::TaskStatus ReplicaUpdateDestroyedProxyTask::Run(const RunContext& context)
    {
        if (!m_replica->IsUpdateFromReplicaEnabled())
        {
            return TaskStatus::Repeat;
        }

        ReplicaContext rc(context.m_replicaManager, context.m_replicaManager->GetTime());
        ProcessRPCs(rc);
        m_replica->UpdateFromReplica(rc);
        DestroyReplica(context.m_replicaManager, rc);

        return TaskStatus::Done;
    }
    //-----------------------------------------------------------------------------
    ReplicaDestroyPeerTask::ReplicaDestroyPeerTask(ReplicaPeer* peer)
        : m_peer(peer)
    {
        AZ_Assert(m_peer, "Invalid peer");
    }
    //-----------------------------------------------------------------------------
    ReplicaDestroyPeerTask::~ReplicaDestroyPeerTask()
    {
        delete m_peer;
    }
    //-----------------------------------------------------------------------------
    ReplicaTask::TaskStatus ReplicaDestroyPeerTask::Run(const RunContext& context)
    {
        ReplicaManager& rm = *context.m_replicaManager;
        if (rm.IsSyncHost())
        {
            context.m_replicaManager->m_sessionInfo->DiscardOrphansRpc(m_peer->GetId());
            if (m_peer->GetId() && m_peer->GetId() == rm.m_sessionInfo->m_formerHost) // peerId might be zero if that peer did not accomplish replica mgr's greetings yet
            {
                AZ_TracePrintf("GridMate", "Completed migration for orphaned peerId 0x%x. Announcing ourselves(peerId 0x%x) as new host.\n",
                    m_peer->GetId(),
                    rm.m_self.GetId());
                rm.m_sessionInfo->m_formerHost = 0;
                rm.m_sessionInfo->AnnounceNewHostRpc();
            }
        }

        delete m_peer;
        m_peer = nullptr;
        return TaskStatus::Done;
    }
} // namespace GridMate
