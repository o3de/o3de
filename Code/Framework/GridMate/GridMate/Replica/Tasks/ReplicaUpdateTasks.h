/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_UPDATETASKS_H
#define GM_REPLICA_UPDATETASKS_H

#include <GridMate/Replica/Tasks/ReplicaTaskManager.h>
#include <GridMate/Replica/Replica.h>

namespace GridMate
{
    /**
    *  Base class for all replica update tasks.
    *  Holding reference to replica, and providing it's subclasses access to replica's internals.
    */
    class ReplicaUpdateTaskBase
        : public ReplicaTask
    {
    protected:
        ReplicaUpdateTaskBase(ReplicaPtr replica);
        ~ReplicaUpdateTaskBase();

        bool ProcessRPCs(const ReplicaContext& rc);
        bool TryMigrate(ReplicaManager* rm, const ReplicaContext& rc);

        void DestroyReplica(ReplicaManager* rm, const ReplicaContext& rc);
    };

    /**
    *  Task to update primary & proxy replicas.
    *  Processes RPCs and calls replicas UpdateFromReplica. Will complete immediately if no RPCs
    *  left queued after processing, otherweise will be repeated next update tick.
    *  Initiates replica migration if proxy owner has died.
    */
    class ReplicaUpdateTask
        : public ReplicaUpdateTaskBase
    {
    public:
        ReplicaUpdateTask(ReplicaPtr replica);

        TaskStatus Run(const RunContext& context) override;
    };

    /**
    *  Task to destroy proxy replicas.
    *  Queued when got proxy destruction event from network.
    *  Removes replica from it's peer, and destroys local replica.
    *  Should cancel all other replica's update events before running this.
    */
    class ReplicaUpdateDestroyedProxyTask
        : public ReplicaUpdateTaskBase
    {
    public:
        ReplicaUpdateDestroyedProxyTask(ReplicaPtr replica);

        TaskStatus Run(const RunContext& context) override;
    };

    /**
    *  Task to destroy peer.
    *  Deletes peer object. Calling DiscardOrphans on other peers.
    *  Should only be performed after migration and pending reports has processed to guarantee
    *  that peer's replicas are in a latest state and have transefered ownership.
    */
    class ReplicaDestroyPeerTask
        : public ReplicaTask
    {
    public:
        ReplicaDestroyPeerTask(ReplicaPeer* peer);
        ~ReplicaDestroyPeerTask();
        TaskStatus Run(const RunContext& context) override;

    private:
        ReplicaPeer* m_peer;
    };
} // namespace GridMate

#endif // GM_REPLICA_MARSHALTASK_H
