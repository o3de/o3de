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
#include "SkinningInfoVertexAttributeLayer.h"
#include "Actor.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SkinningInfoVertexAttributeLayer, MeshAllocator, 0)


    // constructor
    SkinningInfoVertexAttributeLayer::SkinningInfoVertexAttributeLayer(uint32 numAttributes, bool allocData)
        : VertexAttributeLayer(numAttributes, false)
    {
        if (allocData)
        {
            mData.SetNumPreCachedElements(2); // assume 2 weights per vertex
            mData.Resize(numAttributes);
        }

        // setup the memory category
        mData.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_VERTEXATTRIBUTES);
    }


    // destructor
    SkinningInfoVertexAttributeLayer::~SkinningInfoVertexAttributeLayer()
    {
    }


    // create
    SkinningInfoVertexAttributeLayer* SkinningInfoVertexAttributeLayer::Create(uint32 numAttributes, bool allocData)
    {
        return aznew SkinningInfoVertexAttributeLayer(numAttributes, allocData);
    }


    // get the unique layer type
    uint32 SkinningInfoVertexAttributeLayer::GetType() const
    {
        return TYPE_ID;
    }


    // get the description of the vertex attributes or layer
    const char* SkinningInfoVertexAttributeLayer::GetTypeString() const
    {
        return "SkinningInfoVertexAttribute";
    }


    // reset to original data, but there isn't any for this layer, so it doesn't do anything
    void SkinningInfoVertexAttributeLayer::ResetToOriginalData()
    {
    }


    // add a given influence (using a bone and a weight)
    void SkinningInfoVertexAttributeLayer::AddInfluence(uint32 attributeNr, uint32 nodeNr, float weight, uint32 boneNr)
    {
        mData.Add(attributeNr, SkinInfluence(static_cast<uint16>(nodeNr), weight, static_cast<uint16>(boneNr)));
    }


    // remove the given skin influence
    void SkinningInfoVertexAttributeLayer::RemoveInfluence(uint32 attributeNr, uint32 influenceNr)
    {
        mData.Remove(attributeNr, influenceNr);
    }


    // the uv vertex attribute layer
    VertexAttributeLayer* SkinningInfoVertexAttributeLayer::Clone()
    {
        SkinningInfoVertexAttributeLayer* clone = aznew SkinningInfoVertexAttributeLayer(mNumAttributes);

        // copy over the data
        clone->mData    = mData;
        clone->mNameID  = mNameID;

        // return the clone
        return clone;
    }


    // swap attribute data data
    void SkinningInfoVertexAttributeLayer::SwapAttributes(uint32 attribA, uint32 attribB)
    {
        mData.Swap(attribA, attribB);
    }


    // remove attributes
    void SkinningInfoVertexAttributeLayer::RemoveAttributes(uint32 startAttributeNr, uint32 endAttributeNr)
    {
        mData.RemoveRows(startAttributeNr, endAttributeNr, true);
    }


    // remap all influences for bone (oldNodeNr) to bone (newNodeNr)
    void SkinningInfoVertexAttributeLayer::RemapInfluences(uint32 oldNodeNr, uint32 newNodeNr)
    {
        // get the number of vertices/attributes
        const uint32 numAttributes = mData.GetNumRows();
        for (uint32 a = 0; a < numAttributes; ++a)
        {
            // iterate through all influences and compare them with the old node
            const uint32 numInfluences = GetNumInfluences(a);
            for (uint32 i = 0; i < numInfluences; ++i)
            {
                // remap the influence to the new node when it is linked to the old node
                if (GetInfluence(a, i)->GetNodeNr() == oldNodeNr)
                {
                    GetInfluence(a, i)->SetNodeNr(static_cast<uint16>(newNodeNr));
                }
            }
        }
    }


    // remove all influences which refer to the given node from our skinning info
    void SkinningInfoVertexAttributeLayer::RemoveAllInfluencesForNode(uint32 nodeNr)
    {
        // get the number of vertices/attributes
        const uint32 numAttributes = mData.GetNumRows();
        for (uint32 a = 0; a < numAttributes; ++a)
        {
            // iterate through all influences and compare them with the given node
            uint32 i = 0;
            while (i < GetNumInfluences(a))
            {
                // remove the influence when it is linked to the given node
                if (GetInfluence(a, i)->GetNodeNr() == nodeNr)
                {
                    RemoveInfluence(a, i);
                }
                else
                {
                    i++;
                }
            }
        }
    }


    // collect all nodes to which the skinning info refers to
    void SkinningInfoVertexAttributeLayer::CollectInfluencedNodes(MCore::Array<uint32>& influencedNodes, bool clearInfluencedNodesArray)
    {
        if (clearInfluencedNodesArray)
        {
            influencedNodes.Clear();
        }

        // get the number of vertices/attributes
        const uint32 numAttributes = mData.GetNumRows();
        for (uint32 a = 0; a < numAttributes; ++a)
        {
            // get the number of influences for the current vertex
            const uint32 numInfluences = GetNumInfluences(a);

            // iterate through all influences and compare them with the old node
            for (uint32 i = 0; i < numInfluences; ++i)
            {
                const uint32 influencedNodeNr = GetInfluence(a, i)->GetNodeNr();

                // check if the influenced node is already in our array, if not add it so that we only store each node once
                if (influencedNodes.Find(influencedNodeNr) == MCORE_INVALIDINDEX32)
                {
                    influencedNodes.Add(influencedNodeNr);
                }
            }
        }
    }


    // optimize the memory usage
    void SkinningInfoVertexAttributeLayer::OptimizeMemoryUsage()
    {
        mData.Shrink();
    }


    // optimize weights
    void SkinningInfoVertexAttributeLayer::OptimizeInfluences(float tolerance, uint32 maxWeights)
    {
        // get the number of vertices/attributes
        const uint32 numAttributes = mData.GetNumRows();
        for (uint32 a = 0; a < numAttributes; ++a)
        {
            if (GetNumInfluences(a) == 0)
            {
                continue; // vertex has no weights, so nothing to optimize
            }
            // remove all weights below the tolerance
            // at least keep one weight left after this optimization
            uint32 w;
            for (w = 0; w < GetNumInfluences(a); )
            {
                if (GetNumInfluences(a) == 1)
                {
                    break;
                }

                const float weight = GetInfluence(a, w)->GetWeight();
                if (weight < tolerance)
                {
                    RemoveInfluence(a, w);
                }
                else
                {
                    w++;
                }
            }


            // reduce number of weights when needed
            while (GetNumInfluences(a) > maxWeights)
            {
                float minWeight = 999999.9f;
                uint32 minInfluence = 0;

                // find the smallest weight
                const uint32 numInfluences = GetNumInfluences(a);
                for (uint32 i = 0; i < numInfluences; ++i)
                {
                    const float curWeight = GetInfluence(a, i)->GetWeight();
                    if (curWeight < minWeight)
                    {
                        minWeight = curWeight;
                        minInfluence = i;
                    }
                }

                // remove this smallest weight
                RemoveInfluence(a, minInfluence);
            }


            // calculate the total weight
            float totalWeight = 0;
            const uint32 numInfluences = GetNumInfluences(a);
            for (w = 0; w < numInfluences; ++w)
            {
                totalWeight += GetInfluence(a, w)->GetWeight();
            }

            // spread the weights over
            if (totalWeight < 1.0f)
            {
                const float remaining = 1.0f - totalWeight;
                for (uint32 i = 0; i < numInfluences; ++i)
                {
                    const float percentage = mData.GetElement(a, i).GetWeight() / totalWeight;
                    GetInfluence(a, i)->SetWeight(GetInfluence(a, i)->GetWeight() + percentage * remaining);
                }
            }
        }

        // optimize the skinning informations memory usage
        OptimizeMemoryUsage();
    }


    // collapse the influences when possible
    void SkinningInfoVertexAttributeLayer::CollapseInfluences(uint32 attributeNr)
    {
        const uint32 numInfluences = GetNumInfluences(attributeNr);
        if (numInfluences <= 1) // nothing to optimize if it is just one influence, or none at all
        {
            return;
        }

        // check if all influences use the same bone
        bool allTheSame = true;
        for (uint32 i = 0; i < numInfluences; ++i)
        {
            const SkinInfluence* influence = GetInfluence(attributeNr, i);
            if (influence->GetNodeNr() != GetInfluence(attributeNr, 0)->GetNodeNr())
            {
                allTheSame = false;
                break;
            }
        }

        // if not all the influences use the same bone, there is nothing to optimize
        if (allTheSame == false)
        {
            return;
        }

        //MCore::LogInfo("SkinningInfoVertexAttributeLayer::CollapseInfluences() - Optimizing attribute %d, from %d influences down to one", attributeNr, numInfluences);

        // remove all influences and just create one weight
        while (GetNumInfluences(attributeNr) > 1)
        {
            RemoveInfluence(attributeNr, 0);
        }
        MCORE_ASSERT(GetNumInfluences(attributeNr) == 1);

        // make the remaining influence have a weight of 1.0, to have full influence
        GetInfluence(attributeNr, 0)->SetWeight(1.0f);
    }
} // namespace EMotionFX
