/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include <AzCore/std/sort.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/numeric.h>
#include "MeshBuilderSkinningInfo.h"
#include "MeshBuilderSubMesh.h"
#include "MeshBuilder.h"
#include "MeshBuilderVertexAttributeLayers.h"

namespace AZ::MeshBuilder
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilderSkinningInfo, AZ::SystemAllocator, 0)


    // constructor
    MeshBuilderSkinningInfo::MeshBuilderSkinningInfo(size_t numOrgVertices)
    {
        mInfluences.SetNumPreCachedElements(4); // TODO: verify if this is the fastest
        mInfluences.Resize(numOrgVertices);
    }


    // optimize weights
    void MeshBuilderSkinningInfo::OptimizeSkinningInfluences(AZStd::vector<Influence>& influences, float tolerance, size_t maxWeights)
    {
        const auto influenceLess = [](const Influence& left, const Influence& right) { return left.mWeight < right.mWeight; };

        // Move all the items greater than the tolerance to the end of the array
        auto removePoint = AZStd::remove_if(begin(influences), end(influences), [tolerance](const Influence& influence) { return influence.mWeight < tolerance; });
        if (removePoint == begin(influences))
        {
            // If this would remove all influences, keep the biggest one
            auto [_, maxElement] = AZStd::minmax_element(begin(influences), end(influences), influenceLess);
            AZStd::swap(removePoint, maxElement);
            ++removePoint;
        }
        // remove all weights below the tolerance
        influences.erase(removePoint, end(influences));

        // reduce number of weights when needed
        while (influences.size() > maxWeights)
        {
            // remove this smallest weight
            const auto [minInfluence, _] = AZStd::minmax_element(begin(influences), end(influences), influenceLess);
            influences.erase(minInfluence);
        }

        // calculate the total weight
        const float totalWeight = AZStd::accumulate(begin(influences), end(influences), 0.0f, [](float total, const Influence& influence) { return total + influence.mWeight; });

        // normalize
        for (Influence& influence : influences)
        {
            influence.mWeight /= totalWeight;
        }
    }


    // sort influences on weights, from big to small
    void MeshBuilderSkinningInfo::SortInfluences(AZStd::vector<Influence>& influences)
    {
        AZStd::sort(begin(influences), end(influences), [](const auto& lhs, const auto& rhs)
        {
            return lhs.mWeight > rhs.mWeight;
        });
    }


    // optimize the weight data
    void MeshBuilderSkinningInfo::Optimize(AZ::u32 maxNumWeightsPerVertex, float weightThreshold)
    {
        AZStd::vector<Influence> influences;

        // for all vertices
        const size_t numOrgVerts = GetNumOrgVertices();
        for (size_t v = 0; v < numOrgVerts; ++v)
        {
            // gather all weights
            const size_t numInfluences = GetNumInfluences(v);
            influences.resize(numInfluences);
            for (size_t i = 0; i < numInfluences; ++i)
            {
                influences[i] = GetInfluence(v, i);
            }

            // optimize the weights and sort them from big to small weight
            OptimizeSkinningInfluences(influences, weightThreshold, maxNumWeightsPerVertex);
            SortInfluences(influences);

            // remove all influences
            for (size_t i = 0; i < numInfluences; ++i)
            {
                mInfluences.Remove(v, 0);
            }

            // re-add them
            for (const Influence& influence : influences)
            {
                mInfluences.Add(v, influence);
            }
        }
    }
} // namespace AZ::MeshBuilder
