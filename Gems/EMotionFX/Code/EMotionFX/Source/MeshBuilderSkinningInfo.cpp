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
#include "MeshBuilderSkinningInfo.h"
#include "MeshBuilderSubMesh.h"
#include "MeshBuilder.h"
#include "MeshBuilderVertexAttributeLayers.h"
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilderSkinningInfo, MeshAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilderSkinningInfo::Influence, MeshAllocator, 0)


    // constructor
    MeshBuilderSkinningInfo::MeshBuilderSkinningInfo(uint32 numOrgVertices)
        : BaseObject()
    {
        mInfluences.SetNumPreCachedElements(4); // TODO: verify if this is the fastest
        mInfluences.Resize(numOrgVertices);
        mInfluences.SetMemoryCategory(EMFX_MEMCATEGORY_MESHBUILDER_SKINNINGINFO);
    }


    // destructor
    MeshBuilderSkinningInfo::~MeshBuilderSkinningInfo()
    {
    }


    // creation
    MeshBuilderSkinningInfo* MeshBuilderSkinningInfo::Create(uint32 numOrgVertices)
    {
        return aznew MeshBuilderSkinningInfo(numOrgVertices);
    }


    // optimize weights
    void MeshBuilderSkinningInfo::OptimizeSkinningInfluences(MCore::Array<Influence>& influences, float tolerance, uint32 maxWeights)
    {
        if (influences.GetLength() == 0)
        {
            return; // vertex has no weights, so nothing to optimize
        }
        // remove all weights below the tolerance
        // at least keep one weight left after this optimization
        uint32 w;
        for (w = 0; w < influences.GetLength(); )
        {
            if (influences.GetLength() == 1)
            {
                break;
            }

            float weight = influences[w].mWeight;
            if (weight < tolerance)
            {
                influences.Remove(w);
            }
            else
            {
                w++;
            }
        }


        // reduce number of weights when needed
        while (influences.GetLength() > maxWeights)
        {
            float minWeight = FLT_MAX;
            uint32 minInfluence = 0;

            // find the smallest weight
            for (uint32 i = 0; i < influences.GetLength(); i++)
            {
                if (influences[i].mWeight < minWeight)
                {
                    minWeight = influences[i].mWeight;
                    minInfluence = i;
                }
            }

            // remove this smallest weight
            influences.Remove(minInfluence);
        }


        // calculate the total weight
        float totalWeight = 0;
        for (w = 0; w < influences.GetLength(); w++)
        {
            totalWeight += influences[w].mWeight;
        }
        /*
            // spread the weights over
            if (totalWeight < 1)
            {
                float remaining = 1.0 - totalWeight;
                for (w=0; w<influences.GetLength(); w++)
                {
                    float percentage = influences[w].mWeight / totalWeight;
                    influences[w].mWeight += percentage * remaining;
                }
            }*/

        // normalize
        for (w = 0; w < influences.GetLength(); w++)
        {
            influences[w].mWeight /= totalWeight;
        }
    }


    // the global weight compare function, used for sorting
    int32 WeightCompareFunction(const MeshBuilderSkinningInfo::Influence& influenceA, const MeshBuilderSkinningInfo::Influence& influenceB)
    {
        if (influenceA.mWeight < influenceB.mWeight)
        {
            return +1;
        }

        if (influenceA.mWeight > influenceB.mWeight)
        {
            return -1;
        }

        return 0;
    }


    // sort influences on weights, from big to small
    void MeshBuilderSkinningInfo::SortInfluences(MCore::Array<Influence>& influences)
    {
        influences.Sort(WeightCompareFunction);
    }


    void CopySkinningInfo(MeshBuilderSkinningInfo* from, MeshBuilderSkinningInfo* to)
    {
        if (from == nullptr || to == nullptr)
        {
            return;
        }

        // get the number of original vertices
        const uint32 numOrgVerts = from->GetNumOrgVertices();
        for (uint32 v = 0; v < numOrgVerts; ++v)
        {
            const uint32 weightCount = from->GetNumInfluences(v);
            for (uint32 w = 0; w < weightCount; ++w)
            {
                uint32 nodeNr = from->GetInfluence(v, w).mNodeNr;
                float  weight = from->GetInfluence(v, w).mWeight;

                MeshBuilderSkinningInfo::Influence influence;
                influence.mNodeNr = nodeNr;
                influence.mWeight = weight;

                to->AddInfluence(v, influence);
            }
        }
    }


    // optimize the weight data
    void MeshBuilderSkinningInfo::Optimize(uint32 maxNumWeightsPerVertex, float weightThreshold)
    {
        MCore::Array<Influence> influences;

        // for all vertices
        const uint32 numOrgVerts = GetNumOrgVertices();
        for (uint32 v = 0; v < numOrgVerts; ++v)
        {
            // gather all weights
            const uint32 numInfluences = GetNumInfluences(v);
            influences.Resize(numInfluences);
            for (uint32 i = 0; i < numInfluences; ++i)
            {
                influences[i] = GetInfluence(v, i);
            }

            // optimize the weights and sort them from big to small weight
            OptimizeSkinningInfluences(influences, weightThreshold, maxNumWeightsPerVertex);
            SortInfluences(influences);

            // remove all influences
            for (uint32 i = 0; i < numInfluences; ++i)
            {
                mInfluences.Remove(v, 0);
            }

            // re-add them
            const uint32 numNewInfluences = influences.GetLength();
            for (uint32 i = 0; i < numNewInfluences; ++i)
            {
                mInfluences.Add(v, influences[i]);
            }
        }
    }
} // namespace EMotionFX
