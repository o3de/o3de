/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
