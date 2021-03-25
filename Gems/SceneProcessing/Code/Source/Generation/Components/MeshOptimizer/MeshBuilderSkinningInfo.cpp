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
#include <AzCore/std/sort.h>

#include <AzCore/Memory/SystemAllocator.h>
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
        if (influences.empty())
        {
            return; // vertex has no weights, so nothing to optimize
        }
        // remove all weights below the tolerance
        // at least keep one weight left after this optimization
        for (auto it = begin(influences); it != end(influences);)
        {
            if (influences.size() == 1)
            {
                break;
            }

            float weight = it->mWeight;
            if (weight < tolerance)
            {
                influences.erase(it);
            }
            else
            {
                ++it;
            }
        }


        // reduce number of weights when needed
        while (influences.size() > maxWeights)
        {
            float minWeight = FLT_MAX;
            AZStd::vector<Influence>::iterator minInfluence = end(influences);

            // find the smallest weight
            for (auto it = begin(influences); it != end(influences); ++it)
            {
                if (it->mWeight < minWeight)
                {
                    minWeight = it->mWeight;
                    minInfluence = it;
                }
            }

            // remove this smallest weight
            influences.erase(minInfluence);
        }


        // calculate the total weight
        float totalWeight = 0;
        for (const Influence& influence : influences)
        {
            totalWeight += influence.mWeight;
        }

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
