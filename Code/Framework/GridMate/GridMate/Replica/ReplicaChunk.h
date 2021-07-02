/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_CHUNK_H
#define GM_REPLICA_CHUNK_H

/** \file ReplicaChunk.h
    ReplicaChunk is a logical unit of network data for replication across the network. This file contains the base functionality for a
    ReplicaChunk. The user is expected to extend a ReplicaChunk object to create their own networkable classes.
*/

#include <GridMate/Containers/unordered_set.h>
#include <GridMate/Containers/list.h>

#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Replica/ReplicaChunkInterface.h>

#include <AzCore/std/containers/bitset.h>
#include <AzCore/std/containers/ring_buffer.h>

namespace UnitTest
{
    template <typename ComponentType>
    class NetContextMarshalFixture;
}

namespace GridMate
{
    class ReplicaChunkDescriptor;

    namespace Internal
    {
        struct RpcRequest;
    }

    // Replica Chunk Base
    /** A single unit of network functionality
        A ReplicaChunk is a user extendable network object. One or more ReplicaChunks can be
        owned by a Replica, which is both a container and manager for them. A replica is owned
        by a Primary, and is propagated to other network nodes, who interact with is as a Proxy.
        The data a ReplicaChunk contains should generally be related to the other data stored
        within it. Since multiple chunks can be attached to a Replica, unrelated data can simply
        be stored in other chunks on the same Replica.

        A ReplicaChunk has two primary ways to interact with it: DataSets and Remote Procedure
        Calls (RPCs).
        DataSets store arbitrary data, which only the Primary is able to modify. Any changes are
        propagated to the Proxy ReplicaChunks on the other nodes.
        RPCs are methods that can be executed on a remote node. They are first invoked on the
        Primary, who then decides if the invocation should be propagated to the Proxies.

        ReplicaChunks can be created by inheriting from the class and registered by calling
        ReplicaChunkDescriptorTable::RegisterChunkType() to create the factory required by
        the network.
        Every concrete replica chunk type needs to implement a static member function
        const char* GetChunkName(). The string returned by this function will be used to generate
        a ReplicaChunkClassId which will be used to identify this chunk type throughout the
        system.

        Replica chunks can be instantiated directly in a Replica, or standalone and attached to a Replica
        afterwards. Once attached to a replica they are bound to the network.

        To add a handler interface for RPC calls and DataSet changed events, call SetHandler with
        an object that inherits from ReplicaChunkInterface.

        Use ReplicaChunkBase as the parent class when the event handler logic is separate from the
        chunk itself. This is useful for example when a client and server want to connect different
        logic to the chunk.
    */
    class ReplicaChunkBase
    {
    public:
        friend Replica;
        friend RpcBase;
        friend DataSetBase;
        template<typename DataType, typename Marshaler, typename Throttle>
        friend class DataSet;
        template <typename ComponentType>
        friend class UnitTest::NetContextMarshalFixture;

        using RPCQueue = AZStd::ring_buffer<Internal::RpcRequest*, SysContAlloc>;
        /**
         * \brief Specify the maximum size of a RPC queues for each replica chunk.
         * This queue can grow while RPCs are being delivered back to all clients.
         */
        static constexpr AZStd::size_t MaxRpcQueueSize = 512;

        ReplicaChunkBase();
        virtual ~ReplicaChunkBase();

        //! Initializes the chunk. Must be called before the chunk can be used.
        void Init(ReplicaChunkClassId chunkTypeId);
        void Init(ReplicaChunkDescriptor* descriptor);

        bool IsClassType(ReplicaChunkClassId classId) const;

        ReplicaChunkDescriptor* GetDescriptor() const { return m_descriptor; }
        ReplicaId GetReplicaId() const;
        PeerId GetPeerId() const;
        virtual ReplicaManager* GetReplicaManager();
        bool IsActive() const;
        bool IsPrimary() const;
        bool IsProxy() const;

        virtual void OnAttachedToReplica(Replica* replica) { (void) replica; }
        virtual void OnDetachedFromReplica(Replica* replica) { (void) replica; }

        virtual bool IsReplicaMigratable() = 0; // Return true to allow migration. A single chunk rejecting migration will prevent the replica itself from migrating

        virtual void UpdateChunk(const ReplicaContext& rc) { (void) rc; } // Called when updating replica with game info
        virtual void UpdateFromChunk(const ReplicaContext& rc) { (void) rc; } // Called when updating game with replica info

        virtual bool AcceptChangeOwnership(PeerId requestor, const ReplicaContext& rc) { (void) requestor; (void) rc; return true; } // Return true to accept the transfer
        virtual void OnReplicaActivate(const ReplicaContext& rc) { (void) rc; }
        virtual void OnReplicaDeactivate(const ReplicaContext& rc) { (void) rc; }
        virtual void OnReplicaChangeOwnership(const ReplicaContext& rc) { (void) rc; }

        virtual bool IsUpdateFromReplicaEnabled() { return true; } // Return false to suspend getting updates from replica, rpcs and dataset changes callbacks will be queued

        Replica* GetReplica() { return m_replica; }

        void SetHandler(ReplicaChunkInterface* handler) { m_handler = handler; }
        ReplicaChunkInterface* GetHandler() { return m_handler; }

        ReplicaPriority GetPriority() const { return m_priority; }
        void SetPriority(ReplicaPriority priority);

        virtual bool ShouldSendToPeer(ReplicaPeer* peer) const;

        template<typename T>
        bool IsType()
        {
            return IsClassType(ReplicaChunkClassId(T::GetChunkName()));
        }

        virtual bool IsBroadcast() { return false; }

        AZ::u64 GetLastChangeStamp() const { return m_revision; };

        virtual bool ShouldBindToNetwork();

    private:
        //---------------------------------------------------------------------
        // DEBUG and Test Interface. Do not use in production code.
        //---------------------------------------------------------------------
        virtual AZ::u32 Debug_CalculateDirtyDataSetMask(MarshalContext& mc) { return CalculateDirtyDataSetMask(mc); }
        virtual void Debug_OnDataSetChanged(const DataSetBase& dataSet) { OnDataSetChanged(dataSet); }
        virtual void Debug_Marshal(MarshalContext& mc, AZ::u32 chunkIndex) { Marshal(mc, chunkIndex); }
        virtual void Debug_Unmarshal(UnmarshalContext& mc, AZ::u32 chunkIndex) { Unmarshal(mc, chunkIndex); }
        PrepareDataResult Debug_PrepareData(EndianType endianType, AZ::u32 marshalFlags) { return PrepareData(endianType, marshalFlags); }
        void Debug_AttachedToReplica(Replica* replica) { AttachedToReplica(replica); }

    protected:

        virtual AZ::u32 CalculateDirtyDataSetMask(MarshalContext& mc);
        virtual void OnDataSetChanged(const DataSetBase& dataSet);
        virtual void Marshal(MarshalContext& mc, AZ::u32 chunkIndex);
        virtual void Unmarshal(UnmarshalContext& mc, AZ::u32 chunkIndex);

        //---------------------------------------------------------------------
        // refcount
        //---------------------------------------------------------------------
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        mutable unsigned int m_refCount;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        void release();

        void AttachedToReplica(Replica* replica);
        void DetachedFromReplica();

        bool IsDirty(AZ::u32 marshalFlags) const;
        PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags);

        void MarshalDataSets(MarshalContext& mc, AZ::u32 chunkIndex);
        void MarshalRpcs(MarshalContext& mc, AZ::u32 chunkIndex);

        void UnmarshalDataSets(UnmarshalContext& mc, AZ::u32 chunkIndex);
        void UnmarshalRpcs(UnmarshalContext& mc, AZ::u32 chunkIndex);

        void InternalUpdateChunk(const ReplicaContext& rc); // Called when updating replica with game info
        void InternalUpdateFromChunk(const ReplicaContext& rc); // Called when updating game with replica info

        void AddDataSetEvent(DataSetBase* dataset); // Called to enqueue a user event handler for a modified DataSet on a proxy node

        void SignalDataSetChanged(const DataSetBase& dataset); // Called when the DataSet changes on the primary node

        void EnqueueMarshalTask();

        void QueueRPCRequest(GridMate::Internal::RpcRequest* rpc);
        bool ProcessRPCs(const ReplicaContext& rc);
        void MarkRPCsAsRelayed();
        void ClearPendingRPCs();

        enum Flags
        {
            RepChunk_Updated = 1 << 0
        };

        Replica *                   m_replica;
        ReplicaChunkDescriptor *    m_descriptor;
        AZ::u32                     m_flags;

        RPCQueue m_rpcQueue{MaxRpcQueueSize};
        ReplicaChunkInterface* m_handler;

        AZStd::bitset<GM_MAX_DATASETS_IN_CHUNK> m_reliableDirtyBits;
        AZStd::bitset<GM_MAX_DATASETS_IN_CHUNK> m_unreliableDirtyBits;

        /*
         * Each bit value of 0 marks a dataset as still having the default value from the initial creation of the replica.
         * A bit value of 1 indicates that the associated dataset has been modified since its default constructor value.
         *
         * Internally, this is used to optimize marshaling of datasets to new proxies by omitting sending default constructor values of datasets.
         */
        AZStd::bitset<GM_MAX_DATASETS_IN_CHUNK> m_nonDefaultValueBits;

        AZ::u32 m_nDownstreamReliableRPCs;
        AZ::u32 m_nDownstreamUnreliableRPCs;
        AZ::u32 m_nUpstreamReliableRPCs;
        AZ::u32 m_nUpstreamUnreliableRPCs;

        AZ::u32 m_dirtiedDataSets; // Downstream changed DataSet bits for triggering the event handler
        ReplicaPriority m_priority;
        AZ::u64             m_revision; //change stamp. increases every time a data set changes
    };

    // Replica Chunk - custom class for internal GridMate classes. You should use @ReplicaChunkBase for AZ::Component work!
    /**
        Use ReplicaChunk as a parent class when the chunk contains the logic for its network events.
        This is useful for peer to peer environments and when the same code can be shared between
        client and server.
    **/
    class ReplicaChunk
        : public ReplicaChunkBase
        , public ReplicaChunkInterface
    {
    public:
        ReplicaChunk()
        {
            SetHandler(this);
        }
    };
} // namespace GridMate

#endif // GM_REPLICA_CHUNK_H
