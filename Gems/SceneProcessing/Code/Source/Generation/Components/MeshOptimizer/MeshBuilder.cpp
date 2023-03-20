/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/numeric.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobContext.h>

#include "MeshBuilder.h"
#include "MeshBuilderSkinningInfo.h"
#include "MeshBuilderSubMesh.h"

namespace AZ::MeshBuilder
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilder, AZ::SystemAllocator)

    MeshBuilder::MeshBuilder(size_t numOrgVerts, bool optimizeDuplicates)
        : MeshBuilder(numOrgVerts, s_defaultMaxBonesPerSubMesh, s_defaultMaxSubMeshVertices, optimizeDuplicates)
    {
    }

    MeshBuilder::MeshBuilder(size_t numOrgVerts, size_t maxBonesPerSubMesh, size_t maxSubMeshVertices, bool optimizeDuplicates)
        : m_vertices(numOrgVerts)
        , m_maxBonesPerSubMesh(AZ::GetMax<size_t>(1, maxBonesPerSubMesh))
        , m_maxSubMeshVertices(AZ::GetMax<size_t>(1, maxSubMeshVertices))
        , m_numOrgVerts(numOrgVerts)
        , m_optimizeDuplicates(optimizeDuplicates)
    {
    }

    const MeshBuilderVertexLookup MeshBuilder::FindMatchingDuplicate(size_t orgVertexNr) const
    {
        // check with all vertex duplicates
        const size_t numDuplicates = m_layers[0]->GetNumDuplicates(orgVertexNr);
        for (size_t d = 0; d < numDuplicates; ++d)
        {
            // check if the submitted vertex data is equal in all layers for the current duplicate
            const bool allDataEqual = AZStd::all_of(begin(m_layers), end(m_layers), [orgVertexNr, d](const AZStd::unique_ptr<MeshBuilderVertexAttributeLayer>& layer)
            {
                return layer->CheckIfIsVertexEqual(orgVertexNr, d);
            });

            // if so, we have found a matching vertex!
            if (allDataEqual)
            {
                return {orgVertexNr, d};
            }
        }

        // no matching vertex found
        return {};
    }

    const MeshBuilderVertexLookup MeshBuilder::AddVertex(const size_t orgVertexNr)
    {
        // when there are no layers, there is nothing to do
        if (m_layers.empty())
        {
            return {};
        }

        // try to find a matching duplicate number for the current vertex
        const MeshBuilderVertexLookup index = m_optimizeDuplicates ? FindMatchingDuplicate(orgVertexNr) : MeshBuilderVertexLookup{};

        if (index.mOrgVtx != InvalidIndex)
        {
            return index;
        }

        // if there isn't a similar vertex, we have to submit it to all layers
        for (AZStd::unique_ptr<MeshBuilderVertexAttributeLayer>& layer : m_layers)
        {
            layer->AddVertex(orgVertexNr);
        }
        return {orgVertexNr, m_layers.back()->GetNumDuplicates(orgVertexNr) - 1};
    }

    // find the index value for the current set vertex
    const MeshBuilderVertexLookup MeshBuilder::FindVertexIndex(size_t orgVertexNr) const
    {
        // if there are no layers, we can't find a valid index
        if (m_layers.empty())
        {
            return {};
        }

        // try to locate a matching duplicate
        return FindMatchingDuplicate(orgVertexNr);
    }

    void MeshBuilder::BeginPolygon(size_t materialIndex)
    {
        m_materialIndex = materialIndex;
        m_polyIndices.clear();
        m_polyOrgVertexNumbers.clear();
    }

    void MeshBuilder::AddPolygonVertex(size_t orgVertexNr)
    {
        m_polyIndices.emplace_back(AddVertex(orgVertexNr));
        m_polyOrgVertexNumbers.emplace_back(orgVertexNr);
    }

    void MeshBuilder::EndPolygon()
    {
        AZ_Assert(m_polyIndices.size() >= 3, "Polygon should at least have three vertices.");

        // add the triangle
        AddPolygon(m_polyIndices, m_polyOrgVertexNumbers, m_materialIndex);
    }

    const MeshBuilderSubMesh* MeshBuilder::FindSubMeshForPolygon(const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex) const
    {
        // collect all bones that influence the given polygon
        AZStd::vector<size_t> polyJointList;
        ExtractBonesForPolygon(orgVertexNumbers, polyJointList);

        // find the submesh with the most similar bones
        size_t maxMatchings = 0;
        const auto* bestMatchingSubMesh = m_subMeshes.cend();
        for (const auto* subMesh = m_subMeshes.cbegin(); subMesh != m_subMeshes.cend(); ++subMesh)
        {
            // get the number of matching bones from the current submesh and the given polygon
            const size_t currentNumMatches = (*subMesh)->CalcNumSimilarJoints(polyJointList);

            // if the current submesh has more similar bones than the current maximum we found a better one
            if (currentNumMatches > maxMatchings)
            {
                maxMatchings = currentNumMatches;
                bestMatchingSubMesh = subMesh;

                // check if this submesh already our perfect match
                if (currentNumMatches == polyJointList.size())
                {
                    break;
                }
            }
        }

        // if we cannot find a submesh with enough similar joints, find any one that can handle the polygon
        if (bestMatchingSubMesh == m_subMeshes.cend())
        {
            bestMatchingSubMesh = AZStd::find_if(m_subMeshes.cbegin(), m_subMeshes.cend(), [&orgVertexNumbers, &materialIndex, &polyJointList](const auto& subMesh)
            {
                return subMesh->CanHandlePolygon(orgVertexNumbers, materialIndex, polyJointList);
            });

            if (bestMatchingSubMesh != m_subMeshes.cend())
            {
                return (*bestMatchingSubMesh).get();
            }
            return nullptr;
        }

        // check if the submesh which has the most common bones with the given polygon can handle it
        if ((*bestMatchingSubMesh)->CanHandlePolygon(orgVertexNumbers, materialIndex, polyJointList))
        {
            return (*bestMatchingSubMesh).get();
        }

        return nullptr;
    }

    void MeshBuilder::AddPolygon(const AZStd::vector<MeshBuilderVertexLookup>& indices, const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex)
    {
        // add the polygon to the list of poly vertex counts
        const size_t numPolyVerts = indices.size();
        AZ_Assert(numPolyVerts <= 255, "Polygon has too many vertices.");
        m_polyVertexCounts.emplace_back(static_cast<AZ::u8>(numPolyVerts));

        // try to find a submesh where we can add it
        MeshBuilderSubMesh* subMesh = FindSubMeshForPolygon(orgVertexNumbers, materialIndex);

        // if there is none where we can add it to, create a new one
        if (!subMesh)
        {
            m_subMeshes.emplace_back(AZStd::make_unique<MeshBuilderSubMesh>(materialIndex, this));
            subMesh = m_subMeshes.back().get();
        }

        // add the polygon to the submesh
        ExtractBonesForPolygon(orgVertexNumbers, m_polyJointList);
        subMesh->AddPolygon(indices, m_polyJointList);
    }

    void MeshBuilder::ExtractBonesForPolygon(const AZStd::vector<size_t>& orgVertexNumbers, AZStd::vector<size_t>& outPolyJointList) const
    {
        // get rid of existing data
        outPolyJointList.clear();

        // get the skinning info, if there is any
        const MeshBuilderSkinningInfo* skinningInfo = GetSkinningInfo();
        if (skinningInfo == nullptr)
        {
            return;
        }

        // for all 3 vertices of the polygon
        for (size_t orgVtxNr : orgVertexNumbers)
        {
            // traverse all influences for this vertex
            const size_t numInfluences = skinningInfo->GetNumInfluences(orgVtxNr);
            for (size_t n = 0; n < numInfluences; ++n)
            {
                const size_t nodeNr = skinningInfo->GetInfluence(orgVtxNr, n).mNodeNr;

                // if it isn't yet in the output array with bones, add it
                if (AZStd::find(outPolyJointList.begin(), outPolyJointList.end(), nodeNr) == outPolyJointList.end())
                {
                    outPolyJointList.emplace_back(nodeNr);
                }
            }
        }
    }

    size_t MeshBuilder::CalcNumIndices() const
    {
        size_t totalIndices = 0;
        for (const AZStd::unique_ptr<MeshBuilderSubMesh>& subMesh : m_subMeshes)
        {
            totalIndices += subMesh->GetNumIndices();
        }
        return totalIndices;
    }

    size_t MeshBuilder::CalcNumVertices() const
    {
        size_t total = 0;
        for (const AZStd::unique_ptr<MeshBuilderSubMesh>& subMesh : m_subMeshes)
        {
            total += subMesh->GetNumVertices();
        }
        return total;
    }

    void MeshBuilder::GenerateSubMeshVertexOrders()
    {
        AZ::JobCompletion jobCompletion;           

        for (const AZStd::unique_ptr<MeshBuilderSubMesh>& subMesh : m_subMeshes)
        {
            AZ::JobContext* jobContext = nullptr;
            AZ::Job* job = AZ::CreateJobFunction([&subMesh]()
            {
                AZ_PROFILE_SCOPE(Animation, "MeshBuilder::GenerateSubMeshVertexOrders::SubMeshJob");
                subMesh->GenerateVertexOrder();
            }, true, jobContext);

            job->SetDependent(&jobCompletion);               
            job->Start();
        }

        jobCompletion.StartAndWaitForCompletion();
    }

    void MeshBuilder::SetSkinningInfo(AZStd::unique_ptr<MeshBuilderSkinningInfo> skinningInfo)
    {
        m_skinningInfo = AZStd::move(skinningInfo);
    }

    size_t MeshBuilder::FindRealVertexNr(const MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr) const
    {
        for (const SubMeshVertex& subMeshVertex : m_vertices[orgVtx])
        {
            if (subMeshVertex.m_subMesh == subMesh && subMeshVertex.m_dupeNr == dupeNr)
            {
                return subMeshVertex.m_realVertexNr;
            }
        }

        return InvalidIndex;
    }

    void MeshBuilder::SetRealVertexNrForSubMeshVertex(const MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr, size_t realVertexNr)
    {
        SubMeshVertex* subMeshVertex = FindSubMeshVertex(subMesh, orgVtx, dupeNr);
        if (!subMeshVertex)
        {
            return;
        }
        subMeshVertex->m_realVertexNr = realVertexNr;
    }


    const MeshBuilder::SubMeshVertex* MeshBuilder::FindSubMeshVertex(const MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr) const
    {
        auto subMeshVertex = AZStd::find_if(m_vertices[orgVtx].begin(), m_vertices[orgVtx].end(), [subMesh, dupeNr](const SubMeshVertex& vertex)
        {
            return vertex.m_subMesh == subMesh && vertex.m_dupeNr == dupeNr;
        });
        return (subMeshVertex == m_vertices[orgVtx].end()) ? nullptr : subMeshVertex;
    }

    size_t MeshBuilder::CalcNumVertexDuplicates(const MeshBuilderSubMesh* subMesh, size_t orgVtx) const
    {
        size_t numDupes = 0;
        for (const SubMeshVertex& subMeshVertex : m_vertices[orgVtx])
        {
            if (subMeshVertex.m_subMesh == subMesh)
            {
                numDupes++;
            }
        }

        return numDupes;
    }

    size_t MeshBuilder::GetNumSubMeshVertices(size_t orgVtx) const
    {
        return m_vertices[orgVtx].size();
    }

    void MeshBuilder::AddSubMeshVertex(size_t orgVtx, SubMeshVertex&& vtx)
    {
        m_vertices[orgVtx].emplace_back(vtx);
    }
} // namespace AZ::MeshBuilder
