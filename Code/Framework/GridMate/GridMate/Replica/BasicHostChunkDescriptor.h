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

#pragma once

#include <GridMate/Replica/ReplicaChunkDescriptor.h>
#include <GridMate/Replica/ReplicaMgr.h>

namespace GridMate
{
    /**
    * BasicHostChunkDescriptor implements a helper descriptor
    * that prevents chunk proxies from being created on the host.
    * The idea is to prevent a malicious client from creating
    * (and owning) chunk types that should always be authoritative
    * on the host.
    */
    template<typename ReplicaChunkType>
    class BasicHostChunkDescriptor
        : public DefaultReplicaChunkDescriptor<ReplicaChunkType>
    {
    public:
        ReplicaChunkBase* CreateFromStream(UnmarshalContext& mc) override
        {
            ReplicaChunkBase* replicaChunk = nullptr;
            AZ_Assert(!mc.m_rm->IsSyncHost(), "Replicas of type %s can only be owned by the host!", DefaultReplicaChunkDescriptor<ReplicaChunkType>::GetChunkName());
            if (!mc.m_rm->IsSyncHost())
            {
                replicaChunk = aznew ReplicaChunkType;
            }
            return replicaChunk;
        }
    };
}
