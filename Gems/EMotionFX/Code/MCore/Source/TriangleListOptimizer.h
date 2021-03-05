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

// include the required headers
#include "StandardHeaders.h"
#include "MemoryManager.h"


namespace MCore
{
    /**
     * The triangle list optimizer.
     * This can be used to improve cache efficiency. It reorders the index buffers to maximize the number of cache hits.
     */
    class MCORE_API TriangleListOptimizer
    {
        MCORE_MEMORYOBJECTCATEGORY(TriangleListOptimizer, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_TRILISTOPTIMIZER)

    public:
        /**
         * The constructor.
         * @param numCacheEntries The cache size in number of elements. Smaller values often result in better optimizations.
         */
        TriangleListOptimizer(uint32 numCacheEntries = 8);

        /**
         * The destructor.
         */
        ~TriangleListOptimizer();

        /**
         * Optimizes an index buffer.
         * This will modify the input triangle list index buffer.
         * Each triangle needs three indices.
         * @param triangleList The index buffer.
         * @param numIndices The number of indices inside the specified index buffer.
         */
        void OptimizeIndexBuffer(uint32* triangleList, uint32 numIndices);

        /**
         * Calculate the number of cache hits that the a given triange list would get.
         * Higher values will be better than lower values.
         * @param triangleList The index buffer, with three indices per triangle.
         * @param numIndices The number of indices.
         * @result The number of cache hits.
         */
        uint32 CalcNumCacheHits(uint32* triangleList, uint32 numIndices);


    private:
        uint32* mEntries;           /**< The cache entries. */
        uint32  mNumEntries;        /**< The number of cache entries. */
        uint32  mNumUsedEntries;    /**< The number of used cache entries. */
        uint32  mOldestEntry;       /**< The index to the oldest entry, which will be overwritten first when the cache is full. */

        /**
         * Flush the cache.
         * This empties the cache.
         */
        void Flush();

        /**
         * Find the cache entry number/index for a given index value.
         * @param index The index buffer index value (vertex number) to search for.
         * @result Returns the cache entry number that stores this index value, or MCORE_INVALIDINDEX32 in case it hasn't been found in the cache.
         */
        uint32 FindEntryNumber(uint32 index) const;

        /**
         * Calculate the number of cache hits for a given triangle.
         * @param indexA The first vertex index.
         * @param indexB The second vertex index.
         * @param indexC The third vertex index.
         * @result The number of cache hits that this triangle would give.
         */
        uint32 CalcNumCacheHits(uint32 indexA, uint32 indexB, uint32 indexC) const;

        /**
         * Get an index value from the cache.
         * If the vertex index isn't inside the cache yet, it will be loaded into the cache.
         * @param vertexIndex The vertex index value.
         * @result Returns the same value as the vertexIndex parameter.
         */
        uint32 GetFromCache(uint32 vertexIndex);
    };
}   // namespace MCore
