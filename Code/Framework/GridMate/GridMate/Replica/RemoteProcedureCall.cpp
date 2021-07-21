/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>

#include <GridMate/Replica/RemoteProcedureCall.h>
#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaDrillerEvents.h>
#include <GridMate/Replica/ReplicaMgr.h>
#include <GridMate/Replica/ReplicaUtils.h>

namespace GridMate
{
    RpcBase::RpcBase(const char* debugName)
        : m_replicaChunk(nullptr)
    {
        ReplicaChunkInitContext* initContext = ReplicaChunkDescriptorTable::Get().GetCurrentReplicaChunkInitContext();
        AZ_Assert(initContext, "Replica construction stack is NOT pushed on the stack! Call Replica::Desriptor::Push() before construction!");
        ReplicaChunkDescriptor* descriptor = initContext->m_descriptor;
        AZ_Assert(descriptor, "Replica's descriptor is NOT pushed on the stack! Call Replica::Desriptor::Push() before construction!");
        descriptor->RegisterRPC(debugName, this);
    }

    void RpcBase::Queue(GridMate::Internal::RpcRequest* rpc)
    {
        m_replicaChunk->QueueRPCRequest(rpc);
    }

    void RpcBase::OnRpcRequest(GridMate::Internal::RpcRequest* rpc) const
    {
        EBUS_EVENT(Debug::ReplicaDrillerBus, OnRequestRpc, m_replicaChunk, rpc);
    }

    void RpcBase::OnRpcInvoke(GridMate::Internal::RpcRequest* rpc) const
    {
        EBUS_EVENT(Debug::ReplicaDrillerBus, OnInvokeRpc, m_replicaChunk, rpc);
    }

    PeerId RpcBase::GetSourcePeerId()
    {
        if (m_replicaChunk->GetReplicaManager())
        {
            return m_replicaChunk->GetReplicaManager()->GetLocalPeerId();
        }
        return InvalidReplicaPeerId;
    }
}
