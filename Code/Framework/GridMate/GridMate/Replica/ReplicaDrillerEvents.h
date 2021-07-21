/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_DRILLER_EVENTS_H
#define GM_REPLICA_DRILLER_EVENTS_H

#include <AzCore/Driller/DrillerBus.h>

/*!
* The replica system emits debugging EBus events via the ReplicaDrillerEvents interface.
* To listen for these events, derive from ReplicaDrillerBus::Handler and implement all
* the functions declared in the ReplicaDrillerEvents interface.
*/

namespace GridMate
{
    class Replica;
    class ReplicaChunk;
    class ReplicaChunkBase;
    class DataSetBase;
    typedef AZ::u32 PeerId;

    namespace Internal
    {
        struct RpcRequest;
    }

    namespace Debug
    {
        /*!
        * These are the driller events that the replica system will emit.
        * All functions in this interface should be implemented by the user.
        */
        class ReplicaDrillerEvents
            : public AZ::Debug::DrillerEBusTraits
        {
        public:
            //! Called when a replica is instantiated. It doesn't mean it will be added to the system.
            virtual void OnCreateReplica(Replica* replica) { (void)replica; }
            //! Called when a replica is actually destroyed.
            virtual void OnDestroyReplica(Replica* replica) { (void)replica; }
            //! Called when a replica is added to the system.
            virtual void OnActivateReplica(Replica* replica) { (void)replica; }
            //! Called when a replica is removed from the system.
            virtual void OnDeactivateReplica(Replica* replica) { (void)replica; }
            //! Called every time the replica data is sent to a peer.
            virtual void OnSendReplicaBegin(Replica* replica) { (void)replica; }
            //! Called every time the replica data is sent to a peer.
            virtual void OnSendReplicaEnd(Replica* replica, const void* data, size_t len) { (void)replica; (void)data; (void)len; }
            //! Called when data is received for a replica. Called with nullptr replica pointer when data for unknown replica received.
            virtual void OnReceiveReplicaBegin(Replica* replica, const void* data, size_t len) { (void)replica; (void)data; (void)len; }
            //! Called when data is received for a replica. Called with nullptr replica pointer when data for unknown replica received.
            virtual void OnReceiveReplicaEnd(Replica* replica) { (void)replica; }
            //! Called when an ownership transfer request is received.
            virtual void OnRequestReplicaChangeOwnership(Replica* replica, PeerId requestor) { (void)replica; (void)requestor; }
            //! Called when a replica changes ownership, not necessarily to or from the local node.
            virtual void OnReplicaChangeOwnership(Replica* replica, bool wasPrimary) { (void)replica; (void)wasPrimary; }

            //! Called when a chunk has been created. It doesn't mean it will be added to the system.
            //! Object will be partially constructed at this point if you inherit from ReplicaChunk
            virtual void OnCreateReplicaChunk(ReplicaChunkBase* chunk) { (void)chunk; }
            //! Called when a chuck is actually destroyed.
            virtual void OnDestroyReplicaChunk(ReplicaChunkBase* chunk) { (void)chunk; }
            //! Called when a chunk is added to the system.
            virtual void OnActivateReplicaChunk(ReplicaChunkBase* chunk) { (void)chunk; }
            //! Called when a chunk is removed from the system.
            virtual void OnDeactivateReplicaChunk(ReplicaChunkBase* chunk) { (void)chunk; }
            //! Called when a chunk is attached to a replica.
            virtual void OnAttachReplicaChunk(ReplicaChunkBase* chunk) { (void)chunk; }
            //! Called when a chunk is detached from a replica.
            virtual void OnDetachReplicaChunk(ReplicaChunkBase* chunk) { (void)chunk; }
            //! Called every time the chunk data is sent to a peer.
            virtual void OnSendReplicaChunkBegin(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, PeerId from, PeerId to) { (void)chunk; (void)chunkIndex; (void)from; (void)to; }
            //! Called every time the chunk data is sent to a peer.
            virtual void OnSendReplicaChunkEnd(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, const void* data, size_t len) { (void)chunk; (void)chunkIndex; (void)data; (void)len; }
            //! Called when data is received for a chunk.
            virtual void OnReceiveReplicaChunkBegin(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, PeerId from, PeerId to, const void* data, size_t len) { (void)chunk; (void)chunkIndex; (void)from; (void)to; (void)data; (void)len; }
            //! Called when data is received for a chunk.
            virtual void OnReceiveReplicaChunkEnd(ReplicaChunkBase* chunk, AZ::u32 chunkIndex) { (void)chunk; (void)chunkIndex; }

            //! Called every time a dataset is sent to a peer.
            virtual void OnSendDataSet(ReplicaChunkBase* chunk, AZ::u32 chunkIndex,  DataSetBase* dataSet, PeerId from, PeerId to, const void* data, size_t len) { (void)chunk; (void)chunkIndex; (void)dataSet; (void)from; (void)to; (void)data; (void)len; }
            //! Called when data is received for a dataset.
            virtual void OnReceiveDataSet(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, DataSetBase* dataSet, PeerId from, PeerId to, const void* data, size_t len) { (void)chunk; (void)chunkIndex; (void)dataSet; (void)from; (void)to; (void)data; (void)len; }

            //! Called when an rpc request is received. RpcRequest pointer will be null if rpc is called on primary replica.
            virtual void OnRequestRpc(ReplicaChunkBase* chunk, Internal::RpcRequest* rpc) { (void)chunk; (void)rpc; }
            //! Called when an rpc is invoked. RpcRequest pointer will be null if rpc is called on primary replica.
            virtual void OnInvokeRpc(ReplicaChunkBase* chunk, Internal::RpcRequest* rpc) { (void)chunk; (void)rpc; }
            //! Called every time an rpc is sent to a peer.
            virtual void OnSendRpc(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, Internal::RpcRequest* rpc, PeerId from, PeerId to, const void* data, size_t len) { (void)chunk; (void)chunkIndex; (void)rpc; (void)from; (void)to; (void)data; (void)len; }
            //! Called when an rpc is received.
            virtual void OnReceiveRpc(ReplicaChunkBase* chunk, AZ::u32 chunkIndex, Internal::RpcRequest* rpc, PeerId from, PeerId to, const void* data, size_t len) { (void)chunk; (void)chunkIndex; (void)rpc; (void)from; (void)to; (void)data; (void)len; }

            //! Called when a replica packet is sent.
            virtual void OnSend(PeerId to, const void* data, size_t len, bool isReliable) { (void)to; (void)data; (void)len; (void)isReliable; }
            //! Called when a replica packet is received.
            virtual void OnReceive(PeerId from, const void* data, size_t len) { (void)from; (void)data; (void)len; }
        };

        /*!
        * Replica driller events are sent are sent via this the ReplicaDrillerBus.
        * To receive events, derive a handler from ReplicaDrillerBus::Handler and
        * attach it to the bus.
        */
        typedef AZ::EBus<ReplicaDrillerEvents> ReplicaDrillerBus;
    } // namespace Debug
} // namespace GridMate

#endif // GM_REPLICA_DRILLER_EVENTS_H

#pragma once
