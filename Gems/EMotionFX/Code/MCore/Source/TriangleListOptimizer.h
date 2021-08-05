/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/vector.h>

namespace MCore
{
    /**
     * The triangle list optimizer.
     * This can be used to improve cache efficiency. It reorders the index buffers to maximize the number of cache hits.
     */
    template<typename IndexType>
    class TriangleListOptimizer
    {
    public:
        /**
         * The constructor.
         * @param numCacheEntries The cache size in number of elements. Smaller values often result in better optimizations.
         */
        TriangleListOptimizer(size_t numCacheEntries = 8);

        /**
         * Optimizes an index buffer.
         * This will modify the input triangle list index buffer.
         * Each triangle needs three indices.
         * @param triangleList The index buffer.
         * @param numIndices The number of indices inside the specified index buffer.
         */
        void OptimizeIndexBuffer(IndexType* triangleList, size_t numIndices);

        /**
         * Calculate the number of cache hits that the a given triange list would get.
         * Higher values will be better than lower values.
         * @param triangleList The index buffer, with three indices per triangle.
         * @param numIndices The number of indices.
         * @result The number of cache hits.
         */
        size_t CalcNumCacheHits(IndexType* triangleList, size_t numIndices);

    private:
        AZStd::vector<IndexType> m_entries{};
        size_t m_numUsedEntries = 0; /**< The number of used cache entries. */
        size_t m_oldestEntry = 0; /**< The index to the oldest entry, which will be overwritten first when the cache is full. */

        void Flush();

        /**
         * Calculate the number of cache hits for a given triangle.
         * @param indexA The first vertex index.
         * @param indexB The second vertex index.
         * @param indexC The third vertex index.
         * @result The number of cache hits that this triangle would give.
         */
        size_t CalcNumCacheHits(IndexType indexA, IndexType indexB, IndexType indexC) const;

        /**
         * Add an index value to the cache.
         * @param vertexIndex The vertex index value.
         */
        void AddToCache(IndexType vertexIndex);
    };

    template<typename IndexType>
    TriangleListOptimizer<IndexType>::TriangleListOptimizer(size_t numCacheEntries)
    {
        // We never push more items than this capacity. The vector stores the
        // max number of entries for us in its capacity() method.
        m_entries.set_capacity(numCacheEntries);
    }

    template<typename IndexType>
    void TriangleListOptimizer<IndexType>::OptimizeIndexBuffer(IndexType* triangleList, size_t numIndices)
    {
        Flush();

        // create a temporary buffer
        AZStd::vector<IndexType> newBuffer(numIndices);
        size_t maxIndices = numIndices;

        // for all triangles in the triangle list
        for (size_t f = 0; f < numIndices; f += 3)
        {
            size_t mostEfficient = 0;
            size_t mostHits = 0;

            for (size_t i = 0; i < maxIndices; i += 3)
            {
                // get the triangle indices
                const IndexType indexA = triangleList[i];
                const IndexType indexB = triangleList[i + 1];
                const IndexType indexC = triangleList[i + 2];

                // calculate how many hits this triangle would give
                size_t numHits = CalcNumCacheHits(indexA, indexB, indexC);

                // if the number of hits is the maximum hits we can score, use this triangle
                if (numHits == 3)
                {
                    mostHits = numHits;
                    mostEfficient = i;
                    break;
                }

                // check if this gives more hits than any other triangle we tested before
                if (numHits > mostHits)
                {
                    mostHits = numHits;
                    mostEfficient = i;
                }
            }

            // insert the triangle that gave most cache hits to the new triangle list
            AddToCache(triangleList[mostEfficient]);
            AddToCache(triangleList[mostEfficient + 1]);
            AddToCache(triangleList[mostEfficient + 2]);
            newBuffer[f] = triangleList[mostEfficient];
            newBuffer[f + 1] = triangleList[mostEfficient + 1];
            newBuffer[f + 2] = triangleList[mostEfficient + 2];

            // remove the triangle from the old list, so that we don't test it anymore next time, since we already inserted it inside the new optimized list
            AZStd::move(
                /*input first*/ triangleList + mostEfficient + 3,
                /*input last*/ triangleList + maxIndices,
                /*output first*/ triangleList + mostEfficient);

            // we need to test one less triangle next time, since we just added one of the triangles to the new list
            // so there is one less left
            maxIndices -= 3;
        }

        // copy the results
        AZStd::copy(begin(newBuffer), end(newBuffer), triangleList);
    }

    // calculate the number of cache hits
    template<typename IndexType>
    size_t TriangleListOptimizer<IndexType>::CalcNumCacheHits(IndexType* triangleList, size_t numIndices)
    {
        // clear the cache
        Flush();

        size_t totalHits = 0;

        // for all triangles in the triangle list
        for (size_t f = 0; f < numIndices; f += 3)
        {
            const IndexType indexA = triangleList[f];
            const IndexType indexB = triangleList[f + 1];
            const IndexType indexC = triangleList[f + 2];

            totalHits += CalcNumCacheHits(indexA, indexB, indexC);

            AddToCache(indexA);
            AddToCache(indexB);
            AddToCache(indexC);
        }

        return totalHits;
    }

    template<typename IndexType>
    void TriangleListOptimizer<IndexType>::Flush()
    {
        m_numUsedEntries = 0;
        m_oldestEntry = 0;
        m_entries.clear();
    }

    // calculate the number of cache hits for a triangle
    template<typename IndexType>
    size_t TriangleListOptimizer<IndexType>::CalcNumCacheHits(IndexType indexA, IndexType indexB, IndexType indexC) const
    {
        size_t total = 0;

        // check all cache entries
        for (IndexType entryValue : m_entries)
        {
            if (entryValue == indexA || entryValue == indexB || entryValue == indexC)
            {
                total++;
            }
            if (total == 3)
            {
                break;
            }
        }

        return total;
    }

    // get a value from the cache
    template<typename IndexType>
    void TriangleListOptimizer<IndexType>::AddToCache(IndexType vertexIndex)
    {
        // check if this entry is already in the cache, if so we have a cache hit and can quit
        const auto entryIt = AZStd::find(m_entries.begin(), m_entries.end(), vertexIndex);
        if (entryIt != m_entries.end())
        {
            return;
        }

        // the entry is not inside the cache and the cache is not full yet, we can simply insert the cache entry and return
        if (m_numUsedEntries < m_entries.capacity())
        {
            m_entries.emplace_back(vertexIndex);
            ++m_numUsedEntries;
            return;
        }

        // the cache is full, remove one of the entries
        // since we simulate a FIFO cache, we have to remove the oldest entry
        m_entries[m_oldestEntry] = vertexIndex;
        ++m_oldestEntry %= m_entries.capacity();
    }
} // namespace MCore
