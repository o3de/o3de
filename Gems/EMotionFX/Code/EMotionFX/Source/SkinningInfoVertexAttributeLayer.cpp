/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "SkinningInfoVertexAttributeLayer.h"
#include "Actor.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SkinningInfoVertexAttributeLayer, MeshAllocator)


    // constructor
    SkinningInfoVertexAttributeLayer::SkinningInfoVertexAttributeLayer(uint32 numAttributes, bool allocData)
        : VertexAttributeLayer(numAttributes, false)
    {
        if (allocData)
        {
            m_data.SetNumPreCachedElements(2); // assume 2 weights per vertex
            m_data.Resize(numAttributes);
        }
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
    void SkinningInfoVertexAttributeLayer::AddInfluence(size_t attributeNr, size_t nodeNr, float weight, size_t boneNr)
    {
        m_data.Add(attributeNr, SkinInfluence(static_cast<uint16>(nodeNr), weight, static_cast<uint16>(boneNr)));
    }


    // remove the given skin influence
    void SkinningInfoVertexAttributeLayer::RemoveInfluence(size_t attributeNr, size_t influenceNr)
    {
        m_data.Remove(attributeNr, influenceNr);
    }


    // the uv vertex attribute layer
    VertexAttributeLayer* SkinningInfoVertexAttributeLayer::Clone()
    {
        SkinningInfoVertexAttributeLayer* clone = aznew SkinningInfoVertexAttributeLayer(m_numAttributes);

        // copy over the data
        clone->m_data    = m_data;
        clone->m_nameId  = m_nameId;

        // return the clone
        return clone;
    }


    // swap attribute data data
    void SkinningInfoVertexAttributeLayer::SwapAttributes(uint32 attribA, uint32 attribB)
    {
        m_data.Swap(attribA, attribB);
    }


    // remove attributes
    void SkinningInfoVertexAttributeLayer::RemoveAttributes(uint32 startAttributeNr, uint32 endAttributeNr)
    {
        m_data.RemoveRows(startAttributeNr, endAttributeNr, true);
    }


    // remap all influences for bone (oldNodeNr) to bone (newNodeNr)
    void SkinningInfoVertexAttributeLayer::RemapInfluences(size_t oldNodeNr, size_t newNodeNr)
    {
        // get the number of vertices/attributes
        const size_t numAttributes = m_data.GetNumRows();
        for (size_t a = 0; a < numAttributes; ++a)
        {
            // iterate through all influences and compare them with the old node
            const size_t numInfluences = GetNumInfluences(a);
            for (size_t i = 0; i < numInfluences; ++i)
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
    void SkinningInfoVertexAttributeLayer::RemoveAllInfluencesForNode(size_t nodeNr)
    {
        // get the number of vertices/attributes
        const size_t numAttributes = m_data.GetNumRows();
        for (size_t a = 0; a < numAttributes; ++a)
        {
            // iterate through all influences and compare them with the given node
            size_t i = 0;
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
    void SkinningInfoVertexAttributeLayer::CollectInfluencedNodes(AZStd::vector<uint32>& influencedNodes, bool clearInfluencedNodesArray)
    {
        if (clearInfluencedNodesArray)
        {
            influencedNodes.clear();
        }

        // get the number of vertices/attributes
        const size_t numAttributes = m_data.GetNumRows();
        for (size_t a = 0; a < numAttributes; ++a)
        {
            // get the number of influences for the current vertex
            const size_t numInfluences = GetNumInfluences(a);

            // iterate through all influences and compare them with the old node
            for (size_t i = 0; i < numInfluences; ++i)
            {
                const size_t influencedNodeNr = GetInfluence(a, i)->GetNodeNr();

                // check if the influenced node is already in our array, if not add it so that we only store each node once
                if (AZStd::find(begin(influencedNodes), end(influencedNodes), influencedNodeNr) == end(influencedNodes))
                {
                    influencedNodes.emplace_back(aznumeric_cast<uint32>(influencedNodeNr));
                }
            }
        }
    }


    // optimize the memory usage
    void SkinningInfoVertexAttributeLayer::OptimizeMemoryUsage()
    {
        m_data.Shrink();
    }


    // optimize weights
    void SkinningInfoVertexAttributeLayer::OptimizeInfluences(float tolerance, size_t maxWeights)
    {
        // get the number of vertices/attributes
        const size_t numAttributes = m_data.GetNumRows();
        for (size_t a = 0; a < numAttributes; ++a)
        {
            if (GetNumInfluences(a) == 0)
            {
                continue; // vertex has no weights, so nothing to optimize
            }
            // remove all weights below the tolerance
            // at least keep one weight left after this optimization
            size_t w;
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
                size_t minInfluence = 0;

                // find the smallest weight
                const size_t numInfluences = GetNumInfluences(a);
                for (size_t i = 0; i < numInfluences; ++i)
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
            const size_t numInfluences = GetNumInfluences(a);
            for (w = 0; w < numInfluences; ++w)
            {
                totalWeight += GetInfluence(a, w)->GetWeight();
            }

            // spread the weights over
            if (totalWeight < 1.0f)
            {
                const float remaining = 1.0f - totalWeight;
                for (size_t i = 0; i < numInfluences; ++i)
                {
                    const float percentage = m_data.GetElement(a, i).GetWeight() / totalWeight;
                    GetInfluence(a, i)->SetWeight(GetInfluence(a, i)->GetWeight() + percentage * remaining);
                }
            }
        }

        // optimize the skinning informations memory usage
        OptimizeMemoryUsage();
    }


    // collapse the influences when possible
    void SkinningInfoVertexAttributeLayer::CollapseInfluences(size_t attributeNr)
    {
        const size_t numInfluences = GetNumInfluences(attributeNr);
        if (numInfluences <= 1) // nothing to optimize if it is just one influence, or none at all
        {
            return;
        }

        // check if all influences use the same bone
        bool allTheSame = true;
        for (size_t i = 0; i < numInfluences; ++i)
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

    AZStd::set<uint16> SkinningInfoVertexAttributeLayer::CalcLocalJointIndices(AZ::u32 numOrgVertices)
    {
        AZStd::set<uint16> result;

        for (AZ::u32 i = 0; i < numOrgVertices; i++)
        {
            // now we have located the skinning information for this vertex, we can see if our bones array
            // already contains the bone it uses by traversing all influences for this vertex, and checking
            // if the bone of that influence already is in the array with used bones
            const size_t numInfluences = GetNumInfluences(i);
            for (size_t a = 0; a < numInfluences; ++a)
            {
                EMotionFX::SkinInfluence* influence = GetInfluence(i, a);
                const uint16 jointNr = influence->GetNodeNr();

                result.emplace(jointNr);
            }
        }

        return result;
    }
} // namespace EMotionFX
