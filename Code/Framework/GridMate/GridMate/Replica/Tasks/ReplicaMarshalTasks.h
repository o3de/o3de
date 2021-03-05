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
#ifndef GM_REPLICA_MARSHALTASKS_H
#define GM_REPLICA_MARSHALTASKS_H

#include <GridMate/Replica/Tasks/ReplicaTaskManager.h>
#include <GridMate/Replica/Replica.h>

namespace GridMate
{
    /**
    *  ReplicaMarshalTaskBase: Base class for all replica marshaling tasks.
    *  Holding reference to replica, and providing it's subclasses access to replica's internals.
    */
    class ReplicaMarshalTaskBase
        : public ReplicaTask
    {
    protected:
        ReplicaMarshalTaskBase(ReplicaPtr replica);
        ~ReplicaMarshalTaskBase();

        void MarshalNewReplica(Replica* replica, ReservedIds cmdId, WriteBuffer& outBuffer);

        PrepareDataResult PrepareData(ReplicaPtr replica, EndianType endianType);

        ReplicaPeer* GetUpstreamHop();
        list<Internal::RpcRequest*>& GetRPCQueue();

        virtual bool CanUpstream() const;
        virtual void ResetMarshalState();
        virtual void OnSendReplicaBegin();
        virtual void OnSendReplicaEnd(ReplicaPeer* to, const void* data, size_t len);
    };

    /**
    *  Marshaling task. Initiates marshaling of a given replica to group of peers.
    *  Every time it's executed it requests marshaling targets for a replica and marshaling data to every peer.
    *  This task might be repeated through several ticks because of dataset updates.
    */
    class ReplicaMarshalTask
        : public ReplicaMarshalTaskBase
    {
    public:
        GM_CLASS_ALLOCATOR(ReplicaMarshalTask);

        ReplicaMarshalTask(ReplicaPtr replica);

        TaskStatus Run(const RunContext& context) override;
    };

    /**
    *  Task to marshal a zombie replica.
    *  Zombie replica contains latest state of real replica that was destroyed.
    *  Note: Later zombie replica itself should be replaced with this task completely.
    */
    class ReplicaMarshalZombieTask
        : public ReplicaMarshalTaskBase
    {
    public:
        GM_CLASS_ALLOCATOR(ReplicaMarshalZombieTask);

        ReplicaMarshalZombieTask(ReplicaPtr replica);

        TaskStatus Run(const RunContext& context) override;
    };
} // namespace GridMate

#endif // GM_REPLICA_MARSHALTASK_H
