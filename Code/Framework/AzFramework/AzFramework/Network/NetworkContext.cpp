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

#include <AzFramework/Network/NetworkContext.h>
#include <AzFramework/Network/NetBindable.h>
#include <GridMate/Replica/DataSet.h>


namespace AzFramework
{
    NetworkContext::DescBase::DescBase(const char* name, ptrdiff_t offset)
        : m_name(name)
        , m_offset(offset)
    {
    }

    NetworkContext::FieldDescBase::FieldDescBase(const char* name, ptrdiff_t offset)
        : DescBase(name, offset)
        , m_dataSetIdx(static_cast<size_t>(-1))
    {
    }

    NetworkContext::RpcDescBase::RpcDescBase(const char* name, ptrdiff_t offset)
        : DescBase(name, offset)
        , m_rpcIdx(static_cast<size_t>(-1))
    {
    }

    NetworkContext::CtorDataBase::CtorDataBase(const char* name)
        : m_name(name)
    {
    }

    NetworkContext::ClassBuilder::ClassBuilder(NetworkContext* context, ClassDescPtr binding)
        : m_binding(binding)
        , m_context(context)
    {
    }

    NetworkContext::ClassBuilder::~ClassBuilder()
    {
        if (m_context->IsRemovingReflection())
        {
            if (m_binding->UnregisterChunkType)
            {
                m_binding->UnregisterChunkType();
            }
        }
        else
        {
            if (m_binding->RegisterChunkType)
            {
                m_binding->RegisterChunkType();
            }
        }
    }

    NetworkContext::ClassDesc::ClassDesc(const char* name, const AZ::Uuid& typeId /* = AZ::Uuid() */)
        : m_name(name)
        , m_typeId(typeId)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    /// NetworkContext
    ///////////////////////////////////////////////////////////////////////////
    NetworkContext::NetworkContext()
    {
    }

    NetworkContext::~NetworkContext()
    {
    }

    size_t NetworkContext::GetReflectedChunkSize(const AZ::Uuid& typeId) const
    {
        size_t totalSize = 0;
        auto it = m_classBindings.find(typeId);
        if (it != m_classBindings.end())
        {
            ClassDescPtr binding = it->second;
            for (const auto& field : binding->m_chunkDesc.m_fields)
            {
                totalSize += field->GetDataSetSize();
            }

            for (const auto& rpc : binding->m_chunkDesc.m_rpcs)
            {
                totalSize += rpc->GetRpcSize();
            }
        }

        return totalSize;
    }

    bool NetworkContext::UsesSelfAsChunk(const AZ::Uuid& typeId) const
    {
        auto it = m_classBindings.find(typeId);
        if (it != m_classBindings.end())
        {
            ClassDescPtr binding = it->second;
            return !binding->m_chunkDesc.m_external && binding->m_chunkDesc.m_fields.size() > 0;
        }
        return false;
    }

    bool NetworkContext::UsesExternalChunk(const AZ::Uuid& typeId) const
    {
        auto it = m_classBindings.find(typeId);
        if (it != m_classBindings.end())
        {
            ClassDescPtr binding = it->second;
            return binding->m_chunkDesc.m_external && (AZ::u32(binding->m_chunkDesc.m_chunkId) != 0);
        }
        return false;
    }

    ReplicaChunkBase* NetworkContext::CreateReplicaChunk(const AZ::Uuid& typeId)
    {
        const auto it = m_classBindings.find(typeId);
        if (it != m_classBindings.end())
        {
            const ClassDescPtr binding = it->second;
            if (binding->CreateReplicaChunk)
            {
                ReplicaChunkDescriptor* descriptor = ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(binding->m_chunkDesc.m_chunkId);
                AZ_Assert(descriptor, "NetworkContext cannot find replica chunk descriptor for %s. Did you remember to register the chunk type?", binding->m_name);
                ReplicaChunkDescriptorTable::Get().BeginConstructReplicaChunk(descriptor);
                ReplicaChunkBase* chunk = binding->CreateReplicaChunk();
                ReplicaChunkDescriptorTable::Get().EndConstructReplicaChunk();
                chunk->Init(descriptor);
                return chunk;
            }
        }

        /*
         * Special case: empty declarations such as:
         *
         * static void Reflect() {
         *     ....
         *     NetworkContext->Class<MyComponent>();
         * }
         *
         * Result in no ReplicaChunks being created. It's treated as a no-op. No replication will be performed.
         */
        return nullptr;
    }

    void NetworkContext::DestroyReplicaChunk(ReplicaChunkBase* chunk)
    {
        ReplicaChunkClassId chunkId = chunk->GetDescriptor()->GetChunkTypeId();
        auto it = m_chunkBindings.find(chunkId);
        if (it != m_chunkBindings.end())
        {
            ClassDescPtr binding = it->second;
            binding->DestroyReplicaChunk(chunk);
            return;
        }

        AZ_Warning("NetworkContext", false, "DestroyReplicaChunk could not find a binding for %s", chunk->GetDescriptor()->GetChunkName());
    }

    void NetworkContext::Bind(NetBindable* instance, ReplicaChunkPtr chunk, NetworkContextBindMode mode)
    {
        const AZ::Uuid& typeId = instance->RTTI_GetType();
        auto it = m_classBindings.find(typeId);
        if (it != m_classBindings.end())
        {
            ClassDescPtr binding = it->second;
            if (chunk)
            {
                ReplicaChunkClassId chunkId = chunk->GetDescriptor()->GetChunkTypeId();
                AZ_Assert(binding->m_chunkDesc.m_chunkId == chunkId, "NetworkContext detected a type mismatch while trying to bind an instance to a ReplicaChunk");
                if (binding->m_chunkDesc.m_chunkId == chunkId)
                {
                    if (!binding->m_chunkDesc.m_external)
                    {
                        ReflectedReplicaChunkBase* refChunk = static_cast<ReflectedReplicaChunkBase*>(chunk.get());
                        refChunk->Bind(instance, mode);
                    }
                }
            }
            else
            {
                if (binding->BindRpcs)
                {
                    binding->BindRpcs(instance);
                }
            }
        }
    }

    void NetworkContext::EnumerateFields(const ReplicaChunkClassId& chunkId, FieldVisitor visitor) const
    {
        auto it = m_chunkBindings.find(chunkId);
        if (it != m_chunkBindings.end())
        {
            const ChunkDesc& chunkDesc = it->second->m_chunkDesc;
            for (const auto& field : chunkDesc.m_fields)
            {
                visitor(field.get());
            }
        }
    }

    void NetworkContext::EnumerateFields(const AZ::Uuid& typeId, FieldVisitor visitor) const
    {
        auto it = m_classBindings.find(typeId);
        if (it != m_classBindings.end())
        {
            const ChunkDesc& chunkDesc = it->second->m_chunkDesc;
            for (const auto& field : chunkDesc.m_fields)
            {
                visitor(field.get());
            }
        }
    }

    void NetworkContext::EnumerateRpcs(const ReplicaChunkClassId& chunkId, RpcVisitor visitor) const
    {
        auto it = m_chunkBindings.find(chunkId);
        if (it != m_chunkBindings.end())
        {
            const ChunkDesc& chunkDesc = it->second->m_chunkDesc;
            for (const auto& rpc : chunkDesc.m_rpcs)
            {
                visitor(rpc.get());
            }
        }
    }

    void NetworkContext::EnumerateRpcs(const AZ::Uuid& typeId, RpcVisitor visitor) const
    {
        auto it = m_classBindings.find(typeId);
        if (it != m_classBindings.end())
        {
            const ChunkDesc& chunkDesc = it->second->m_chunkDesc;
            for (const auto& rpc : chunkDesc.m_rpcs)
            {
                visitor(rpc.get());
            }
        }
    }

    void NetworkContext::EnumerateCtorData(const ReplicaChunkClassId& chunkId, CtorVisitor visitor) const
    {
        auto it = m_chunkBindings.find(chunkId);
        if (it != m_chunkBindings.end())
        {
            const ChunkDesc& chunkDesc = it->second->m_chunkDesc;
            for (const auto& ctor : chunkDesc.m_ctors)
            {
                visitor(ctor.get());
            }
        }
    }

    void NetworkContext::EnumerateCtorData(const AZ::Uuid& typeId, CtorVisitor visitor) const
    {
        auto it = m_classBindings.find(typeId);
        if (it != m_classBindings.end())
        {
            const ChunkDesc& chunkDesc = it->second->m_chunkDesc;
            for (const auto& ctor : chunkDesc.m_ctors)
            {
                visitor(ctor.get());
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    ReflectedReplicaChunkBase::ReflectedReplicaChunkBase()
        : m_ctorBuffer(GridMate::EndianType::IgnoreEndian, 0)
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    NetworkContextChunkDescriptor::NetworkContextChunkDescriptor(const char* name, size_t size, const AZ::Uuid& typeId)
        : ReplicaChunkDescriptor(name, size)
        , m_typeId(typeId)
    {
    }

    ReplicaChunkBase* NetworkContextChunkDescriptor::CreateFromStream(UnmarshalContext& ctx)
    {
        AZ_Assert(!m_typeId.IsNull(), "No typeid associated with NetworkContextChunkDescriptor, cannot spawn Chunk");
        if (!m_typeId.IsNull())
        {
            NetworkContext* netContext = nullptr;
            EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
            AZ_Assert(netContext, "No NetworkContext found while trying to construct ReflectedReplicaChunk");

            ReplicaChunkBase* replicaChunk = netContext->CreateReplicaChunk(m_typeId);
            if (ctx.m_hasCtorData && ctx.m_iBuf)
            {
                NetworkContextChunkDescriptor* netChunkDesc = static_cast<NetworkContextChunkDescriptor*>(replicaChunk->GetDescriptor());
                if (netChunkDesc->IsAuto())
                {
                    // copy each ctor data field into the ctor buffer
                    ReflectedReplicaChunkBase* refChunk = static_cast<ReflectedReplicaChunkBase*>(replicaChunk);
                    netContext->EnumerateCtorData(m_typeId,
                        [&ctx, refChunk](NetworkContext::CtorDataBase* ctorData)
                        {
                            ctorData->Copy(*ctx.m_iBuf, refChunk->m_ctorBuffer);
                        });
                }
            }
            return replicaChunk;
        }
        return nullptr;
    }

    void NetworkContextChunkDescriptor::DeleteReplicaChunk(ReplicaChunkBase* chunk)
    {
        NetworkContext* netContext = nullptr;
        EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
        AZ_Assert(netContext, "No NetworkContext found while trying to destroy ReflectedReplicaChunk");
        netContext->DestroyReplicaChunk(chunk);
    }

    void NetworkContextChunkDescriptor::MarshalCtorData(ReplicaChunkBase* chunk, WriteBuffer& buffer)
    {
        NetworkContext* netContext = nullptr;
        EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
        AZ_Assert(netContext, "No NetworkContext found while trying to collect ctor data for ReflectedReplicaChunk");
        NetBindable* netBindable = static_cast<NetBindable*>(chunk->GetHandler());
        NetworkContextChunkDescriptor* netChunkDesc = static_cast<NetworkContextChunkDescriptor*>(chunk->GetDescriptor());
        if (!netChunkDesc->IsAuto())
        {
            return;
        }

        if (netBindable) // chunk is bound, get source data from the netBindable
        {
            netContext->EnumerateCtorData(m_typeId,
                [netBindable, &buffer](NetworkContext::CtorDataBase* ctorData)
                {
                    ctorData->Marshal(netBindable, buffer);
                });
        }
        else // chunk is not bound yet, copy the ctor data for forwarding
        {
            ReflectedReplicaChunkBase* refChunk = static_cast<ReflectedReplicaChunkBase*>(chunk);
            ReadBuffer src(refChunk->m_ctorBuffer.GetEndianType(), refChunk->m_ctorBuffer.Get(), refChunk->m_ctorBuffer.Size());
            netContext->EnumerateCtorData(m_typeId,
                [&src, &buffer](NetworkContext::CtorDataBase* ctorData)
                {
                    ctorData->Copy(src, buffer);
                });
        }
    }

    void NetworkContextChunkDescriptor::DiscardCtorStream(UnmarshalContext& ctx)
    {
        NetworkContext* netContext = nullptr;
        EBUS_EVENT_RESULT(netContext, NetSystemRequestBus, GetNetworkContext);
        AZ_Assert(netContext, "No NetworkContext found while trying to skip ctor data for ReflectedReplicaChunk");
        if (ctx.m_hasCtorData)
        {
            // Iterate over all of the ctor data and unmarshal it with no destination,
            // which will advance the buffer past the ctor data for this object
            netContext->EnumerateCtorData(m_typeId,
                [&ctx](NetworkContext::CtorDataBase* ctorData)
                {
                    ctorData->Unmarshal(*ctx.m_iBuf, nullptr);
                });
        }
    }
}
