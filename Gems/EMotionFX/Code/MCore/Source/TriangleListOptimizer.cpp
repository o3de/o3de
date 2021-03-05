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

// include the required headers
#include "TriangleListOptimizer.h"
#include "MemoryManager.h"
#include "LogManager.h"


namespace MCore
{
    // constructor
    TriangleListOptimizer::TriangleListOptimizer(uint32 numCacheEntries)
    {
        // allocate and initialize the cache entries
        mNumEntries = numCacheEntries;
        mEntries    = (uint32*)Allocate(numCacheEntries * sizeof(uint32), MCORE_MEMCATEGORY_TRILISTOPTIMIZER);

        // flush the cache, so it starts in an initial (empty) state
        Flush();
    }



    // destructor
    TriangleListOptimizer::~TriangleListOptimizer()
    {
        // get rid of the cache data
        MCore::Free(mEntries);
    }



    // optimize a triangle list
    void TriangleListOptimizer::OptimizeIndexBuffer(uint32* triangleList, uint32 numIndices)
    {
        // flush the cache
        Flush();

        // check the number of hits before optimizations
        //  uint32 beforeHits = CalcNumCacheHits(triangleList, numIndices); // TODO: remove in final version

        // create a temparory buffer
        uint32* newBuffer   = (uint32*)Allocate(sizeof(uint32) * numIndices, 0);
        uint32 maxIndices   = numIndices;
        uint32 totalHits    = 0;

        // for all triangles in the triangle list
        for (uint32 f = 0; f < numIndices; f += 3)
        {
            uint32 mostEfficient = 0;
            uint32 mostHits = 0;

            //
            for (uint32 i = 0; i < maxIndices; i += 3)
            {
                // get the triangle indices
                const uint32 indexA = triangleList[i];
                const uint32 indexB = triangleList[i + 1];
                const uint32 indexC = triangleList[i + 2];

                // calculate how many hits this triangle would give
                uint32 numHits = CalcNumCacheHits(indexA, indexB, indexC);

                // if the number of hits is the maximum hits we can score, use this triangle
                if (numHits == 3)
                {
                    mostHits      = numHits;
                    mostEfficient = i;
                    break;
                }

                // check if this gives more hits than any other triangle we tested before
                if (numHits > mostHits)
                {
                    mostHits      = numHits;
                    mostEfficient = i;
                }
            }

            // insert the triangle that gave most cache hits to the new triangle list
            newBuffer[f]   = GetFromCache(triangleList[mostEfficient]);
            newBuffer[f + 1] = GetFromCache(triangleList[mostEfficient + 1]);
            newBuffer[f + 2] = GetFromCache(triangleList[mostEfficient + 2]);

            // increase the number of cache hits
            totalHits += mostHits;

            // remove the triangle from the old list, so that we don't test it anymore next time, since we already inserted it inside the new optimized list
            MCore::MemMove(triangleList + mostEfficient, triangleList + mostEfficient + 3, (maxIndices - mostEfficient - 3) * sizeof(uint32));

            // we need to test one less triangle next time, since we just added one of the triangles to the new list
            // so there is one less left
            maxIndices -= 3;
        }

        // display some optimization results
        //  LOG("Vertex cache optimization performance gain factor = %.2fx", (totalHits / (float)numIndices) * 100.0f, totalHits / (float)beforeHits);

        // copy the results
        MCore::MemCopy(triangleList, newBuffer, numIndices * sizeof(uint32));

        // get rid of the new buffer again
        MCore::Free(newBuffer);
    }



    // calculate the number of cache hits
    uint32 TriangleListOptimizer::CalcNumCacheHits(uint32* triangleList, uint32 numIndices)
    {
        // clear the cache
        Flush();

        uint32 totalHits = 0;

        // for all triangles in the triangle list
        for (uint32 f = 0; f < numIndices; f += 3)
        {
            const uint32 indexA = triangleList[f];
            const uint32 indexB = triangleList[f + 1];
            const uint32 indexC = triangleList[f + 2];

            // calculate the number of cache hits
            totalHits += CalcNumCacheHits(indexA, indexB, indexC);

            // insert the indices inside the cache, so basically update the cache
            GetFromCache(indexA);
            GetFromCache(indexB);
            GetFromCache(indexC);
        }

        return totalHits;
    }



    // flush the cache
    void TriangleListOptimizer::Flush()
    {
        mNumUsedEntries = 0;
        mOldestEntry    = 0;

        for (uint32 i = 0; i < mNumEntries; ++i)
        {
            mEntries[i] = MCORE_INVALIDINDEX32;
        }
    }



    // find the cache number for a given index
    uint32 TriangleListOptimizer::FindEntryNumber(uint32 index) const
    {
        for (uint32 i = 0; i < mNumEntries; ++i)
        {
            if (mEntries[i] == index)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }



    // calculate the number of cache hits for a triangle
    uint32 TriangleListOptimizer::CalcNumCacheHits(uint32 indexA, uint32 indexB, uint32 indexC) const
    {
        uint32 total = 0;

        // check all cache entries
        for (uint32 i = 0; i < mNumEntries && total < 3; ++i)
        {
            uint32 entryValue = mEntries[i];
            if (entryValue == indexA || entryValue == indexB || entryValue == indexC)
            {
                total++;
            }
        }

        return total;
    }



    // get a value from the cache
    uint32 TriangleListOptimizer::GetFromCache(uint32 vertexIndex)
    {
        // check if this entry is already in the cache, if so we have a cache hit and can quit
        uint32 entryNr = FindEntryNumber(vertexIndex);
        if (entryNr != MCORE_INVALIDINDEX32)
        {
            return vertexIndex;
        }

        // the entry is not inside the cache and the cache is not full yet, we can simply insert the cache entry and return
        if (mNumUsedEntries < mNumEntries)
        {
            mEntries[mNumUsedEntries] = vertexIndex;
            mNumUsedEntries++;
            return vertexIndex;
        }

        // the cache is full, remove one of the entries
        // since we simulate a FIFO cache, we have to remove the oldest entry
        mEntries[mOldestEntry] = vertexIndex;
        mOldestEntry++;
        if (mOldestEntry >= mNumEntries)
        {
            mOldestEntry = 0;
        }

        return vertexIndex;
    }
}   // namespace MCore
