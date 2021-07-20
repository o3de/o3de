/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_INLINE_H
#define GM_REPLICA_INLINE_H

namespace GridMate
{
    /**
         Find a ReplicaChunk by type.
     **/
    template<class R>
    inline AZStd::intrusive_ptr<R> Replica::FindReplicaChunk()
    {
        static_assert(AZStd::is_base_of<ReplicaChunkBase, R>::value, "Class must inherit from ReplicaChunkBase");

        for (auto chunk : m_chunks)
        {
            if (chunk && chunk->IsType<R>())
            {
                return AZStd::static_pointer_cast<R>(chunk);
            }
        }
        return nullptr;
    }
}

#endif // GM_REPLICA_INLINE_H
