/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/DataSet.h>
#include <GridMate/Replica/ReplicaStatus.h>
#include <GridMate/Replica/SystemReplicas.h>

namespace GridMate
{
    //=========================================================================
    // ReplicaChunkDescriptor
    //=========================================================================
    ReplicaChunkDescriptor::ReplicaChunkDescriptor(const char* pNameStr, AZStd::size_t classSize)
        : m_chunkTypeId(pNameStr)
        , m_chunkClassName(pNameStr)
        , m_chunkClassSize(classSize)
        , m_isInitialized(false)
    {
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkDescriptor::RegisterDataSet(const char* debugName, DataSetBase* ds)
    {
        ReplicaChunkBase* chunk = ReplicaChunkDescriptorTable::Get().GetCurrentReplicaChunkInitContext()->m_chunk;
        AZ_Assert(chunk, "Replica chunk pointer was not pushed on the stack! Datasets can only be members of replica chunks!");

        if (!m_isInitialized)
        {
            size_t dsOffset = reinterpret_cast<size_t>(ds);
            size_t baseOffset = reinterpret_cast<size_t>(chunk);
            AZ_Assert(baseOffset <= dsOffset && baseOffset + m_chunkClassSize > dsOffset, "Dataset offset is not within its parent's boundaries. Datasets must be part of replica chunks!");
            const char* finalName = (debugName && strlen(debugName) ? debugName : "<Unknown DataSet>");
            ptrdiff_t offset = dsOffset - baseOffset;
            RegisterDataSet(finalName, offset);
        }
    }

    //-----------------------------------------------------------------------------
    void ReplicaChunkDescriptor::RegisterDataSet(const char* debugName, ptrdiff_t offset)
    {
        if (!m_isInitialized)
        {
            const char* finalName = (debugName && strlen(debugName) ? debugName : "<Unknown DataSet>");
            AZ_Assert(m_vdt.size() < GM_MAX_DATASETS_IN_CHUNK, "Replica chunks can only support up to %d datasets.", GM_MAX_DATASETS_IN_CHUNK);
            m_vdt.push_back();
            m_vdt.back().m_offset = offset;
            m_vdt.back().m_debugName = finalName;
        }
    }
    //-----------------------------------------------------------------------------

    DataSetBase* ReplicaChunkDescriptor::GetDataSet(const ReplicaChunkBase* base, size_t index) const
    {
        AZ_Assert(index < m_vdt.size(), "Invalid DataSet index!");
        return reinterpret_cast<DataSetBase*>(reinterpret_cast<size_t>(base) + m_vdt[index].m_offset);
    }
    //-----------------------------------------------------------------------------
    size_t ReplicaChunkDescriptor::GetDataSetIndex(const ReplicaChunkBase* base, const DataSetBase* dataset) const
    {
        ptrdiff_t offset = reinterpret_cast<size_t>(dataset) - reinterpret_cast<size_t>(base);
        return GetDataSetIndex(offset);
    }
    //-----------------------------------------------------------------------------
    size_t ReplicaChunkDescriptor::GetDataSetIndex(ptrdiff_t offset) const
    {
        for (size_t i = 0; i < m_vdt.size(); ++i)
        {
            if (m_vdt[i].m_offset == offset)
            {
                return i;
            }
        }
        AZ_Assert(false, "Can't find DataSet index! Please check that DataSet pointer is valid!");
        return static_cast<size_t>(-1);
    }
    //-----------------------------------------------------------------------------
    const char* ReplicaChunkDescriptor::GetDataSetName(const ReplicaChunkBase* base, const DataSetBase* dataset) const
    {
        ptrdiff_t offset = reinterpret_cast<size_t>(dataset) - reinterpret_cast<size_t>(base);
        for (size_t i = 0; i < m_vdt.size(); ++i)
        {
            if (m_vdt[i].m_offset == offset)
            {
                return m_vdt[i].m_debugName;
            }
        }
        return "<Unknown DataSet>";
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkDescriptor::RegisterRPC(const char* debugName, RpcBase* rpc)
    {
        ReplicaChunkBase* chunk = ReplicaChunkDescriptorTable::Get().GetCurrentReplicaChunkInitContext()->m_chunk;
        AZ_Assert(chunk, "Replica chunk pointer was not pushed on the stack! RPCs can only be members of replica chunks!");

        if (!m_isInitialized)
        {
            size_t rpcOffset = reinterpret_cast<size_t>(rpc);
            size_t baseOffset = reinterpret_cast<size_t>(chunk);
            AZ_Assert(baseOffset <= rpcOffset && baseOffset + m_chunkClassSize > rpcOffset, "RPC offset is not within its parent's boundaries. RPCs must be part of replica chunks!");
            const char* finalName = (debugName && strlen(debugName) ? debugName : "<Unknown RPC>");
            ptrdiff_t offset = rpcOffset - baseOffset;
            RegisterRPC(finalName, offset);
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkDescriptor::RegisterRPC(const char* debugName, ptrdiff_t offset)
    {
        if (!m_isInitialized)
        {
            const char* finalName = (debugName && strlen(debugName) ? debugName : "<Unknown RPC>");
            AZ_Assert(m_vrt.size() < GM_MAX_RPCS_DECL_PER_CHUNK, "Replica chunks can only support up to %d RPCs.", GM_MAX_RPCS_DECL_PER_CHUNK);
            m_vrt.push_back();
            m_vrt.back().m_offset = offset;
            m_vrt.back().m_debugName = finalName;
        }
    }
    //-----------------------------------------------------------------------------
    RpcBase* ReplicaChunkDescriptor::GetRpc(const ReplicaChunkBase* base, size_t index) const
    {
        if (index < m_vrt.size())
        {
            return reinterpret_cast<RpcBase*>(reinterpret_cast<size_t>(base)+m_vrt[index].m_offset);
        }

        AZ_Warning("GridMate", false, "Invalid RPC index!");
        return nullptr;
    }
    //-----------------------------------------------------------------------------
    size_t ReplicaChunkDescriptor::GetRpcIndex(const ReplicaChunkBase* base, const RpcBase* rpc) const
    {
        ptrdiff_t offset = reinterpret_cast<size_t>(rpc) - reinterpret_cast<size_t>(base);
        return GetRpcIndex(offset);
    }
    //-----------------------------------------------------------------------------
    size_t ReplicaChunkDescriptor::GetRpcIndex(ptrdiff_t offset) const
    {
        for (size_t i = 0; i < m_vrt.size(); ++i)
        {
            if (m_vrt[i].m_offset == offset)
            {
                return i;
            }
        }
        AZ_Assert(false, "Can't find RPC index! Please check that rpc pointer is valid!");
        return static_cast<size_t>(-1);
    }
    //-----------------------------------------------------------------------------
    const char* ReplicaChunkDescriptor::GetRpcName(const ReplicaChunkBase* base, const RpcBase* rpc) const
    {
        ptrdiff_t offset = reinterpret_cast<size_t>(rpc) - reinterpret_cast<size_t>(base);
        for (size_t i = 0; i < m_vrt.size(); ++i)
        {
            if (m_vrt[i].m_offset == offset)
            {
                return m_vrt[i].m_debugName;
            }
        }
        return "<Unknown RPC>";
    }
    //-----------------------------------------------------------------------------

    //=========================================================================
    // ReplicaChunkDescriptorTable
    //=========================================================================
#   define GRIDMATE_DESCRIPTOR_TABLE_VARIABLE_NAME AZ_CRC("GridMateReplicaChunkDescriptorTable", 0xd1d00091)
#   define GRIDMATE_CHUNK_INIT_CONTEXT_STACK_VARIABLE_NAME AZ_CRC("GridMateReplicaChunkInitContextStack", 0x67fbe724)

    ReplicaChunkDescriptorTable ReplicaChunkDescriptorTable::s_theTable;
    //-----------------------------------------------------------------------------
    ReplicaChunkDescriptorTable& ReplicaChunkDescriptorTable::Get()
    {
        if (!s_theTable.m_globalDescriptorTable)
        {
            s_theTable.m_globalDescriptorTable = AZ::Environment::CreateVariable<DescriptorContainerType>(GRIDMATE_DESCRIPTOR_TABLE_VARIABLE_NAME);

            // Register all the internal replica chunk types
            if (!s_theTable.FindReplicaChunkDescriptor(ReplicaChunkClassId(ReplicaStatus::GetChunkName())))
            {
                ReplicaStatus::RegisterType();
            }
            if (!s_theTable.FindReplicaChunkDescriptor(ReplicaChunkClassId(ReplicaInternal::SessionInfo::GetChunkName())))
            {
                ReplicaInternal::SessionInfo::RegisterType();
            }
            if (!s_theTable.FindReplicaChunkDescriptor(ReplicaChunkClassId(ReplicaInternal::PeerReplica::GetChunkName())))
            {
                ReplicaInternal::PeerReplica::RegisterType();
            }
        }

        if (!s_theTable.m_globalChunkInitContextStack)
        {
            s_theTable.m_globalChunkInitContextStack = AZ::Environment::CreateVariable<ReplicaChunkInitContextStack>(GRIDMATE_CHUNK_INIT_CONTEXT_STACK_VARIABLE_NAME);
        }

        return s_theTable;
    }
    //-----------------------------------------------------------------------------
    ReplicaChunkDescriptorTable::~ReplicaChunkDescriptorTable()
    {
        // This cannot currently be shut down on some platforms, as this static is shutdown after
        // all allocators (and in this case, specifically the OSAllocator) are gone
        if (AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
        {
            while (!m_moduleDescriptorTable.empty())
            {
                UnregisterReplicaChunkDescriptor(m_moduleDescriptorTable.back().m_chunkTypeId);
            }
        }
    }
    //-----------------------------------------------------------------------------
    ReplicaChunkDescriptor* ReplicaChunkDescriptorTable::FindReplicaChunkDescriptor(ReplicaChunkClassId chunkTypeId)
    {
        for (DescriptorInfo& info : * m_globalDescriptorTable)
        {
            if (chunkTypeId == info.m_chunkTypeId)
            {
                return info.m_descriptor;
            }
        }
        return nullptr;
    }
    //-----------------------------------------------------------------------------
    bool ReplicaChunkDescriptorTable::UnregisterReplicaChunkDescriptor(ReplicaChunkClassId chunkTypeId)
    {
        for (DescriptorContainerType::iterator itModuleTable = m_moduleDescriptorTable.begin(); itModuleTable != m_moduleDescriptorTable.end(); ++itModuleTable)
        {
            DescriptorInfo* descInfo = &*itModuleTable;
            if (chunkTypeId == descInfo->m_chunkTypeId)
            {
                azdestroy(descInfo->m_descriptor, AZ::OSAllocator);
                m_moduleDescriptorTable.erase(itModuleTable);
                delete descInfo;
                if (UnregisterReplicaChunkDescriptorFromGlobalTable(chunkTypeId))
                {
                    return true;
                }
                AZ_TracePrintf("GridMate", "Failed to find replica chunk descriptor in global table! Removing from local table.");
                return false;
            }
        }
        AZ_TracePrintf("GridMate", "Failed to find replica chunk descriptor in local table! Descriptor cannot be unregistered from this module!");
        return false;
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkDescriptorTable::AddReplicaChunkDescriptor(ReplicaChunkClassId chunkTypeId, ReplicaChunkDescriptor* descriptor)
    {
        DescriptorInfo* localDescInfo = aznew DescriptorInfo();
        localDescInfo->m_chunkTypeId = chunkTypeId;
        localDescInfo->m_descriptor = descriptor;
        m_moduleDescriptorTable.push_back(*localDescInfo);

        DescriptorInfo* globalDescInfo = aznew DescriptorInfo();
        globalDescInfo->m_chunkTypeId = chunkTypeId;
        globalDescInfo->m_descriptor = descriptor;
        m_globalDescriptorTable->push_back(*globalDescInfo);
    }
    //-----------------------------------------------------------------------------
    bool ReplicaChunkDescriptorTable::UnregisterReplicaChunkDescriptorFromGlobalTable(ReplicaChunkClassId chunkTypeId)
    {
        for (DescriptorContainerType::iterator itGlobalTable = m_globalDescriptorTable->begin(); itGlobalTable != m_globalDescriptorTable->end(); ++itGlobalTable)
        {
            DescriptorInfo* descInfo = &*itGlobalTable;
            if (descInfo->m_chunkTypeId == chunkTypeId)
            {
                m_globalDescriptorTable->erase(itGlobalTable);
                delete descInfo;
                return true;
            }
        }
        return false;
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkDescriptorTable::BeginConstructReplicaChunk(ReplicaChunkDescriptor* descriptor)
    {
        m_globalChunkInitContextStack->push_back();
        m_globalChunkInitContextStack->back().m_descriptor = descriptor;

        // If the descriptor's tables have already been populated, don't re-populate them.
        if (descriptor->GetDataSetCount() > 0 || descriptor->GetRpcCount() > 0)
        {
            descriptor->m_isInitialized = true;
        }
    }
    //-----------------------------------------------------------------------------
    void ReplicaChunkDescriptorTable::EndConstructReplicaChunk()
    {
        m_globalChunkInitContextStack->back().m_descriptor->m_isInitialized = true;
        m_globalChunkInitContextStack->pop_back();
    }
    //-----------------------------------------------------------------------------
    ReplicaChunkInitContext* ReplicaChunkDescriptorTable::GetCurrentReplicaChunkInitContext()
    {
        return m_globalChunkInitContextStack->size() > 0 ? &m_globalChunkInitContextStack->back() : nullptr;
    }
    //-----------------------------------------------------------------------------
} // GridMate
