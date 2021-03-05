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
#ifndef INCLUDE_GRIDMATEENTITYREPLICA_HEADER
#define INCLUDE_GRIDMATEENTITYREPLICA_HEADER

#pragma once

#include "../NetworkGridmateMarshaling.h"
#include "../Compatibility/GridMateRMI.h"
#include "../Compatibility/GridMateNetSerialize.h"
#include "../Compatibility/GridMateNetSerializeAspectProfiles.h"

#include "EntityReplicaSpawnParams.h"

#include <GridMate/Serialize/CompressionMarshal.h>
#include <GridMate/Replica/ReplicaFunctions.h>

enum EEntityAspects
{
    eEA_All                     = NET_ASPECT_ALL,

    #define ADD_ASPECT(x, y)   x = BIT(y),
    #include "../Compatibility/GridMateNetSerializeAspects.inl"
    #undef ADD_ASPECT
};

namespace GridMate
{
    class EntityScriptReplicaChunk;

    /*!
     * For replication of IEntity's.
     *
     * Upon being bound to the network, an EntityReplica is created on the server to ensure
     * the entity is spawned identically on all machines.
     *
     * This replica also supports a GridMate-backed compatibility implementation of the
     * CryEngine NetSerialize() model.
     */
    class EntityReplica
        : public GridMate::ReplicaChunk
    {
        friend class NetworkGridMate;

        /*
        * Special dataset customized to support aspects
        * Allows the dataset to be part of an aspect array while still using descriptive
        * debug names.
        */
        using AspectDataSet = DataSet<NetSerialize::AspectSerializeState, NetSerialize::AspectSerializeState::Marshaler>;
        class SerializedNetSerializeState : public AspectDataSet
        {
        public:
            GM_CLASS_ALLOCATOR(SerializedNetSerializeState);

            SerializedNetSerializeState();            

            void DispatchChangedEvent(const TimeContext& tc) override;

            size_t m_aspectIndex;

            PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags) override
            {
                return AspectDataSet::PrepareData(endianType, marshalFlags);
            }

            void SetDirty() override
            {
                AspectDataSet::SetDirty();
            }
            
            static size_t s_nextAspectIndex;   // Reset to 0 at the end of EntityReplica construction
        };

    public:
        /*
        * Entity replica construction parameteres
        */
        struct EntityReplicaCtorContext : public CtorContextBase
        {
            CtorDataSet<EntitySpawnParamsStorage, EntitySpawnParamsStorage::Marshaler> m_spawnParams;
        };

        /*
        * Chunk descriptor
        */
        class EntityReplicaDesc : public ReplicaChunkDescriptor
        {
        public:
            EntityReplicaDesc()
                : ReplicaChunkDescriptor(EntityReplica::GetChunkName(), sizeof(EntityReplica))
            {
            }

            ReplicaChunkBase* CreateFromStream(UnmarshalContext& mc) override
            {
                ReplicaChunkBase* replicaChunk = nullptr;
                AZ_Assert(!mc.m_rm->IsSyncHost(), "EntityReplica can only be owned by the host!");
                if (!mc.m_rm->IsSyncHost())
                {
                    EntityReplicaCtorContext ctorContext;
                    ctorContext.Unmarshal(*mc.m_iBuf);

                    replicaChunk = CreateReplicaChunk<EntityReplica>(ctorContext.m_spawnParams.Get());
                }
                else
                {
                    DiscardCtorStream(mc);
                }

                return replicaChunk;
            }

            void DiscardCtorStream(UnmarshalContext& mc) override
            {
                EntityReplicaCtorContext ctorContext;
                ctorContext.Unmarshal(*mc.m_iBuf);
            }

            void DeleteReplicaChunk(ReplicaChunkBase* chunkInstance) override
            {
                delete chunkInstance;
            }

            void MarshalCtorData(ReplicaChunkBase* chunkInstance, WriteBuffer& wb) override
            {
                EntityReplica* entityChunk = static_cast<EntityReplica*>(chunkInstance);
                EntityReplicaCtorContext ctorContext;
                ctorContext.m_spawnParams.Set(entityChunk->GetSerializedSpawnParams());
                ctorContext.Marshal(wb);
            }
        };

        GM_CLASS_ALLOCATOR(EntityReplica);

        EntityReplica();
        EntityReplica(const EntitySpawnParamsStorage& paramsStorage);
        virtual ~EntityReplica() {};

        //////////////////////////////////////////////////////////////////////
        //! GridMate::ReplicaChunk overrides.
        static const char* GetChunkName() { return "EntityReplicaChunk"; }
        void UpdateChunk(const GridMate::ReplicaContext& rc) override { (void)rc; }
        void OnReplicaActivate(const GridMate::ReplicaContext& rc) override;
        void OnReplicaDeactivate(const GridMate::ReplicaContext& rc) override;
        void UpdateFromChunk(const GridMate::ReplicaContext& rc) override { (void)rc; }
        bool IsReplicaMigratable() override;        
        //////////////////////////////////////////////////////////////////////

        //! Retrieve the entity's spawn params as serialized from the server.
        const EntitySpawnParamsStorage& GetSerializedSpawnParams() const;

        //! Returns the Id of the local entity for this replica.
        EntityId GetLocalEntityId() const;

        //! Unbinds from the local entity
        void UnbindLocalEntity();

        //! Handler for game code calls to ChangedNetworkState(). This tells the replica that we need to gather new
        //! data for this aspect. Internally we keep a hash, so only actual changes will result in a re-send.
        void MarkAspectsDirty(NetworkAspectType aspects);

        //! Returns a bitmask of the current aspects that are marked dirty.
        NetworkAspectType GetDirtyAspects() const;

        //! Returns true if the local machine has client-aspect authority, and the specified
        //! aspect is in fact delegated.
        bool IsAspectDelegatedToThisClient(size_t aspectIndex) const;

        //! Returns true if the local machine has client-aspect authority
        bool IsAspectDelegatedToThisClient() const;

        //! Upload client-delegated aspects to the server.
        void UploadClientDelegatedAspects();

        //! Marks aspects that are delegated to the controlling authority.
        void SetClientDelegatedAspectMask(NetworkAspectType aspects);

        //! Retrieves client-delegated aspects for this replica.
        NetworkAspectType GetClientDelegatedAspectMask() const;

        //! Retrieves the active profile for the specified aspect.
        NetSerialize::AspectProfile GetAspectProfile(size_t aspectIndex) const;

        //! Sets the active profile for the specified aspect.
        void SetAspectProfile(size_t aspectIndex, NetSerialize::AspectProfile profile);

        //! RPC for dispatching legacy-style CryEngine RMIs.
        bool HandleLegacyServerRMI(RMI::LegacyInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc);
        GridMate::Rpc<GridMate::RpcArg<RMI::LegacyInvocationWrapper::Ptr, RMI::LegacyInvocationWrapper::Marshaler>>::BindInterface<EntityReplica, &EntityReplica::HandleLegacyServerRMI> RPCHandleLegacyServerRMI;

        bool HandleLegacyClientRMI(RMI::LegacyInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc);
        GridMate::Rpc<GridMate::RpcArg<RMI::LegacyInvocationWrapper::Ptr, RMI::LegacyInvocationWrapper::Marshaler>>::BindInterface<EntityReplica, &EntityReplica::HandleLegacyClientRMI, RMI::ClientRMITraits> RPCHandleLegacyClientRMI;

        //! RPC for dispatching GameCore Actor RMIs.
        bool HandleActorServerRMI(RMI::ActorInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc);
        GridMate::Rpc<GridMate::RpcArg<RMI::ActorInvocationWrapper::Ptr, RMI::ActorInvocationWrapper::Marshaler>>::BindInterface<EntityReplica, &EntityReplica::HandleActorServerRMI> RPCHandleActorServerRMI;

        bool HandleActorClientRMI(RMI::ActorInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc);
        GridMate::Rpc<GridMate::RpcArg<RMI::ActorInvocationWrapper::Ptr, RMI::ActorInvocationWrapper::Marshaler>>::BindInterface<EntityReplica, &EntityReplica::HandleActorClientRMI, RMI::ClientRMITraits> RPCHandleActorClientRMI;

        //! RPC for dispatching client-delegated aspect updates.
        typedef ManagedFlexibleBuffer<256> AspectUploadBuffer;
        bool UploadClientAspect(uint32 aspectIndex, AspectUploadBuffer::Ptr buffer, const GridMate::RpcContext& rc);
        GridMate::Rpc<GridMate::RpcArg<uint32, Marshaler<uint32>>, GridMate::RpcArg<AspectUploadBuffer::Ptr, AspectUploadBuffer::PtrMarshaler>>::BindInterface<EntityReplica, &EntityReplica::UploadClientAspect> RPCUploadClientAspect;

        //! RPC for notifying clients of delegation.
        bool DelegateAuthorityToOwner(ChannelId ownerChannelId, const GridMate::RpcContext& rc);
        GridMate::Rpc<GridMate::RpcArg<ChannelId>>::BindInterface<EntityReplica, &EntityReplica::DelegateAuthorityToOwner> RPCDelegateAuthorityToOwner;

        //! Forces expedited handling of a new replica. This addresses a specific case where
        //! we need to establish the local entity during decoding of its master-side entity Id.
        EntityId HandleNewlyReceivedNow();

        enum EFlags
        {
            kFlag_None          = 0,
            kFlag_NewlyReceived = BIT(0),       //! The replica was just activated.
        };

        uint32 GetFlags() const         { return m_flags; }

    protected:
        AZ::u32 CalculateDirtyDataSetMask(MarshalContext& rc) override;
        void OnDataSetChanged(const DataSetBase& dataset) override;

    private: 

        //! Process a newly received replica. This includes establishing (either linking to or creating)
        //! the machine-local entity associated with the replica.
        void HandleNewlyReceived();

        //! Registers callbacks for aspect changes.
        void SetupAspectCallbacks();

        //! Commits a new data image to the aspect and prepares for outgoing marshaling.
        bool CommitAspectData(size_t aspectIndex, const char* newData, size_t newDataSize, uint32 hash);

        //! Delegate for changed aspect notifications.
        void OnAspectChanged(size_t aspectIndex);

        //! Delegate for changed aspect profiles.
        void OnAspectProfileChanged(size_t aspectIndex,
            NetSerialize::AspectProfile oldProfile,
            NetSerialize::AspectProfile newProfile);        

        //! Id of the *local* entity.
        EntityId m_localEntityId;
        //! Mask representing aspects that have been dirtied by the game (matters on master only).
        NetworkAspectType m_gameDirtiedAspects;

        //! Entity spawn parameters received from master.
        EntitySpawnParamsStorage m_spawnParams;

        //! Arbitrary spawn info gathered on the master.
        SerializedEntityExtraSpawnInfo m_extraSpawnInfo;

        //! Mask representing aspects the server has delegated tot he client.
        DataSet<NetworkAspectType> m_clientDelegatedAspects;

        //! Stored hashes to detect changes in client-delegated aspects (to prevent constant uploading).
        uint32 m_clientDelegatedAspectHashes[ NetSerialize::kNumAspectSlots ];

        //! Per-aspect profile (NetSerialize compatibility/shim).
        NetSerialize::SerializedEntityAspectProfiles m_aspectProfiles;

        //! Per-aspect serialization state (NetSerialize compatibility/shim).
        SerializedNetSerializeState m_netSerializeState[ NetSerialize::kNumAspectSlots ];        

        AZ::u32 m_modifiedDataSets;

        // Specific chunk for the script aspect to allow for independent updates of various parts of the aspect.
        EntityScriptReplicaChunk* m_scriptReplicaChunk;

        typedef std::vector<std::pair< RMI::LegacyInvocationWrapper::Ptr, RpcContext> > PendingLegacyRMIs;
        typedef std::vector<std::pair< RMI::ActorInvocationWrapper::Ptr, RpcContext> > PendingActorRMIs;

        //! RMIs are queued during the interval between receiving the replica, and having everything
        //! we need to spawn the local entity.
        PendingLegacyRMIs m_pendingLegacyRMIs;
        PendingActorRMIs m_pendingActorRMIs;

        WriteBufferDynamic m_masterAspectScratchBuffer;

        //! Set if the server has designated us as the authority of client-delegated aspects.
        bool m_isClientAspectAuthority;

        //! Internal state flags. See EFlags above for details.
        uint32 m_flags;
    };

    typedef AZStd::intrusive_ptr<EntityReplica> EntityReplicaPtr;
} // namespace GridMate

#endif // INCLUDE_GRIDMATEENTITYREPLICA_HEADER
