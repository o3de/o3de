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

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaStatus.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>

namespace GridMate
{
    ReplicaStatus::ReplicaStatus()
        : RequestOwnership("RequestOwnership")
        , MigrationSuspendUpstream("MigrationSuspendUpstream")
        , MigrationRequestDownstreamAck("MigrationRequestDownstreamAck")
        , m_options("Options")
        , m_ownerSeq("OwnerSeq")
    {
        SetPriority(0);
    }

    void ReplicaStatus::RegisterType()
    {
        ReplicaChunkDescriptorTable::Get().RegisterChunkType<ReplicaStatus>();
    }

    void ReplicaStatus::OnAttachedToReplica(Replica* replica)
    {
        SetHandler(replica);
    }

    void ReplicaStatus::OnDetachedFromReplica(Replica* replica)
    {
        (void)replica;
        SetHandler(nullptr);
    }

    bool ReplicaStatus::IsReplicaMigratable()
    {
        // Return true to not interfere with the other chunks migration election
        return true;
    }

    const char* ReplicaStatus::GetDebugName() const
    {
        return m_options.Get().m_replicaName.c_str();
    }

    void ReplicaStatus::SetDebugName(const char* debugName)
    {
        m_options.Modify([debugName](ReplicaOptions& opts)
        {
            if (debugName)
            {
                opts.SetDebugName(debugName);
            }
            else
            {
                opts.UnsetDebugName();
            }
            return true;
        });
    }

    void ReplicaStatus::SetUpstreamSuspended(bool isSuspended)
    {
        m_options.Modify([isSuspended](ReplicaOptions& opts)
        {
            bool wasSuspended = opts.IsUpstreamSuspended();
            opts.SetUpstreamSuspended(isSuspended);
            return wasSuspended != isSuspended;
        });
    }

    bool ReplicaStatus::IsUpstreamSuspended() const
    {
        return m_options.Get().IsUpstreamSuspended();
    }
} // namespace GridMate
