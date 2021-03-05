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
