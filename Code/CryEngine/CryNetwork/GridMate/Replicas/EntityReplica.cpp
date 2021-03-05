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
#include "CryNetwork_precompiled.h"

#include "../NetworkGridMate.h"
#include "../NetworkGridmateDebug.h"
#include "../NetworkGridmateMarshaling.h"
#include "../NetworkGridMateEntityEventBus.h"

#include "EntityReplica.h"
#include "EntityScriptReplicaChunk.h"

#include <GridMate/Replica/ReplicaFunctions.h>

namespace GridMate
{
    static const NetworkAspectType kAllEntityAspectBits = (1 << NetSerialize::kNumAspectSlots) - 1;
    //-----------------------------------------------------------------------------
    size_t EntityReplica::SerializedNetSerializeState::s_nextAspectIndex = 0;

    //-----------------------------------------------------------------------------
    EntityReplica::SerializedNetSerializeState::SerializedNetSerializeState()
        : DataSet(Debug::GetAspectNameByBitIndex(s_nextAspectIndex))
    {
        m_aspectIndex = s_nextAspectIndex++;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::SerializedNetSerializeState::DispatchChangedEvent([[maybe_unused]] const TimeContext& tc)
    {
        static_cast<EntityReplica*>(m_replicaChunk)->OnAspectChanged(m_aspectIndex);
    }

    //-----------------------------------------------------------------------------
    EntityReplica::EntityReplica()
        : m_gameDirtiedAspects(kAllEntityAspectBits)
        , m_localEntityId(kInvalidEntityId)
        , RPCHandleLegacyServerRMI("RPCHandleLegacyServerRMI")
        , RPCHandleLegacyClientRMI("RPCHandleLegacyClientRMI")
        , RPCHandleActorServerRMI("RPCHandleActorServerRMI")
        , RPCHandleActorClientRMI("RPCHandleActorClientRMI")
        , RPCUploadClientAspect("RPCUploadClientAspect")
        , RPCDelegateAuthorityToOwner("RPCDelegateAuthorityToOwner")
        , m_extraSpawnInfo("ExtraSpawnInfo")
        , m_clientDelegatedAspects("ClientDelegatedAspects", 0)
        , m_aspectProfiles("AspectProfiles")
        , m_modifiedDataSets(0)
        , m_scriptReplicaChunk(nullptr)
        , m_masterAspectScratchBuffer(EndianType::BigEndian)
        , m_isClientAspectAuthority(false)
        , m_flags(0)
    {
        SerializedNetSerializeState::s_nextAspectIndex = 0;
    }

    //-----------------------------------------------------------------------------
    EntityReplica::EntityReplica(const EntitySpawnParamsStorage& paramsStorage)
        : m_gameDirtiedAspects(kAllEntityAspectBits)
        , m_localEntityId(kInvalidEntityId)
        , RPCHandleLegacyServerRMI("RPCHandleLegacyServerRMI")
        , RPCHandleLegacyClientRMI("RPCHandleLegacyClientRMI")
        , RPCHandleActorServerRMI("RPCHandleActorServerRMI")
        , RPCHandleActorClientRMI("RPCHandleActorClientRMI")
        , RPCUploadClientAspect("RPCUploadClientAspect")
        , RPCDelegateAuthorityToOwner("RPCDelegateAuthorityToOwner")
        , m_spawnParams(paramsStorage)
        , m_extraSpawnInfo("ExtraSpawnInfo")
        , m_clientDelegatedAspects("ClientDelegatedAspects", 0)
        , m_aspectProfiles("AspectProfiles")
        , m_modifiedDataSets(0)
        , m_scriptReplicaChunk(nullptr)
        , m_masterAspectScratchBuffer(EndianType::BigEndian)
        , m_isClientAspectAuthority(false)
        , m_flags(0)
    {
        SerializedNetSerializeState::s_nextAspectIndex = 0;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::OnReplicaActivate([[maybe_unused]] const GridMate::ReplicaContext& rc)
    {
        m_scriptReplicaChunk = GetReplica()->FindReplicaChunk<EntityScriptReplicaChunk>().get();

        Network& net = Network::Get();

        GM_DEBUG_TRACE("EntityReplica::OnActivate - IsMaster:%s EntityId:%u EntityName:%s EntityClass:%s, Address:0x%p",
            IsMaster() ? "yes" : "no",
            m_spawnParams.m_id,
            m_spawnParams.m_entityName.c_str(),
            m_spawnParams.m_className.c_str(),
            this);

        #if GRIDMATE_DEBUG_ENABLED
        for (size_t aspectIndex = 0; aspectIndex < NetSerialize::kNumAspectSlots; ++aspectIndex)
        {
            NetSerialize::AspectSerializeState::Marshaler& marshaler =
                m_netSerializeState[ aspectIndex ].GetMarshaler();

            marshaler.m_debugName = GridMate::Debug::GetAspectNameByBitIndex(aspectIndex);
            marshaler.m_debugIndex = aspectIndex;
        }
        #endif // GRIDMATE_DEBUG_ENABLED

        // Objects initially assume all globally-delegatable aspects are delegatable
        // by the object.
        m_clientDelegatedAspects.Set(eEA_All);

        if (IsMaster())
        {
            NetSerialize::EntityAspectProfiles aspectProfiles;
            for (size_t i = 0; i < NetSerialize::kNumAspectSlots; ++i)
            {
                aspectProfiles.SetAspectProfile(i, NetSerialize::kUnsetAspectProfile);
            }

            // Initialize aspect profiles
            m_aspectProfiles.Set(aspectProfiles);
        }
        else
        {
            // Flag replica such that we can establish (create or link) the local entity
            // associated this replica as soon as it's safe to do so.
            net.GetNewProxyEntityMap()[m_spawnParams.m_id] = this;
            m_flags |= kFlag_NewlyReceived;

            SetupAspectCallbacks();
        }
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::OnReplicaDeactivate([[maybe_unused]] const GridMate::ReplicaContext& rc)
    {
        GM_DEBUG_TRACE("EntityReplica::OnDeactivate - IsMaster:%s EntityId:%u EntityName:%s EntityClass:%s",
            IsMaster() ? "yes" : "no",
            m_spawnParams.m_id,
            m_spawnParams.m_entityName.c_str(),
            m_spawnParams.m_className.c_str());

        EBUS_EVENT_ID(m_localEntityId, NetworkGridMateEntityEventBus, OnEntityUnboundFromNetwork, GetReplica());

        // Remove knowledge of the now-dead replica.
        Network::Get().GetNewProxyEntityMap().erase(m_spawnParams.m_id);

        m_localEntityId = kInvalidEntityId;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::IsReplicaMigratable()
    {
        return false;
    }

    //-----------------------------------------------------------------------------
    EntityId EntityReplica::HandleNewlyReceivedNow()
    {
        if (m_localEntityId == kInvalidEntityId)
        {
            HandleNewlyReceived();
        }

        return m_localEntityId;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::HandleNewlyReceived()
    {
        if (m_spawnParams.m_id != kInvalidEntityId &&
            m_localEntityId == kInvalidEntityId)
        {
            Network& net = Network::Get();

            if (!net.AllowEntityCreation())
            {
                return;
            }

            bool isGameRules =
                !!(m_spawnParams.m_paramsFlags & EntitySpawnParamsStorage::kParamsFlag_IsGameRules);

            if (isGameRules)
            {
                GM_DEBUG_TRACE("Established game rules? %s", m_localEntityId != kInvalidEntityId ? "yes" : "no");
            }
            else
            {
                GM_DEBUG_TRACE_LEVEL(2, "Waiting for game rules...");
                return;
            }

            // Flush pending RMIs.
            if (kInvalidEntityId != m_localEntityId)
            {
                GM_DEBUG_TRACE("Flushing pending RMIs (%u / %u)",
                    m_pendingLegacyRMIs.size(), m_pendingActorRMIs.size());

                for (const auto& rmi : m_pendingLegacyRMIs)
                {
                    HandleLegacyClientRMI(rmi.first, rmi.second);
                }

                for (const auto& rmi : m_pendingActorRMIs)
                {
                    HandleActorClientRMI(rmi.first, rmi.second);
                }

                m_pendingLegacyRMIs.clear();
                m_pendingActorRMIs.clear();
            }
        }

        m_flags &= ~kFlag_NewlyReceived;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::UnbindLocalEntity()
    {
        m_localEntityId = kInvalidEntityId;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::SetupAspectCallbacks()
    {
        using namespace NetSerialize;

        for (size_t aspectIndex = 0; aspectIndex < NetSerialize::kNumAspectSlots; ++aspectIndex)
        {
            SerializedNetSerializeState& aspectState = m_netSerializeState[ aspectIndex ];
            AspectSerializeState::Marshaler& marshaler = aspectState.GetMarshaler();

            // Trigger initial dispatch of all aspects.
            marshaler.MarkWaitingForDispatch();
        }

        // Setup client-side callback for aspect profile changes.
        using namespace AZStd::placeholders;
        auto aspectProfileCallback =
            AZStd::bind(&EntityReplica::OnAspectProfileChanged, this, _1, _2, _3);
        m_aspectProfiles.GetMarshaler().SetChangeDelegate(aspectProfileCallback);
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::CommitAspectData(size_t aspectIndex, const char* newData, size_t newDataSize, uint32 hash)
    {
        GM_ASSERT_TRACE(newData, "Invalid data buffer.");

        SerializedNetSerializeState& aspectState = m_netSerializeState[ aspectIndex ];

        // Update outgoing storage for marshaling.
        NetSerialize::AspectSerializeState::Marshaler& marshaler = aspectState.GetMarshaler();
        if (marshaler.GetStorageSize() < newDataSize)
        {
            marshaler.AllocateAspectSerializationBuffer(newDataSize);
        }

        if (newDataSize > 0)
        {
            FRAME_PROFILER("AspectBufferCopy", GetISystem(), PROFILE_NETWORK);
            WriteBufferType writeBuffer = marshaler.GetWriteBuffer();
            writeBuffer.Clear();
            writeBuffer.WriteRaw(newData, newDataSize);
        }

        // Store updated contents & hash. Any change will result in a downstream update.
        NetSerialize::AspectSerializeState updatedState = aspectState.Get();
        bool changed = false;

        {
            FRAME_PROFILER("AspectBufferHash", GetISystem(), PROFILE_NETWORK);
            changed = updatedState.UpdateHash(hash, newDataSize);
        }
        {
            FRAME_PROFILER("AspectUpdate", GetISystem(), PROFILE_NETWORK);
            aspectState.Set(updatedState);
        }

        if (changed)
        {
            FRAME_PROFILER("AspectSentEvent", GetISystem(), PROFILE_NETWORK);
            EBUS_EVENT(NetworkSystemEventBus, AspectSent, m_localEntityId, BIT(aspectIndex), newDataSize);
        }

        return changed;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::IsAspectDelegatedToThisClient(size_t aspectIndex) const
    {
        const NetworkAspectType engineAspectBit = BIT(aspectIndex);

        return IsAspectDelegatedToThisClient() &&                                  // Authority over this entity has been delegated to this client.
               !!(engineAspectBit & NetSerialize::GetDelegatableAspectMask()) &&   // This aspect supports client-delegation (globally).
               !!(engineAspectBit & m_clientDelegatedAspects.Get());               // This aspect supports client-delegation (on this object).
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::IsAspectDelegatedToThisClient() const
    {
        return m_isClientAspectAuthority;   // Authority over this entity has been delegated to this client.
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::OnAspectChanged(size_t aspectIndex)
    {
        GM_ASSERT_TRACE(!IsMaster(), "We shouldn't have unmarshaled on master.");

        SerializedNetSerializeState& aspectState = m_netSerializeState[ aspectIndex ];
        aspectState.GetMarshaler().MarkWaitingForDispatch();
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::OnAspectProfileChanged([[maybe_unused]] size_t aspectIndex,
        [[maybe_unused]] NetSerialize::AspectProfile oldProfile,
        [[maybe_unused]] NetSerialize::AspectProfile newProfile)
    {
    }
    //-----------------------------------------------------------------------------
    bool EntityReplica::HandleLegacyServerRMI(RMI::LegacyInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        AZ_Assert(IsMaster(), "Legacy Server RMIs should only ever be processed on the server!");
        if (IsMaster())
        {
            AZ_Assert(kInvalidEntityId != m_localEntityId, "local entity ids should be immediately available on the server!");
            if (kInvalidEntityId != m_localEntityId)
            {
                RMI::HandleLegacy(m_localEntityId, invocation, rc);
            }
        }
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::HandleLegacyClientRMI(RMI::LegacyInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        if (kInvalidEntityId != m_localEntityId)
        {
            return RMI::HandleLegacy(m_localEntityId, invocation, rc);
        }

        m_pendingLegacyRMIs.push_back(std::make_pair(invocation, rc));
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::HandleActorServerRMI(RMI::ActorInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        AZ_Assert(IsMaster(), "Legacy Server RMIs should only ever be processed on the server!");
        if (IsMaster())
        {
            AZ_Assert(kInvalidEntityId != m_localEntityId, "local entity ids should be immediately available on the server!");
            if (kInvalidEntityId != m_localEntityId)
            {
                RMI::HandleActor(m_localEntityId, invocation, rc);
            }
        }
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::HandleActorClientRMI(RMI::ActorInvocationWrapper::Ptr invocation, const GridMate::RpcContext& rc)
    {
        if (kInvalidEntityId != m_localEntityId)
        {
            return RMI::HandleActor(m_localEntityId, invocation, rc);
        }

        m_pendingActorRMIs.push_back(std::make_pair(invocation, rc));
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::UploadClientAspect([[maybe_unused]] uint32 aspectIndex, AspectUploadBuffer::Ptr buffer, [[maybe_unused]] const GridMate::RpcContext& rc)
    {
        GM_ASSERT_TRACE(buffer.get(), "UploadClientAspect: Empty buffer received for client-delegated aspect.");

        if (buffer.get())
        {
            const char* data = buffer->GetData();
            size_t dataSize = buffer->GetSize();
            ReadBufferType rb(EndianType::BigEndian, data, dataSize);
        }

        // No need to pass on - this only occurs on the server, and changes will be marshaled
        // down through aspect states.
        return false;
    }

    //-----------------------------------------------------------------------------
    bool EntityReplica::DelegateAuthorityToOwner(ChannelId ownerChannelId, [[maybe_unused]] const GridMate::RpcContext& rc)
    {
        if (!IsMaster() && Network::Get().GetLocalChannelId() == ownerChannelId)
        {
            m_isClientAspectAuthority = true;

            // Wipe hash values for client-delegated aspects.
            for (size_t aspectIndex = 0; aspectIndex < NetSerialize::kNumAspectSlots; ++aspectIndex)
            {
                m_clientDelegatedAspectHashes[ aspectIndex ] = 0;
            }

            m_gameDirtiedAspects = 0;
        }

        return true;
    }

    //-----------------------------------------------------------------------------
    const EntitySpawnParamsStorage& EntityReplica::GetSerializedSpawnParams() const
    {
        return m_spawnParams;
    }

    //-----------------------------------------------------------------------------
    EntityId EntityReplica::GetLocalEntityId() const
    {
        return m_localEntityId;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::MarkAspectsDirty(NetworkAspectType aspects)
    {
        m_gameDirtiedAspects |= aspects;
    }

    //-----------------------------------------------------------------------------
    NetworkAspectType EntityReplica::GetDirtyAspects() const
    {
        return m_gameDirtiedAspects;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::UploadClientDelegatedAspects()
    {
        m_gameDirtiedAspects = 0;
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::SetClientDelegatedAspectMask(NetworkAspectType aspects)
    {
        m_clientDelegatedAspects.Set(aspects);
    }

    //-----------------------------------------------------------------------------
    NetworkAspectType EntityReplica::GetClientDelegatedAspectMask() const
    {
        return m_clientDelegatedAspects.Get();
    }

    //-----------------------------------------------------------------------------
    NetSerialize::AspectProfile EntityReplica::GetAspectProfile(size_t aspectIndex) const
    {
        GM_ASSERT_TRACE(aspectIndex < NetSerialize::kNumAspectSlots, "Invalid aspect index: %u", aspectIndex);

        return m_aspectProfiles.Get().GetAspectProfile(aspectIndex);
    }

    //-----------------------------------------------------------------------------
    void EntityReplica::SetAspectProfile(size_t aspectIndex, NetSerialize::AspectProfile profile)
    {
        GM_ASSERT_TRACE(aspectIndex < NetSerialize::kNumAspectSlots, "Invalid aspect index: %u", aspectIndex);

        if (GetAspectProfile(aspectIndex) != profile)
        {
            NetSerialize::EntityAspectProfiles aspectProfiles = m_aspectProfiles.Get();
            aspectProfiles.SetAspectProfile(aspectIndex, profile);
            m_aspectProfiles.Set(aspectProfiles);
        }
    }
    //-----------------------------------------------------------------------------
    AZ::u32 EntityReplica::CalculateDirtyDataSetMask(MarshalContext& mc)
    {
        if ((mc.m_marshalFlags & ReplicaMarshalFlags::ForceDirty))
        {
            return m_modifiedDataSets;
        }

        return ReplicaChunkBase::CalculateDirtyDataSetMask(mc);
    }
    //-----------------------------------------------------------------------------
    void EntityReplica::OnDataSetChanged(const DataSetBase& dataSet)
    {
        // Keep track of which DataSets have been chagned, so when we initialize
        // we only initialize the DataSets with data.
        int index = GetDescriptor()->GetDataSetIndex(this,&dataSet);
        m_modifiedDataSets |= (1 << index);
    }
} // namespace GridMate
