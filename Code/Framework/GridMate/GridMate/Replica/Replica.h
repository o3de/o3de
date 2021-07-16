/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_H
#define GM_REPLICA_H

#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/typetraits/is_base_of.h>

#include <GridMate/Containers/unordered_set.h>
#include <GridMate/Containers/vector.h>

#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaStatusInterface.h>
#include <GridMate/Replica/ReplicaTarget.h>

#include <GridMate/Serialize/MarshalerTypes.h>

namespace GridMate
{
    namespace ReplicaInternal
    {
        class MigrationSequence;
    }

    class ReplicaStatus;
    class ReplicaTask;
    class InterestManager;

    //-------------------------------------------------------------------------
    // Replica
    //-------------------------------------------------------------------------
    class Replica final
        : public ReplicaStatusInterface
    {
        friend class ReplicaChunkBase;
        friend class ReplicaManager;
        friend class ReplicaPeer;
        friend class ReplicaInternal::MigrationSequence;
        friend ReplicaStatus;

        friend class ReplicaMarshalTaskBase;
        friend class ReplicaUpdateTaskBase;
        friend class ReplicaMarshalUpstreamTask;
        friend class ReplicaMarshalUpdateTask;
        friend class RelayRpcsTask;
        friend class ReplicaMarshalTask;
        friend class ReplicaMarshalZombieToPeerTask;
        friend class ReplicaMarshalZombieTask;
        friend class SendLimitProcessPolicy;

        friend class ReplicaMarshalNewTask;

        friend class InterestManager;
        friend class ReplicaTarget;

        enum Flags
        {
            Rep_SyncStage         = 1 << 0,
            Rep_ManagedAlloc      = 1 << 1,
            Rep_CanMigrate        = 1 << 2,
            Rep_New               = 1 << 3,
            Rep_Primary            = 1 << 4,
            Rep_Active            = 1 << 6,
            Rep_ChangedOwner      = 1 << 7,
            Rep_SuspendDownstream = 1 << 8,

            Rep_Traits = Rep_SyncStage | Rep_ManagedAlloc | Rep_CanMigrate
        };

    public:
        typedef vector<ReplicaChunkPtr> ChunkListType;

        GM_CLASS_ALLOCATOR(Replica);

        static Replica* CreateReplica(const char* replicaName);

        void Destroy();

        void UpdateReplica(const ReplicaContext& rc); // Called when updating replica primary from source
        void UpdateFromReplica(const ReplicaContext& rc); // Called when updating game with replica info
        bool AcceptChangeOwnership(PeerId requestor, const ReplicaContext& rc); // Return true to accept the transfer
        void OnActivate(const ReplicaContext& rc);
        void OnDeactivate(const ReplicaContext& rc);
        void OnChangeOwnership(const ReplicaContext& rc);

        bool AttachReplicaChunk(const ReplicaChunkPtr& chunk);
        bool DetachReplicaChunk(const ReplicaChunkPtr& chunk);

        ReplicaId GetRepId() const { return m_myId; }
        PeerId GetPeerId() const;
        const char* GetDebugName() const;
        unsigned int GetCreateTime() const { return m_createTime; }
        ReplicaContext GetMyContext() const;
        ReplicaManager* GetReplicaManager() { return m_manager; }

        void RegisterMarshalingTask(ReplicaTask* task) { m_marshalingTasks.insert(task); }
        void UnregisterMarshalingTask(ReplicaTask* task) { m_marshalingTasks.erase(task); }
        bool HasMarshalingTask() const { return !m_marshalingTasks.empty(); }

        void RegisterUpdateTask(ReplicaTask* task) { m_updateTasks.insert(task); }
        void UnregisterUpdateTask(ReplicaTask* task) { m_updateTasks.erase(task); }
        bool HasUpdateTask() const { return !m_updateTasks.empty(); }

        void RequestChangeOwnership(PeerId newOwner = InvalidReplicaPeerId); // If newOwner is not specified we assume it should be the local peer

        bool IsPrimary() const { return !IsActive() || !!(m_flags & Rep_Primary); }
        bool IsProxy() const { return !IsPrimary(); }
        bool IsNew() const { return !!(m_flags & Rep_New); }
        bool IsNewOwner() const { return !!(m_flags & Rep_ChangedOwner); }
        bool IsActive() const { return !!(m_flags & Rep_Active); }
        void SetSyncStage(bool b = true);
        bool IsSyncStage() const { return !!(m_flags & Rep_SyncStage); }
        bool IsMigratable() const { return !!(m_flags & Rep_CanMigrate); }
        bool IsDirty() const { return m_dirtyHook.m_prev || m_dirtyHook.m_next; }
        bool IsBroadcast() const;
        bool IsUpdateFromReplicaEnabled() const;

        ReplicaPriority GetPriority() const { return m_priority; } // returns replica's priority aggregated across all its chunks

        size_t GetNumChunks() const { return m_chunks.size(); }
        ReplicaChunkPtr GetChunkByIndex(size_t index);

        template<class R>
        inline AZStd::intrusive_ptr<R> FindReplicaChunk();

        AZ::u64 GetRevision() const { return m_revision; };

        //---------------------------------------------------------------------
        // DEBUG and Test Interface. Do not use in production code.
        //---------------------------------------------------------------------
        const ReplicaTargetList&    DebugGetTargets() const { return m_targets; }
        PrepareDataResult           DebugPrepareData(EndianType endian, AZ::u32 marshalFlags) { return PrepareData(endian, marshalFlags); }
        void                        DebugMarshal(MarshalContext& mc) { Marshal(mc); }
        void                        DebugPreDestruct() { PreDestruct(); }
        //---------------------------------------------------------------------
        
        explicit Replica(const char* replicaName);
        ~Replica();

    protected:
        //---------------------------------------------------------------------
        // refcount
        //---------------------------------------------------------------------
        template<class T>
        friend struct AZStd::IntrusivePtrCountPolicy;
        mutable unsigned int m_refCount;
        AZ_FORCE_INLINE void add_ref() { ++m_refCount; }
        void release();
        //---------------------------------------------------------------------

        //
        // These functions are internal to the replication system
        //
        void InitReplica(ReplicaManager* manager); // Initialize internal replica components. Called by ReplicaManager right before calling Activate()
        void Activate(const ReplicaContext& rc);
        void Deactivate(const ReplicaContext& rc);

        void PreDestruct();

        ReplicaChunkPtr CreateReplicaChunkFromStream(ReplicaChunkClassId classId, UnmarshalContext& mc);

        void MarkRPCsAsRelayed();

        void SetPrimary(bool isPrimary) { m_flags = isPrimary ? m_flags | Rep_Primary : m_flags & ~Rep_Primary; }
        void SetNew() { m_flags |= Rep_New; }
        void SetRepId(ReplicaId id);
        void SetMigratable(bool migratable);
        bool IsSuspendDownstream() const;

        void InternalCreateInitialChunks(const char* replicaName);

        PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags = 0);
        void Marshal(MarshalContext& mc);
        bool Unmarshal(UnmarshalContext& mc);

        bool ProcessRPCs(const ReplicaContext& rc);
        void ClearPendingRPCs();

        void OnReplicaPriorityUpdated(ReplicaChunkBase* chunk);

        //---------------------------------------------------------------------
        // RPC handlers
        //---------------------------------------------------------------------
        bool RequestOwnershipFn(PeerId requestor, const RpcContext& rpcContext) override;
        bool MigrationSuspendUpstreamFn(PeerId ownerId, AZ::u32 requestTime, const RpcContext& rpcContext) override;
        bool MigrationRequestDownstreamAckFn(PeerId ownerId, AZ::u32 requestTime, const RpcContext& rpcContext) override;
        //---------------------------------------------------------------------

        ReplicaId           m_myId;
        AZ::u32             m_flags;
        unsigned int        m_createTime;
        ReplicaManager*     m_manager;

        ReplicaPeer*        m_upstreamHop;

        ChunkListType       m_chunks;

        typedef unordered_set<ReplicaTask*> PendingTasks;
        PendingTasks        m_marshalingTasks;
        PendingTasks        m_updateTasks;
        AZStd::intrusive_list_node<Replica> m_dirtyHook;
        ReplicaChunkPtr     m_replicaStatus;
        ReplicaTargetList   m_targets;
        ReplicaPriority     m_priority;
        AZ::u64             m_revision;              ///< Change stamp. Increases every time a data set changes. Start at 1 to send initial value.
    };
    //-----------------------------------------------------------------------------
} // namespace GridMate

#include "ReplicaInline.inl"

#endif // GM_REPLICA_H
