/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>

#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/MigrationSequence.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaDrillerEvents.h>
#include <GridMate/Replica/ReplicaFunctions.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/ReplicaStatus.h>
#include <GridMate/Replica/ReplicaUtils.h>
#include <GridMate/Serialize/CompressionMarshal.h>

namespace GridMate
{
    //-----------------------------------------------------------------------------
    Replica* Replica::CreateReplica(const char* replicaName)
    {
        return aznew Replica(replicaName);
    }
    //-----------------------------------------------------------------------------
    Replica::Replica(const char* replicaName)
        : m_refCount(0)
        , m_myId(InvalidReplicaId)
        , m_flags(0)
        , m_createTime(0)
        , m_manager(nullptr)
        , m_upstreamHop(nullptr)
        , m_replicaStatus(nullptr)
        , m_priority(0)
        , m_revision(1)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        m_upstreamHop = nullptr;
        m_dirtyHook.m_next = m_dirtyHook.m_prev = nullptr;

#if GM_REPLICA_HAS_DEBUG_NAME == 0
        replicaName = nullptr;
#endif
        InternalCreateInitialChunks(replicaName);

        EBUS_EVENT(Debug::ReplicaDrillerBus, OnCreateReplica, this);
    }
    //-----------------------------------------------------------------------------
    Replica::~Replica()
    {
        AZ_Assert(m_refCount == 0, "Attempting to free replica with non-zero refCount(%d)!", m_refCount);
    }
    //-----------------------------------------------------------------------------
    void Replica::release()
    {
        AZ_Assert(m_refCount > 0, "Reference count logic error, trying to remove reference when refcount is 0");
        if (m_refCount == 1)
        {
            {
                ReplicaPtr reference = this;

                // PreDestruct - This is to run any destruction code that can call out to user code.
                // We temporarily hold a refcount here to prevent a double deletion if user code creates
                // and deletes another ref counted container, since that will cause the refcount
                // to change from 0 -> 1 -> 0 again.
                PreDestruct();
            }

            AZ_Assert(m_refCount == 1, "Attempting to hold on to replica refcount while deleting: refCount(%d)!", m_refCount);

            --m_refCount;
            delete this;
        }
        else
        {
            --m_refCount;
        }
    }
    //-----------------------------------------------------------------------------
    void Replica::PreDestruct()
    {
        AZ_PROFILE_FUNCTION(GridMate);

        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                chunk->DetachedFromReplica();
            }
        }
        m_chunks.clear();

        EBUS_EVENT(Debug::ReplicaDrillerBus, OnDestroyReplica, this);
    }
    //-----------------------------------------------------------------------------
    void Replica::Destroy()
    {
        AZ_Assert(IsPrimary(), "We don't own replica 0x%x!", GetRepId());
        if (m_manager)
        {
            m_manager->Destroy(this);
        }
    }
    //-----------------------------------------------------------------------------
    void Replica::InternalCreateInitialChunks(const char* replicaName)
    {
        ReplicaStatus* statusChunk = CreateReplicaChunk<ReplicaStatus>();

        if (replicaName)
        {
            statusChunk->SetDebugName(replicaName);
        }

        statusChunk->SetUpstreamSuspended(false);
        m_replicaStatus = statusChunk;
        AttachReplicaChunk(statusChunk);
    }
    //-----------------------------------------------------------------------------
    PeerId Replica::GetPeerId() const
    {
        PeerId peerId(InvalidReplicaPeerId);

        if (m_manager != nullptr)
        {
            peerId = m_manager->m_cfg.m_myPeerId;
        }

        return peerId;
    }
    //-----------------------------------------------------------------------------
    bool Replica::AttachReplicaChunk(const ReplicaChunkPtr& chunk)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        // Check for duplicate attach
        if (!chunk->GetReplica())
        {
            // Chunks cannot be attached while active
            if (!IsActive())
            {
                if (m_chunks.size() < GM_MAX_CHUNKS_PER_REPLICA)
                {
                    m_chunks.push_back(chunk);
                    OnReplicaPriorityUpdated(chunk.get());
                    chunk->AttachedToReplica(this);
                    AZ_Assert(chunk->GetReplica() == this, "Must be bound to the same replica");

                    return true;
                }
                else
                {
                    AZ_Warning("GridMate", false, "Cannot attach chunk %s because GM_MAX_CHUNKS_PER_REPLICA has been exceeded.", chunk->GetDescriptor()->GetChunkName());
                }
            }
            else
            {
                AZ_Warning("GridMate", false, "Cannot attach chunk %s while replica is active.", chunk->GetDescriptor()->GetChunkName());
            }
        }
        else
        {
            AZ_Warning("GridMate", false, "Cannot attach chunk %s because it is already attached to a replica.", chunk->GetDescriptor()->GetChunkName());
        }

        return false;
    }
    //-----------------------------------------------------------------------------
    bool Replica::DetachReplicaChunk(const ReplicaChunkPtr& chunk)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        if (!IsActive())
        {
            for (auto iter = m_chunks.begin(); iter != m_chunks.end(); ++iter)
            {
                if ((*iter) == chunk)
                {
                    (*iter)->DetachedFromReplica();
                    m_chunks.erase(iter);
                    OnReplicaPriorityUpdated(chunk.get());
                    return true;
                }
            }
        }
        else
        {
            AZ_Warning("GridMate", false, "Cannot detach chunk %s because the replica is active.", chunk->GetDescriptor()->GetChunkName());
        }
        return false;
    }
    //-----------------------------------------------------------------------------
    const char* Replica::GetDebugName() const
    {
        return static_cast<ReplicaStatus*>(m_replicaStatus.get())->GetDebugName();
    }
    //-----------------------------------------------------------------------------
    ReplicaContext Replica::GetMyContext() const
    {
        ReplicaContext rc(nullptr, TimeContext());
        if (m_manager)
        {
            m_manager->GetReplicaContext(this, rc);
        }
        return rc;
    }
    //-----------------------------------------------------------------------------
    void Replica::UpdateReplica(const ReplicaContext& rc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                chunk->InternalUpdateChunk(rc);
            }
        }
    }
    //-----------------------------------------------------------------------------
    void Replica::UpdateFromReplica(const ReplicaContext& rc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                chunk->InternalUpdateFromChunk(rc);
            }
        }
    }
    //-----------------------------------------------------------------------------
    bool Replica::AcceptChangeOwnership(PeerId requestor, const ReplicaContext& rc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                bool accepted = chunk->AcceptChangeOwnership(requestor, rc);
                if (!accepted)
                {
                    return false;
                }
            }
        }
        return true;
    }
    //-----------------------------------------------------------------------------
    void Replica::OnActivate(const ReplicaContext& rc)
    {
        EBUS_EVENT(Debug::ReplicaDrillerBus, OnActivateReplica, this);

        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                {
                    GM_PROFILE_USER_CALLBACK("OnReplicaActivate");
                    chunk->OnReplicaActivate(rc);
                }
                EBUS_EVENT(Debug::ReplicaDrillerBus, OnActivateReplicaChunk, chunk.get());
            }
        }
    }
    //-----------------------------------------------------------------------------
    void Replica::OnDeactivate(const ReplicaContext& rc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        EBUS_EVENT_ID(rc.m_rm->GetGridMate(), ReplicaMgrCallbackBus, OnDeactivateReplica, GetRepId(), rc.m_rm);
        EBUS_EVENT(Debug::ReplicaDrillerBus, OnDeactivateReplica, this);

        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                {
                    GM_PROFILE_USER_CALLBACK("OnReplicaDeactivate");
                    chunk->OnReplicaDeactivate(rc);
                }
                EBUS_EVENT(Debug::ReplicaDrillerBus, OnDeactivateReplicaChunk, chunk.get());
            }
        }
    }
    //-----------------------------------------------------------------------------
    void Replica::OnChangeOwnership(const ReplicaContext& rc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                GM_PROFILE_USER_CALLBACK("OnReplicaChangeOwnership");
                chunk->OnReplicaChangeOwnership(rc);
            }
        }
    }
    //-----------------------------------------------------------------------------
    void Replica::RequestChangeOwnership(PeerId newOwner)
    {
        if (newOwner == InvalidReplicaPeerId)
        {
            newOwner = m_manager->GetLocalPeerId();
        }
        AZStd::static_pointer_cast<ReplicaStatus>(m_replicaStatus)->RequestOwnership(newOwner);
    }
    //-----------------------------------------------------------------------------
    bool Replica::RequestOwnershipFn(PeerId requestor, const RpcContext& rpcContext)
    {
        (void) rpcContext;

        AZ_PROFILE_FUNCTION(GridMate);

        if (IsActive())
        {
            if (IsPrimary())
            {
                EBUS_EVENT(Debug::ReplicaDrillerBus, OnRequestReplicaChangeOwnership, this, requestor);

                if (IsMigratable() && requestor != m_manager->GetLocalPeerId())
                {
                    bool accepted;
                    {
                        GM_PROFILE_USER_CALLBACK("AcceptChangeOwnership");
                        accepted = AcceptChangeOwnership(requestor, GetMyContext());
                    }
                    if (accepted)
                    {
                        m_manager->MigrateReplica(this, requestor);
                    }
                }
            }
        }
        return false;
    }
    //-----------------------------------------------------------------------------
    bool Replica::MigrationSuspendUpstreamFn(PeerId ownerId, AZ::u32 requestTime, const RpcContext& rpcContext)
    {
        (void) rpcContext;
        if (IsProxy())
        {
            //AZ_TracePrintf("GridMate", "Received upstream suspend ack requested at %u for 0x%x from 0x%x.\n", requestTime, GetRepId(), ownerId);
            m_manager->AckUpstreamSuspended(GetRepId(), ownerId, requestTime);
        }
        else
        {
            //AZ_TracePrintf("GridMate", "Sending upstream suspend ack requested at %u for 0x%x.\n", requestTime, GetRepId(), ownerId);
        }
        return true;
    }
    //-----------------------------------------------------------------------------
    bool Replica::MigrationRequestDownstreamAckFn(PeerId ownerId, AZ::u32 requestTime, const RpcContext& rpcContext)
    {
        (void) rpcContext;
        if (IsProxy())
        {
            //AZ_TracePrintf("GridMate", "Received downstream ack requested at %u for 0x%x from 0x%x .\n", requestTime, GetRepId(), ownerId);
            m_manager->AckDownstream(GetRepId(), ownerId, requestTime);
        }
        else
        {
            m_flags |= Rep_SuspendDownstream;
            //AZ_TracePrintf("GridMate", "Sending downstream ack request at %u for 0x%x from 0x%x .\n", requestTime, GetRepId(), ownerId);
        }
        return true;
    }
    //-----------------------------------------------------------------------------
    void Replica::InitReplica(ReplicaManager* manager)
    {
        m_manager = manager;
    }
    //-----------------------------------------------------------------------------
    void Replica::Activate(const ReplicaContext& rc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        // Resolve whether we're migratable or not from the chunks
        // present when we're attached to the network.
        // If there are no chunks (excluding the system chunk) then
        // we can't migrate, as the destination wont know what
        // to do with an empty replica.
        if (!m_chunks.empty())
        {
            bool migratable = true;
            for (auto chunk : m_chunks)
            {
                if (chunk && !chunk->IsReplicaMigratable())
                {
                    migratable = false;
                    break;
                }
            }
            SetMigratable(migratable);
        }

        m_flags |= Rep_Active;

        OnActivate(rc);
    }
    //-----------------------------------------------------------------------------
    void Replica::Deactivate(const ReplicaContext& rc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        if (IsActive())
        {
            OnDeactivate(rc);
            m_flags &= ~Rep_Active;
            m_manager->CancelTasks(this);
        }
        m_manager = nullptr;
    }
    //-----------------------------------------------------------------------------
    void Replica::SetSyncStage(bool b)
    {
        AZ_Assert(!IsActive(), "Synchronization category can only be set before a replica is registered!");
        m_flags = b ? m_flags | Rep_SyncStage : m_flags & ~Rep_SyncStage;
    }
    //-----------------------------------------------------------------------------
    void Replica::SetMigratable(bool migratable)
    {
        AZ_Assert(!IsActive(), "Migration capabilities can only be set before a replica is registered!");
        m_flags = migratable ? m_flags | Rep_CanMigrate : m_flags & ~Rep_CanMigrate;
    }
    //-----------------------------------------------------------------------------
    bool Replica::IsSuspendDownstream() const
    {
        return !!(m_flags & Rep_SuspendDownstream);
    }
    //-----------------------------------------------------------------------------
    bool Replica::ProcessRPCs(const ReplicaContext& rc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        bool isProcessed = true;
        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                isProcessed &= chunk->ProcessRPCs(rc);
            }
        }

        if (!isProcessed) // have some rpcs left that might require forwarding to other peers so marking replica dirty for next marshaling
        {
            rc.m_rm->OnReplicaChanged(this);
        }

        return isProcessed;
    }
    //-----------------------------------------------------------------------------
    void Replica::ClearPendingRPCs()
    {
        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                chunk->ClearPendingRPCs();
            }
        }
    }
    //-----------------------------------------------------------------------------
    void Replica::OnReplicaPriorityUpdated(ReplicaChunkBase* modifiedChunk)
    {
        (void)modifiedChunk;
        ReplicaPriority maxRepPri = 0;
        for (const auto& chunk : m_chunks)
        {
            if (chunk)
            {
                maxRepPri = AZStd::GetMax(maxRepPri, chunk->GetPriority());
            }
        }

        m_priority = maxRepPri;

        if (m_manager)
        {
            m_manager->OnReplicaPriorityUpdated(this);
        }
    }
    //-----------------------------------------------------------------------------
    void Replica::MarkRPCsAsRelayed()
    {
        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                chunk->MarkRPCsAsRelayed();
            }
        }
    }
    //-----------------------------------------------------------------------------
    void Replica::SetRepId(ReplicaId id)
    {
        m_myId = id;
    }
    //-----------------------------------------------------------------------------
    PrepareDataResult Replica::PrepareData(EndianType endianType, AZ::u32 marshalFlags)
    {
        //AZ_PROFILE_SCOPE("GridMate");

        PrepareDataResult pdr(false, false, false, false);
        bool dataSetChange = false;
        for (auto chunk : m_chunks)
        {
            if (chunk)
            {
                PrepareDataResult chunkPDR = chunk->PrepareData(endianType, marshalFlags);

                pdr.m_isDownstreamReliableDirty |= chunkPDR.m_isDownstreamReliableDirty;
                pdr.m_isDownstreamUnreliableDirty |= chunkPDR.m_isDownstreamUnreliableDirty;
                pdr.m_isUpstreamReliableDirty |= chunkPDR.m_isUpstreamReliableDirty;
                pdr.m_isUpstreamUnreliableDirty |= chunkPDR.m_isUpstreamUnreliableDirty;
                dataSetChange |= chunk->m_reliableDirtyBits.any() | chunk->m_unreliableDirtyBits.any();
            }
        }

        if(dataSetChange)
        {
            m_revision++;    //If any chunk's dataset changed increase the replica revision.
        }

        return pdr;
    }
    //-----------------------------------------------------------------------------
    void Replica::Marshal(MarshalContext& mc)
    {
        //AZ_PROFILE_SCOPE("GridMate");

        // We are going to replace the outBuffer with a temporary chunk buffer for each chunk,
        // hold on to the original so we can restore it later and write the chunk buffers into
        WriteBuffer* outBuffer = mc.m_outBuffer;

        mc.m_outBuffer = nullptr;

        AZStd::bitset<GM_MAX_CHUNKS_PER_REPLICA> chunkManifest;

        struct ChunkInfo
        {
            ChunkInfo(EndianType endianness)
                : m_length(endianness)
                , m_payload(endianness, 0)
            {
            }

            WriteBufferStatic<5> m_length;  // length will never need more than 5 bytes.
            WriteBufferDynamic m_payload;
        };

        PackedSize payloadLen = 0;
        AZStd::fixed_vector<ChunkInfo, GM_MAX_CHUNKS_PER_REPLICA> chunkBuffers;
        for (size_t iChunk = 0; iChunk < m_chunks.size(); ++iChunk)
        {
            ReplicaChunkPtr chunk = m_chunks[iChunk];

            if (!chunk)
            {
                continue;
            }

            if (!(mc.m_marshalFlags & ReplicaMarshalFlags::ForceDirty)
                && !chunk->IsDirty(mc.m_marshalFlags)
                && !(ReplicaTarget::IsAckEnabled() && (mc.m_peerLatestVersionAckd < chunk->m_revision )) )
            {
                /*
                 * New operation such as NewProxy are optimized to send chunks that are not currently dirty but
                 * have values are no longer the default constructor values.
                 */
                if ((mc.m_marshalFlags & ReplicaMarshalFlags::NewProxy) != ReplicaMarshalFlags::NewProxy)
                {
                    continue;
                }
            }

            if (!chunk->ShouldSendToPeer(mc.m_peer))
            {
                continue;
            }

            // Add the chunk to the manifest and prepare its buffer
            chunkManifest.set(iChunk);
            chunkBuffers.push_back(ChunkInfo(outBuffer->GetEndianType()));
            ChunkInfo& chunkInfo = chunkBuffers.back();
            chunkInfo.m_payload.Init(128);
            mc.m_outBuffer = &chunkInfo.m_payload;

            EBUS_EVENT(Debug::ReplicaDrillerBus, OnSendReplicaChunkBegin, chunk.get(), static_cast<AZ::u32>(iChunk), mc.m_rm->GetLocalPeerId(), mc.m_peer->GetId());
            PackedSize writeOffset = mc.m_outBuffer->GetExactSize();
            // Write the ctor data if we need to
            if (mc.m_marshalFlags & ReplicaMarshalFlags::IncludeCtorData)
            {
                mc.m_outBuffer->Write(chunk->GetDescriptor()->GetChunkTypeId());
                chunk->GetDescriptor()->MarshalCtorData(chunk.get(), *mc.m_outBuffer);
            }

            // Marshal the chunk data
            chunk->Marshal(mc, static_cast<AZ::u32>(iChunk));
            EBUS_EVENT(Debug::ReplicaDrillerBus, OnSendReplicaChunkEnd, chunk.get(), static_cast<AZ::u32>(iChunk), mc.m_outBuffer->Get() + writeOffset.GetBytes(), mc.m_outBuffer->Size() - writeOffset.GetBytes());

            // Precompute the chunk payload length and add to overall replica payload length
            PackedSize chunkLen = chunkInfo.m_payload.GetExactSize();
            chunkInfo.m_length.Write(chunkLen);

            payloadLen += chunkLen + chunkInfo.m_length.GetExactSize();
        }

        if (!chunkBuffers.empty())
        {
            mc.m_outBuffer = outBuffer;
            mc.m_outBuffer->Write(GetRepId());

            WriteBufferStatic<VlqU64Marshaler::MaxEncodingBytes> chunkManifestBuffer(mc.m_outBuffer->GetEndianType());
            chunkManifestBuffer.Write(chunkManifest.to_ullong(), VlqU64Marshaler());

            payloadLen += chunkManifestBuffer.Size();

            mc.m_outBuffer->Write(payloadLen);

            mc.m_outBuffer->WriteRaw(chunkManifestBuffer.Get(), chunkManifestBuffer.Size());

            for (ChunkInfo& chunkInfo : chunkBuffers)
            {
                mc.m_outBuffer->WriteRaw(chunkInfo.m_length.Get(), chunkInfo.m_length.GetExactSize());
                mc.m_outBuffer->WriteRaw(chunkInfo.m_payload.Get(), chunkInfo.m_payload.GetExactSize());
            }
        }
    }
    //-----------------------------------------------------------------------------
    bool Replica::Unmarshal(UnmarshalContext& mc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        UnmarshalContext chunkContext(mc);
        ReadBuffer& buffer = *mc.m_iBuf;

        // Add new chunks or update existing ones
        AZStd::bitset<GM_MAX_CHUNKS_PER_REPLICA> chunkManifest;
        if (buffer.Read(*reinterpret_cast<AZ::u64*>(chunkManifest.data()), VlqU64Marshaler()))
        {
            for (size_t iChunk = 0; iChunk < GM_MAX_CHUNKS_PER_REPLICA && chunkManifest.any(); ++iChunk)
            {
                if (chunkManifest.test(iChunk))
                {
                    chunkManifest.reset(iChunk);

                    PackedSize chunkSize;
                    if (!buffer.Read(chunkSize))
                    {
                        return false;
                    }

                    // Generate a buffer bound to the size of the chunk
                    ReadBuffer innerBuffer = buffer.ReadInnerBuffer(chunkSize);
                    if (innerBuffer.IsValid())
                    {
                        chunkContext.m_iBuf = &innerBuffer;
                    }
                    else
                    {
                        AZ_Warning("GridMate", false, "We're going to read too much data to unmarshal properly");
                        return false;
                    }

                    ReplicaChunkPtr chunk = m_chunks.size() > iChunk ? m_chunks[iChunk] : nullptr;
                    if (mc.m_hasCtorData)
                    {
                        ReplicaChunkClassId repChunkClassId;
                        if (!innerBuffer.Read(repChunkClassId))
                        {
                            return false;
                        }

                        if (!chunk)
                        {
                            chunk = CreateReplicaChunkFromStream(repChunkClassId, chunkContext);
                            AZ_Warning("GridMate", chunk, "Received unknown replica chunk type 0x%x at index %d, discarding %d bytes and %d bits.",
                                repChunkClassId, static_cast<int>(iChunk), static_cast<int>(innerBuffer.Left().GetBytes()),
                                static_cast<int>(innerBuffer.Left().GetAdditionalBits()));
                        }
                        else
                        {
                            chunk->GetDescriptor()->DiscardCtorStream(chunkContext);
                        }
                    }

                    if (chunk)
                    {
                        EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveReplicaChunkBegin, chunk.get(), static_cast<AZ::u32>(iChunk), chunkContext.m_peer->GetId(), chunkContext.m_rm->GetLocalPeerId(), innerBuffer.Get(), chunkSize.GetSizeInBytesRoundUp());
                        chunk->Unmarshal(chunkContext, static_cast<AZ::u32>(iChunk));
                        EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveReplicaChunkEnd, chunk.get(), static_cast<AZ::u32>(iChunk));
                    }
                    else
                    {
                        innerBuffer.Skip(innerBuffer.Left());
                    }

                    AZ_Warning("GridMate", innerBuffer.IsEmpty() && !innerBuffer.IsOverrun(), "Incorrect number of bytes read while unmarshaling chunk index %d, replica 0x%x. Data may be corrupted!", static_cast<int>(iChunk), GetRepId());
                    innerBuffer.Skip(innerBuffer.Left());
                }
            }
        }
        return true;
    }
    //-----------------------------------------------------------------------------
    ReplicaChunkPtr Replica::CreateReplicaChunkFromStream(ReplicaChunkClassId classId, UnmarshalContext& mc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        ReplicaChunkPtr chunk = nullptr;
        ReplicaChunkDescriptor* pDesc = ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(classId);
        if (pDesc)
        {
            ReplicaChunkDescriptorTable::Get().BeginConstructReplicaChunk(pDesc);
            chunk = pDesc->CreateFromStream(mc);
            ReplicaChunkDescriptorTable::Get().EndConstructReplicaChunk();

            // Push back even if the chunk did not create so the proper indexes are maintained.
            m_chunks.push_back(chunk);

            if (chunk)
            {
                chunk->Init(pDesc);
                chunk->AttachedToReplica(this);
                AZ_Assert(chunk->GetReplica() == this, "Must be bound to the same replica");
            }
        }
        return chunk;
    }
    //-----------------------------------------------------------------------------
    bool Replica::IsUpdateFromReplicaEnabled() const
    {
        for (const auto& chunk : m_chunks)
        {
            if (chunk && !chunk->IsUpdateFromReplicaEnabled())
            {
                return false;
            }
        }

        return true;
    }
    //-----------------------------------------------------------------------------
    ReplicaChunkPtr Replica::GetChunkByIndex(size_t index)
    {
        return m_chunks[index];
    }
    //-----------------------------------------------------------------------------
    bool Replica::IsBroadcast() const
    {
        for (const auto& chunk : m_chunks)
        {
            if (chunk && chunk->IsBroadcast())
            {
                return true;
            }
        }

        return false;
    }

    //-----------------------------------------------------------------------------
    // CtorContextBase
    //-----------------------------------------------------------------------------
    CtorContextBase::CtorDataSetBase::CtorDataSetBase()
    {
        CtorContextBase::s_pCur->m_members.push_back(this);
    }
    //-----------------------------------------------------------------------------
    CtorContextBase* CtorContextBase::s_pCur = nullptr;
    //-----------------------------------------------------------------------------
    CtorContextBase::CtorContextBase()
    {
        s_pCur = this;
    }
    //-----------------------------------------------------------------------------
    void CtorContextBase::Marshal(WriteBuffer& wb)
    {
        for (MembersArrayType::iterator i = m_members.begin(); i != m_members.end(); ++i)
        {
            (*i)->Marshal(wb);
        }
    }
    //-----------------------------------------------------------------------------
    void CtorContextBase::Unmarshal(ReadBuffer& rb)
    {
        for (MembersArrayType::iterator i = m_members.begin(); i != m_members.end(); ++i)
        {
            (*i)->Unmarshal(rb);
        }
    }
    //-----------------------------------------------------------------------------
} // namespace GridMate
