/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#if (GM_FUNCTION_NUM_ARGS == 0)
    #define GM_FUNCTION_TEMPLATE_PARMS
    #define GM_FUNCTION_ARGS
    #define GM_FUNCTION_ARGS_CONCAT
    #define GM_FUNCTION_FORWARD
    #define GM_FUNCTION_FORWARD_CONCAT
#elif (GM_FUNCTION_NUM_ARGS == 1)
    #define GM_FUNCTION_TEMPLATE_PARMS  , typename T0
    #define GM_FUNCTION_ARGS            T0 && t0
    #define GM_FUNCTION_ARGS_CONCAT     , GM_FUNCTION_ARGS
    #define GM_FUNCTION_FORWARD         AZStd::forward<T0>(t0)
    #define GM_FUNCTION_FORWARD_CONCAT  , GM_FUNCTION_FORWARD
#elif (GM_FUNCTION_NUM_ARGS == 2)
    #define GM_FUNCTION_TEMPLATE_PARMS  , typename T0, typename T1
    #define GM_FUNCTION_ARGS            T0 && t0, T1 && t1
    #define GM_FUNCTION_ARGS_CONCAT     , GM_FUNCTION_ARGS
    #define GM_FUNCTION_FORWARD         AZStd::forward<T0>(t0), AZStd::forward<T1>(t1)
    #define GM_FUNCTION_FORWARD_CONCAT  , GM_FUNCTION_FORWARD
#elif (GM_FUNCTION_NUM_ARGS == 3)
    #define GM_FUNCTION_TEMPLATE_PARMS  , typename T0, typename T1, typename T2
    #define GM_FUNCTION_ARGS            T0 && t0, T1 && t1, T2 && t2
    #define GM_FUNCTION_ARGS_CONCAT     , GM_FUNCTION_ARGS
    #define GM_FUNCTION_FORWARD         AZStd::forward<T0>(t0), AZStd::forward<T1>(t1), AZStd::forward<T2>(t2)
    #define GM_FUNCTION_FORWARD_CONCAT  , GM_FUNCTION_FORWARD
#elif (GM_FUNCTION_NUM_ARGS == 4)
    #define GM_FUNCTION_TEMPLATE_PARMS  , typename T0, typename T1, typename T2, typename T3
    #define GM_FUNCTION_ARGS            T0 && t0, T1 && t1, T2 && t2, T3 && t3
    #define GM_FUNCTION_ARGS_CONCAT     , GM_FUNCTION_ARGS
    #define GM_FUNCTION_FORWARD         AZStd::forward<T0>(t0), AZStd::forward<T1>(t1), AZStd::forward<T2>(t2), AZStd::forward<T3>(t3)
    #define GM_FUNCTION_FORWARD_CONCAT  , GM_FUNCTION_FORWARD
#elif (GM_FUNCTION_NUM_ARGS == 5)
    #define GM_FUNCTION_TEMPLATE_PARMS  , typename T0, typename T1, typename T2, typename T3, typename T4
    #define GM_FUNCTION_ARGS            T0 && t0, T1 && t1, T2 && t2, T3 && t3, T4 && t4
    #define GM_FUNCTION_ARGS_CONCAT     , GM_FUNCTION_ARGS
    #define GM_FUNCTION_FORWARD         AZStd::forward<T0>(t0), AZStd::forward<T1>(t1), AZStd::forward<T2>(t2), AZStd::forward<T3>(t3), AZStd::forward<T4>(t4)
    #define GM_FUNCTION_FORWARD_CONCAT  , GM_FUNCTION_FORWARD
#else
    #error Unsupported argument count
#endif

/**
    Create a ReplicaChunk that isn't attached to a Replica. To attach it to a replica,
    call replica->AttachReplicaChunk(chunk).
**/
template<class ChunkType GM_FUNCTION_TEMPLATE_PARMS>
ChunkType* CreateReplicaChunk(GM_FUNCTION_ARGS)
{
    static_assert(AZStd::is_base_of<ReplicaChunkBase, ChunkType>::value, "Class must inherit from ReplicaChunk");

    ReplicaChunkDescriptor* descriptor = ReplicaChunkDescriptorTable::Get().FindReplicaChunkDescriptor(ReplicaChunkClassId(ChunkType::GetChunkName()));
    AZ_Assert(descriptor, "Cannot find replica chunk descriptor for %s. Did you remember to register the chunk type?", ChunkType::GetChunkName());
    ReplicaChunkDescriptorTable::Get().BeginConstructReplicaChunk(descriptor);
    ChunkType* chunk = aznew ChunkType(GM_FUNCTION_FORWARD);
    ReplicaChunkDescriptorTable::Get().EndConstructReplicaChunk();
    chunk->Init(descriptor);

    return chunk;
}

/**
    Create a ReplicaChunk that is automatically attached to the replica.
**/
template<class ChunkType GM_FUNCTION_TEMPLATE_PARMS>
ChunkType* CreateAndAttachReplicaChunk(const ReplicaPtr& replica GM_FUNCTION_ARGS_CONCAT)
{
    return CreateAndAttachReplicaChunk<ChunkType>(replica.get() GM_FUNCTION_FORWARD_CONCAT);
}

/**
    Create a ReplicaChunk that is automatically attached to the replica.
**/
template<class ChunkType GM_FUNCTION_TEMPLATE_PARMS>
ChunkType* CreateAndAttachReplicaChunk(Replica* replica GM_FUNCTION_ARGS_CONCAT)
{
    // Chunks cannot be attached while active
    if (replica->IsActive())
    {
        AZ_Warning("GridMate", false, "Cannot attach chunk %s while replica is active", ChunkType::GetChunkName());
        return nullptr;
    }

    ChunkType* chunk = CreateReplicaChunk<ChunkType>(GM_FUNCTION_FORWARD);
    replica->AttachReplicaChunk(chunk);
    return chunk;
}

#undef GM_FUNCTION_TEMPLATE_PARMS
#undef GM_FUNCTION_ARGS
#undef GM_FUNCTION_ARGS_CONCAT
#undef GM_FUNCTION_FORWARD
#undef GM_FUNCTION_FORWARD_CONCAT
