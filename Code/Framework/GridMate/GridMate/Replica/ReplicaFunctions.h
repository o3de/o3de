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
#ifndef GM_REPLICA_FUNCTIONS_H
#define GM_REPLICA_FUNCTIONS_H

#include <AzCore/std/typetraits/is_base_of.h>

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunkDescriptor.h>

namespace GridMate
{
    /**
        Create a ReplicaChunk that isn't attached to a Replica. To attach it to a replica,
        call replica->AttachReplicaChunk(chunk).
     **/
    template<class ChunkType, class ... Args>
    ChunkType* CreateReplicaChunk(Args&& ... args)
    {
        static_assert(AZStd::is_base_of<ReplicaChunkBase, ChunkType>::value, "Class must inherit from ReplicaChunk");

        ReplicaChunkDescriptor* descriptor = ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(ChunkType::GetChunkName()));
        AZ_Error("GridMate", descriptor, "Cannot find replica chunk descriptor for %s. Did you remember to register the chunk type?", ChunkType::GetChunkName());
        if (descriptor != nullptr)
        {
            ReplicaChunkDescriptorTable::Get().BeginConstructReplicaChunk(descriptor);
            ChunkType* chunk = aznew ChunkType(AZStd::forward<Args>(args) ...);
            ReplicaChunkDescriptorTable::Get().EndConstructReplicaChunk();
            chunk->Init(descriptor);

            return chunk;
        }
        return nullptr;
    }

    /**
        Create a ReplicaChunk that is automatically attached to the replica.
     **/
    template<class ChunkType, class ... Args>
    ChunkType* CreateAndAttachReplicaChunk(const ReplicaPtr& replica, Args&& ... args)
    {
        return CreateAndAttachReplicaChunk<ChunkType>(replica.get(), AZStd::forward<Args>(args) ...);
    }

    /**
        Create a ReplicaChunk that is automatically attached to the replica.
     **/
    template<class ChunkType, class ... Args>
    ChunkType* CreateAndAttachReplicaChunk(Replica* replica, Args&& ... args)
    {
        // Chunks cannot be attached while active
        if (replica->IsActive())
        {
            AZ_Warning("GridMate", false, "Cannot attach chunk %s while replica is active", ChunkType::GetChunkName());
            return nullptr;
        }

        ChunkType* chunk = CreateReplicaChunk<ChunkType>(AZStd::forward<Args>(args) ...);
        replica->AttachReplicaChunk(chunk);
        return chunk;
    }
}

#endif // GM_REPLICA_FUNCTIONS_H
