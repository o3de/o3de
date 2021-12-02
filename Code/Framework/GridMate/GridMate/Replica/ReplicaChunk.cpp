/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>

#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/MigrationSequence.h>
#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/ReplicaUtils.h>
#include <GridMate/Replica/ReplicaDrillerEvents.h>
#include <GridMate/Serialize/CompressionMarshal.h>

namespace GridMate
{
    //-----------------------------------------------------------------------------
    ReplicaChunkBase::ReplicaChunkBase()
        : m_refCount(0)
        , m_replica(nullptr)
        , m_descriptor(nullptr)
        , m_flags(0)
        , m_handler(nullptr)
        , m_reliableDirtyBits()
        , m_unreliableDirtyBits()
        , m_nonDefaultValueBits()
        , m_nDownstreamReliableRPCs(0)
        , m_nDownstreamUnreliableRPCs(0)
        , m_nUpstreamReliableRPCs(0)
        , m_nUpstreamUnreliableRPCs(0)
        , m_dirtiedDataSets(0xFFFFFFFF)
        , m_priority(k_replicaPriorityNormal)
        , m_revision(1)
    {
        ReplicaChunkInitContext* initContext = ReplicaChunkDescriptorTable::Get().GetCurrentReplicaChunkInitContext();
        AZ_Assert(initContext, "Replica's descriptor is NOT pushed on the stack! Call Replica::Desriptor::Push() before construction!");
        initContext->m_chunk = this;
        EBUS_EVENT(Debug::ReplicaDrillerBus, OnCreateReplicaChunk, this);
    }
    //-----------------------------------------------------------------------------
    ReplicaChunkBase::~ReplicaChunkBase()
    {
        AZ_Assert(m_refCount == 0, "Attempting to free replica with non-zero refCount(%d)!", m_refCount);
        EBUS_EVENT(Debug::ReplicaDrillerBus, OnDestroyReplicaChunk, this);
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::Init(ReplicaChunkClassId chunkTypeId)
    {
        ReplicaChunkDescriptor* descriptor = ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(chunkTypeId);
        AZ_Assert(descriptor, "Init failed. Can't find replica chunk descriptor for chunk type 0x%x!", static_cast<AZ::u32>(chunkTypeId));
        Init(descriptor);
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::Init(ReplicaChunkDescriptor* descriptor)
    {
        AZ_Assert(descriptor, "Init failed. descriptor can't be null!");
        AZ_Assert(descriptor->IsInitialized(), "Init failed. Replica chunk descriptor for chunk type 0x%x has not been properly initialized!", static_cast<AZ::u32>(descriptor->GetChunkTypeId()));
        m_descriptor = descriptor;
        for (size_t iDataSet = 0; iDataSet < descriptor->GetDataSetCount(); ++iDataSet)
        {
            descriptor->GetDataSet(this, iDataSet)->m_replicaChunk = this;
        }
        for (size_t iRpc = 0; iRpc < descriptor->GetRpcCount(); ++iRpc)
        {
            descriptor->GetRpc(this, iRpc)->m_replicaChunk = this;
        }
    }
    //-----------------------------------------------------------------------------
    bool ReplicaChunkBase::IsClassType(ReplicaChunkClassId classId) const
    {
        return classId == GetDescriptor()->GetChunkTypeId();
    }
    //-----------------------------------------------------------------------------
    ReplicaId ReplicaChunkBase::GetReplicaId() const
    {
        if (m_replica)
        {
            return m_replica->GetRepId();
        }
        return InvalidReplicaId;
    }
    //-----------------------------------------------------------------------------
    PeerId ReplicaChunkBase::GetPeerId() const
    {
        PeerId peerId = InvalidReplicaPeerId;

        if (m_replica != nullptr)
        {
            ReplicaContext context(m_replica->GetMyContext());

            if (context.m_peer != nullptr)
            {
                peerId = context.m_peer->GetId();
            }
        }

        return peerId;
    }
    //-----------------------------------------------------------------------------
    ReplicaManager* ReplicaChunkBase::GetReplicaManager()
    {
        if (m_replica)
        {
            return m_replica->GetReplicaManager();
        }
        return nullptr;
    }
    //-----------------------------------------------------------------------------
    bool ReplicaChunkBase::IsActive() const
    {
        if (m_replica)
        {
            return m_replica->IsActive();
        }
        return false;
    }
    //-----------------------------------------------------------------------------
    bool ReplicaChunkBase::IsPrimary() const
    {
        if (m_replica)
        {
            return m_replica->IsPrimary();
        }
        return true;
    }
    //-----------------------------------------------------------------------------
    bool ReplicaChunkBase::IsProxy() const
    {
        return !IsPrimary();
    }
    //-----------------------------------------------------------------------------
    bool ReplicaChunkBase::IsDirty(AZ::u32 marshalFlags) const
    {
        if (marshalFlags & ReplicaMarshalFlags::IncludeDatasets)
        {
            const auto& dirtyBits = (marshalFlags& ReplicaMarshalFlags::Reliable) ? m_reliableDirtyBits : m_unreliableDirtyBits;
            if (dirtyBits.any())
            {
                return true;
            }
        }

        // Always send RPCs, no need for a flag
        return !m_rpcQueue.empty();
    }
    //-----------------------------------------------------------------------------
    PrepareDataResult ReplicaChunkBase::PrepareData(EndianType endianType, AZ::u32 marshalFlags)
    {
        //AZ_PROFILE_SCOPE("GridMate");

        PrepareDataResult pdr(false, false, false, false);
        bool forceDatasetsReliable = !!(marshalFlags & ReplicaMarshalFlags::ForceReliable);

        m_nDownstreamReliableRPCs = m_nDownstreamUnreliableRPCs = m_nUpstreamReliableRPCs = m_nUpstreamUnreliableRPCs = 0;

        // RPCs
        for (auto i = m_rpcQueue.rbegin(); i != m_rpcQueue.rend(); ++i)
        {
            Internal::RpcRequest* rpc = *i;
            if (!rpc->m_relayed)
            {
                bool isDownstream = rpc->m_authoritative;

                // If there were reliable rpcs in queue -> keep all preceding rpcs reliable to guarantee the right order of execution
                if ((pdr.m_isDownstreamReliableDirty && isDownstream) || (pdr.m_isUpstreamReliableDirty && !isDownstream))
                {
                    rpc->m_reliable = true;
                }

                pdr.m_isDownstreamReliableDirty |= isDownstream && rpc->m_reliable;
                pdr.m_isDownstreamUnreliableDirty |= isDownstream && !rpc->m_reliable;
                pdr.m_isUpstreamReliableDirty |= !isDownstream && rpc->m_reliable;
                pdr.m_isUpstreamUnreliableDirty |= !isDownstream && !rpc->m_reliable;

                m_nDownstreamReliableRPCs += isDownstream && rpc->m_reliable ? 1 : 0;
                m_nDownstreamUnreliableRPCs += isDownstream && !rpc->m_reliable ? 1 : 0;
                m_nUpstreamReliableRPCs += !isDownstream && rpc->m_reliable ? 1 : 0;
                m_nUpstreamUnreliableRPCs += !isDownstream && !rpc->m_reliable ? 1 : 0;

                // Force all datasets to be sent reliably if there are post-attached rpcs
                // This guarantees correct state when post-attached rpcs arrive
                forceDatasetsReliable |= isDownstream && rpc->m_rpc->IsPostAttached();
            }
        }

        // DataSets
        AZStd::bitset<GM_MAX_DATASETS_IN_CHUNK> dirtyDataSets;
        ReplicaChunkDescriptor* descriptor = GetDescriptor();
        for (size_t i = 0; i < descriptor->GetDataSetCount(); ++i)
        {
            DataSetBase* dataSet = descriptor->GetDataSet(this, i);
            PrepareDataResult pdrDs = dataSet->PrepareData(endianType, marshalFlags);
            dirtyDataSets.set(i, pdrDs.m_isDownstreamReliableDirty | pdrDs.m_isDownstreamUnreliableDirty);
            forceDatasetsReliable |= pdrDs.m_isDownstreamReliableDirty;

            if (!dataSet->IsDefaultValue())
            {
                /*
                * Mark this dataset as having a non-default value.
                * Note: default bits are never reset unlike dirty bits.
                */
                m_nonDefaultValueBits.set(i);
            }
        }

        m_reliableDirtyBits.reset();
        m_unreliableDirtyBits.reset();
        if (dirtyDataSets.any())
        {
            if (forceDatasetsReliable)
            {
                pdr.m_isDownstreamReliableDirty = true;
                m_reliableDirtyBits = dirtyDataSets;
            }
            else
            {
                pdr.m_isDownstreamUnreliableDirty = true;
                m_unreliableDirtyBits = dirtyDataSets;
            }
        }

        // If we know that the next data set send will be reliable,
        // notify the datasets so they can reset their dirty state.
        if (forceDatasetsReliable || m_replica->IsNew() || m_replica->IsNewOwner())
        {
            for (size_t i = 0; i < descriptor->GetDataSetCount(); ++i)
            {
                descriptor->GetDataSet(this, i)->ResetDirty();
            }
        }

        return pdr;
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::SetPriority(ReplicaPriority priority)
    {
        m_priority = priority;

        if (m_replica)
        {
            m_replica->OnReplicaPriorityUpdated(this);
        }
    }
    //-----------------------------------------------------------------------------
    bool ReplicaChunkBase::ShouldSendToPeer(ReplicaPeer* peer) const
    {
        //AZ_PROFILE_SCOPE("GridMate");

        // Only send chunks to the same zone as the peer
        return !!(peer->GetZoneMask() & GetDescriptor()->GetZoneMask());
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::Marshal(MarshalContext& mc, AZ::u32 chunkIndex)
    {
        //AZ_PROFILE_SCOPE("GridMate");

        SafeGuardWrite(mc.m_outBuffer, [this, &mc, &chunkIndex]()
        {
            MarshalDataSets(mc, chunkIndex);
            MarshalRpcs(mc, chunkIndex);
        });
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::Unmarshal(UnmarshalContext& mc, AZ::u32 chunkIndex)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        SafeGuardRead(mc.m_iBuf, [this, &mc, &chunkIndex]()
        {
            UnmarshalDataSets(mc, chunkIndex);
            UnmarshalRpcs(mc, chunkIndex);
            m_flags |= RepChunk_Updated;
        });
    }

    bool ReplicaChunkBase::ShouldBindToNetwork()
    {
        return GetReplica() && GetReplica()->IsActive();
    }

    //-----------------------------------------------------------------------------
    AZ::u32 ReplicaChunkBase::CalculateDirtyDataSetMask(MarshalContext& mc)
    {
        AZ::u32 dataSetMask = 0;

        if ((mc.m_marshalFlags & ReplicaMarshalFlags::ForceDirty))
        {
            // Set all the dataset bits manually because AZStd::bitset doesn't have a ranged set.
            dataSetMask = static_cast<AZ::u32>((static_cast<AZ::u64>(1) << GetDescriptor()->GetDataSetCount()) - 1);
        }
        else if ((mc.m_marshalFlags & ReplicaMarshalFlags::OmitUnmodified))
        {
            // Send all bits that have ever been modified.
            dataSetMask = m_nonDefaultValueBits.to_ulong();
        }
        else if ((mc.m_marshalFlags & ReplicaMarshalFlags::IncludeDatasets))
        {
            if (mc.m_marshalFlags & ReplicaMarshalFlags::Reliable)
            {
                dataSetMask = (*m_reliableDirtyBits.data());
        }
            else
            {
                dataSetMask = (*m_unreliableDirtyBits.data());
                //Handle additional unAck'd for specific peer
                if (ReplicaTarget::IsAckEnabled()
                    && mc.m_peerLatestVersionAckd != 0)
                {
                    AZStd::bitset<GM_MAX_DATASETS_IN_CHUNK> dirtyDataSets;
                    ReplicaChunkDescriptor* descriptor = GetDescriptor();
                    for (size_t i = 0; i < descriptor->GetDataSetCount(); ++i)
                    {
                        const DataSetBase* dataSet = descriptor->GetDataSet(this, i);
                        auto rev = dataSet->GetRevision();
                        const bool isOld = rev > mc.m_peerLatestVersionAckd;
                        if (isOld)
                        {
                            dirtyDataSets.set(i, isOld);
                        }
                    }
                    dataSetMask |= *dirtyDataSets.data();   //Add additional un-ack'd data sets for this target
                }
            }
        }

        return dataSetMask;
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::MarshalDataSets(MarshalContext& mc, AZ::u32 chunkIndex)
    {
        //AZ_PROFILE_SCOPE("GridMate");
        AZ::u32 dirtyDataSetMask = CalculateDirtyDataSetMask(mc);
        AZStd::bitset<GM_MAX_DATASETS_IN_CHUNK> changebits(dirtyDataSetMask);
        ReplicaChunkDescriptor* descriptor = GetDescriptor();
        bool wroteDataSet = false;
        mc.m_outBuffer->Write(changebits.to_ulong(), VlqU32Marshaler());
        if (dirtyDataSetMask == 0)
        {
            return;
        }
        for (size_t i = 0; i < descriptor->GetDataSetCount(); ++i)
        {
            if (changebits[i])
            {
                DataSetBase* dataset = descriptor->GetDataSet(this, i);
                if (!dataset)
                {
                    AZ_Assert(false, "How can we have a dirty dataset that doesn't exist?");
                    continue;
                }

                ReadBuffer data = dataset->GetMarshalData();
                mc.m_outBuffer->WriteRaw(data.Get(), data.Size());
                wroteDataSet = true;

                EBUS_EVENT(Debug::ReplicaDrillerBus, OnSendDataSet,
                    this,
                    chunkIndex,
                    dataset,
                    mc.m_rm->GetLocalPeerId(),
                    mc.m_peer->GetId(),
                    data.Get(),
                    data.Size().GetSizeInBytesRoundUp());
            }
        }
        if(wroteDataSet)
        {
            //Add callback here
            CallbackBuffer* callbackBuffer = mc.m_callbackBuffer;
            if(mc.m_target && callbackBuffer && ReplicaTarget::IsAckEnabled())
            {
                callbackBuffer->push_back(mc.m_target->CreateCallback(m_replica->m_revision));
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::UnmarshalDataSets(UnmarshalContext& mc, AZ::u32 chunkIndex)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        AZStd::bitset<GM_MAX_DATASETS_IN_CHUNK> changebits;
        if (!mc.m_iBuf->Read(*changebits.data(), VlqU32Marshaler()))
        {
            return;
        }

        if (changebits.any())
        {
            if (mc.m_peer != m_replica->m_upstreamHop)
            {
                AZ_TracePrintf("GridMate", "Received dataset updates for replica id %08x(%s) from unexpected peer.", GetReplicaId(), IsActive() && IsPrimary() ? "primary" : "proxy");
                if (IsPrimary())
                {
                    mc.m_iBuf->Skip(mc.m_iBuf->Left());
                    return;
                }
            }
        }

        ReplicaChunkDescriptor* descriptor = GetDescriptor();
        for (size_t i = 0; i < descriptor->GetDataSetCount(); ++i)
        {
            if (changebits[i])
            {
                DataSetBase* dataset = descriptor->GetDataSet(this, i);
                if (!dataset)
                {
                    continue;
                }

                /*
                 * Whenever we get a dataset from the network, we assume it was modified and thus
                 * no longer has the default value.
                 */
                dataset->MarkAsNonDefaultValue();
                m_nonDefaultValueBits.set(i);

                const char* readPtr = mc.m_iBuf->GetCurrent();
                dataset->Unmarshal(mc);

                EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveDataSet,
                    this,
                    chunkIndex,
                    dataset,
                    mc.m_peer->GetId(),
                    mc.m_rm->GetLocalPeerId(),
                    readPtr,
                    mc.m_iBuf->GetCurrent() - readPtr);
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::MarshalRpcs(MarshalContext& mc, AZ::u32 chunkIndex)
    {
        //AZ_PROFILE_SCOPE("GridMate");

        bool isAuthoritative = (mc.m_marshalFlags & ReplicaMarshalFlags::Authoritative) == ReplicaMarshalFlags::Authoritative;
        bool isReliable = (mc.m_marshalFlags & ReplicaMarshalFlags::Reliable) == ReplicaMarshalFlags::Reliable;

        AZ::u32 downstreamReliableToSend = isAuthoritative && (isReliable || (mc.m_marshalFlags & ReplicaMarshalFlags::FullSync) == ReplicaMarshalFlags::FullSync) ? m_nDownstreamReliableRPCs : 0;
        AZ::u32 downstreamUnreliableToSend = isAuthoritative && (!isReliable || (mc.m_marshalFlags & ReplicaMarshalFlags::FullSync) == ReplicaMarshalFlags::FullSync) ? m_nDownstreamUnreliableRPCs : 0;
        AZ::u32 upstreamReliableToSend = !isAuthoritative && isReliable ? m_nUpstreamReliableRPCs : 0;
        AZ::u32 upstreamUnreliableToSend = !isAuthoritative && !isReliable ? m_nUpstreamUnreliableRPCs : 0;

        AZ::u32 rpcCount = downstreamReliableToSend + downstreamUnreliableToSend + upstreamReliableToSend + upstreamUnreliableToSend;
        AZ_Assert(rpcCount < GM_MAX_RPC_SEND_PER_REPLICA, "Attempting to send too many RPCs");
        mc.m_outBuffer->Write(rpcCount, VlqU32Marshaler());

        AZ::u32 rpcsSent = 0;
        for (Internal::RpcRequest* rpc : m_rpcQueue)
        {
            if (rpc->m_relayed || rpc->m_authoritative != isAuthoritative)
            {
                continue;
            }

            if (rpc->m_reliable != isReliable && (mc.m_marshalFlags & ReplicaMarshalFlags::ForceReliable) != ReplicaMarshalFlags::ForceReliable)
            {
                continue;
            }

            AZ::u8 rpcIndex = static_cast<AZ::u8>(GetDescriptor()->GetRpcIndex(this, rpc->m_rpc));

            auto bufferSize = mc.m_outBuffer->Size();

            SafeGuardWrite(mc.m_outBuffer, [rpc, rpcIndex, &mc]()
            {
                mc.m_outBuffer->Write(rpcIndex);
                rpc->m_rpc->Marshal(*mc.m_outBuffer, rpc);
            });

            EBUS_EVENT(Debug::ReplicaDrillerBus, OnSendRpc,
                this,
                chunkIndex,
                rpc,
                mc.m_rm->GetLocalPeerId(),
                mc.m_peer->GetId(),
                mc.m_outBuffer->Get() + bufferSize,
                mc.m_outBuffer->Size() - bufferSize);
            rpc->m_relayed = !(mc.m_marshalFlags & ReplicaMarshalFlags::Authoritative); // marking upstream rpcs relayed, for downstream rpcs - replicamgr marks them relayed after marshaling is finished
            rpcsSent++;
        }
        AZ_Assert(rpcsSent == rpcCount, "We did not write the expected number of rpcs! sent=%u, expected=%u.", rpcsSent, rpcCount);

        if (!m_rpcQueue.empty() && m_replica->GetReplicaManager())
        {
            m_replica->GetReplicaManager()->EnqueueUpdateTask(m_replica);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::UnmarshalRpcs(UnmarshalContext& mc, AZ::u32 chunkIndex)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        // Unmarshal RPCs
        AZ::u32 rpcCount;
        if (mc.m_iBuf->Read(rpcCount, VlqU32Marshaler()))
        {
            for (AZ::u32 rpcsRead = 0; rpcsRead < rpcCount; ++rpcsRead)
            {
                SafeGuardRead(mc.m_iBuf, [this, &mc, &chunkIndex]()
                {
                    unsigned char rpcIndex;
                    if (!mc.m_iBuf->Read(rpcIndex))
                    {
                        return;
                    }

                    RpcBase* rpc = GetDescriptor()->GetRpc(this, rpcIndex);
                    if (!rpc)
                    {
                        AZ_Assert(false, "Cannot find descriptor for rpcIndex %hhu!", rpcIndex);
                        return;
                    }

                    const char* dataPtr = mc.m_iBuf->GetCurrent();
                    Internal::RpcRequest* request = rpc->Unmarshal(*mc.m_iBuf);
                    if (!request)
                    {
                        AZ_Assert(false, "Failed to unmarshal RPC <%s>!", GetDescriptor()->GetRpcName(this, rpc));
                        return;
                    }

                    bool isRpcValid = true;
                    if (request->m_authoritative)
                    {
                        if (m_replica->m_upstreamHop != mc.m_peer)
                        {
                            AZ_Assert(false, "Discarding authoritative RPC <%s> from %p because it did not come from the expected upstream hop(%p)!", GetDescriptor()->GetRpcName(this, rpc), mc.m_peer, m_replica->m_upstreamHop);
                            isRpcValid = false;
                        }
                    }
                    else
                    {
                        if (!rpc->IsAllowNonAuthoritativeRequests())
                        {
                            AZ_Assert(false, "Discarding non-authoritative RPC <%s> because s_allowNonAuthoritativeRequests trait is disabled!", GetDescriptor()->GetRpcName(this, rpc));
                            isRpcValid = false;
                        }
                        if (!rpc->IsAllowNonAuthoritativeRequestsRelay() && !IsPrimary())
                        {
                            AZ_Assert(false, "Discarding non-authoritative RPC <%s> because s_allowNonAuthoritativeRequestRelay trait is disabled!", GetDescriptor()->GetRpcName(this, rpc));
                            isRpcValid = false;
                        }
                    }

                    if (isRpcValid)
                    {
                        if (mc.m_rm->GetSecurityOptions().m_enableStrictSourceValidation)
                        {
                            if (!mc.m_peer->IsSyncHost() && !(request->m_authoritative && m_replica->m_upstreamHop == mc.m_peer))
                            {
                                request->m_sourcePeer = mc.m_peer->GetId();
                            }
                        }

                        if (!request->m_sourcePeer)
                        {
                            request->m_sourcePeer = mc.m_peer->GetId();
                        }

                        size_t dataSize = mc.m_iBuf->GetCurrent() - dataPtr;
                        EBUS_EVENT(Debug::ReplicaDrillerBus, OnReceiveRpc,
                            this,
                            chunkIndex,
                            request,
                            mc.m_peer->GetId(),
                            mc.m_rm->GetLocalPeerId(),
                            dataPtr,
                            dataSize);
                        m_rpcQueue.push_back(request);
                    }
                    else
                    {
                        delete request;
                    }
                });
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::InternalUpdateChunk(const ReplicaContext& rc)
    {
        UpdateChunk(rc);
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::InternalUpdateFromChunk(const ReplicaContext& rc)
    {
        if (!(m_flags & ReplicaChunkBase::RepChunk_Updated))
        {
            return;
        }

        // Call events for any upstream modified datasets
        AZStd::bitset<GM_MAX_DATASETS_IN_CHUNK> eventbits = m_dirtiedDataSets;
        m_dirtiedDataSets = 0;
        m_flags &= ~RepChunk_Updated;
        ReplicaChunkDescriptor* descriptor = GetDescriptor();
        for (size_t i = 0; i < descriptor->GetDataSetCount(); ++i)
        {
            if (eventbits[i])
            {
                descriptor->GetDataSet(this, i)->DispatchChangedEvent(rc);
            }
        }

        UpdateFromChunk(rc);
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::AddDataSetEvent(DataSetBase* dataset)
    {
        m_dirtiedDataSets |= (1 << GetDescriptor()->GetDataSetIndex(this,dataset));
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::release()
    {
        AZ_Assert(m_refCount > 0, "Reference count logic error, trying to remove reference when refcount is 0");
        if (--m_refCount == 0)
        {
            GetDescriptor()->DeleteReplicaChunk(this);
        }
    }
    //-----------------------------------------------------------------------------
    bool ReplicaChunkBase::ProcessRPCs(const ReplicaContext& rc)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        // Process incoming RPCs
        for (RPCQueue::iterator iRPC = m_rpcQueue.begin(); iRPC != m_rpcQueue.end(); )
        {
            Internal::RpcRequest* request = *iRPC;
            bool isPrimary = IsPrimary(); // need to do this check after each RPC because ownership may change

            if (!m_replica->IsActive()) // this can happen if replica was deactivated within a previous RPC call
            {
                request->m_relayed = true;
            }
            else if (!request->m_processed && (isPrimary || request->m_authoritative))
            {
                request->m_realTime = rc.m_realTime;
                request->m_localTime = rc.m_localTime;
                bool ret = request->m_rpc->Invoke(request);
                request->m_processed = true;
                if (isPrimary)
                {
                    if (ret)
                    {
                        // Trickle back down to proxies
                        request->m_authoritative = true;
                    }
                    else
                    {
                        request->m_relayed = true;
                    }
                }
            }

            // This case can happen if the RPC we just invoked caused us to be removed
            if (m_rpcQueue.empty())
            {
                return true;
            }
            if (request->m_relayed)
            {
                iRPC = m_rpcQueue.erase(iRPC);
                delete request;
            }
            else
            {
                ++iRPC;
            }
        }

        return m_rpcQueue.empty();
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::ClearPendingRPCs()
    {
        while (!m_rpcQueue.empty())
        {
            Internal::RpcRequest* request = *m_rpcQueue.begin();
            delete request;
            m_rpcQueue.pop_front();
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::MarkRPCsAsRelayed()
    {
        for (auto rpc : m_rpcQueue)
        {
            if (rpc->m_authoritative)
            {
                rpc->m_relayed = true;
            }
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::QueueRPCRequest(GridMate::Internal::RpcRequest* rpc)
    {
        m_rpcQueue.push_back(rpc);

        if (m_replica && m_replica->GetReplicaManager())
        {
            m_replica->GetReplicaManager()->OnRPCQueued(m_replica);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::SignalDataSetChanged(const DataSetBase& dataset)
    {
        OnDataSetChanged(dataset);

        EnqueueMarshalTask();
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::OnDataSetChanged(const DataSetBase& dataSet)
    {
        (void)dataSet;
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::EnqueueMarshalTask()
    {
        if (m_replica && m_replica->GetReplicaManager())
        {
            m_replica->GetReplicaManager()->OnReplicaChanged(m_replica);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::AttachedToReplica(Replica* replica)
    {
        AZ_PROFILE_FUNCTION(GridMate);

        AZ_Assert(!m_replica, "Should not be attached to a replica");

        m_replica = replica;

        EBUS_EVENT(Debug::ReplicaDrillerBus, OnAttachReplicaChunk, this);
        {
            GM_PROFILE_USER_CALLBACK("OnAttachedToReplica");
            OnAttachedToReplica(replica);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkBase::DetachedFromReplica()
    {
        AZ_PROFILE_FUNCTION(GridMate);

        AZ_Assert(m_replica, "Should be attached to a replica");
        EBUS_EVENT(Debug::ReplicaDrillerBus, OnDetachReplicaChunk, this);
        {
            GM_PROFILE_USER_CALLBACK("OnDetachedFromReplica");
            OnDetachedFromReplica(m_replica);
        }

        m_replica = nullptr;
        ClearPendingRPCs();
    }
} // namespace GridMate
