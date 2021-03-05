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

#include <AzCore/std/numeric.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobContext.h>

#include <EMotionFX/Source/MeshBuilder.h>
#include <EMotionFX/Source/MeshBuilderSkinningInfo.h>
#include <EMotionFX/Source/MeshBuilderSubMesh.h>
#include <EMotionFX/Source/Allocators.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilder, MeshAllocator, 0)

    const int MeshBuilder::s_defaultMaxBonesPerSubMesh = 512;
    const int MeshBuilder::s_defaultMaxSubMeshVertices = 65535; // default: max 16-bit value

    MeshBuilder::MeshBuilder(size_t jointIndex, size_t numOrgVerts, bool optimizeDuplicates)
        : MeshBuilder(jointIndex, numOrgVerts, s_defaultMaxBonesPerSubMesh, s_defaultMaxSubMeshVertices, optimizeDuplicates)
    {
    }

    MeshBuilder::MeshBuilder(size_t jointIndex, size_t numOrgVerts, size_t maxBonesPerSubMesh, size_t maxSubMeshVertices, bool optimizeDuplicates)
    {
        m_jointIndex = jointIndex;
        m_numOrgVerts = numOrgVerts;
        m_maxBonesPerSubMesh = AZ::GetMax<size_t>(1, maxBonesPerSubMesh);
        m_maxSubMeshVertices = AZ::GetMax<size_t>(1, maxSubMeshVertices);
        m_optimizeDuplicates = optimizeDuplicates;

        m_vertices.resize(numOrgVerts);
    }

    MeshBuilder::~MeshBuilder()
    {
        for (MeshBuilderVertexAttributeLayer* layer : m_layers)
        {
            layer->Destroy();
        }

        if (m_skinningInfo)
        {
            m_skinningInfo->Destroy();
        }
    }

    MeshBuilderVertexAttributeLayer* MeshBuilder::FindLayer(uint32 layerID, uint32 occurrence) const
    {
        size_t count = 0;
        for (MeshBuilderVertexAttributeLayer* layer : m_layers)
        {
            if (layer->GetTypeID() == layerID)
            {
                if (occurrence == count)
                {
                    return layer;
                }
                else
                {
                    count++;
                }
            }
        }

        return nullptr;
    }

    size_t MeshBuilder::GetNumLayers(uint32 layerID) const
    {
        size_t count = 0;
        for (MeshBuilderVertexAttributeLayer* layer : m_layers)
        {
            if (layer->GetTypeID() == layerID)
            {
                count++;
            }
        }

        return count;
    }

    void MeshBuilder::AddLayer(MeshBuilderVertexAttributeLayer* layer)
    {
        m_layers.push_back(layer);
    }

    MeshBuilderVertexLookup MeshBuilder::FindMatchingDuplicate(size_t orgVertexNr)
    {
        // check with all vertex duplicates
        const uint32 numDuplicates = m_layers[0]->GetNumDuplicates(static_cast<uint32>(orgVertexNr));
        for (uint32 d = 0; d < numDuplicates; ++d)
        {
            // check if the submitted vertex data is equal in all layers for the current duplicate
            bool allDataEqual = true;
            const size_t numLayers = m_layers.size();
            for (size_t layer = 0; layer < numLayers && allDataEqual; ++layer)
            {
                if (m_layers[layer]->CheckIfIsVertexEqual(static_cast<uint32>(orgVertexNr), d) == false)
                {
                    allDataEqual = false;
                }
            }

            // if so, we have found a matching vertex!
            if (allDataEqual)
            {
                return MeshBuilderVertexLookup(static_cast<uint32>(orgVertexNr), d);
            }
        }

        // no matching vertex found
        return MeshBuilderVertexLookup();
    }

    MeshBuilderVertexLookup MeshBuilder::AddVertex(size_t orgVertexNr)
    {
        // when there are no layers, there is nothing to do
        const size_t numLayers = m_layers.size();
        if (numLayers == 0)
        {
            return MeshBuilderVertexLookup();
        }

        // try to find a matching duplicate number for the current vertex
        // if there isn't a similar vertex, we have to submit it to all layers
        MeshBuilderVertexLookup index;       
        if (m_optimizeDuplicates)
        {
            index = FindMatchingDuplicate(orgVertexNr);
        }

        if (index.mOrgVtx == MCORE_INVALIDINDEX32)
        {
            for (uint32 i = 0; i < numLayers; ++i)
            {
                m_layers[i]->AddVertex(static_cast<uint32>(orgVertexNr));
                index.mOrgVtx = static_cast<uint32>(orgVertexNr);
                index.mDuplicateNr = m_layers[i]->GetNumDuplicates(static_cast<uint32>(orgVertexNr)) - 1;
            }
        }

        return index;
    }

    // find the index value for the current set vertex
    MeshBuilderVertexLookup MeshBuilder::FindVertexIndex(size_t orgVertexNr)
    {
        // if there are no layers, we can't find a valid index
        if (m_layers.size() == 0)
        {
            return MeshBuilderVertexLookup();
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
        m_polyIndices.push_back(AddVertex(orgVertexNr));
        m_polyOrgVertexNumbers.push_back(orgVertexNr);
    }

    void MeshBuilder::EndPolygon()
    {
        AZ_Assert(m_polyIndices.size() >= 3, "Polygon should at least have three vertices.");

        // add the triangle
        AddPolygon(m_polyIndices, m_polyOrgVertexNumbers, m_materialIndex);
    }

    MeshBuilderSubMesh* MeshBuilder::FindSubMeshForPolygon(const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex)
    {
        // collect all bones that influence the given polygon
        AZStd::vector<size_t> polyJointList;
        ExtractBonesForPolygon(orgVertexNumbers, polyJointList);

        // create our list of possible submeshes, start value are all submeshes available
        AZStd::vector<MeshBuilderSubMesh*> possibleSubMeshes;
        possibleSubMeshes.reserve(m_subMeshes.size());
        for (const AZStd::unique_ptr<MeshBuilderSubMesh>& subMesh : m_subMeshes)
        {
            possibleSubMeshes.push_back(subMesh.get());
        }

        while (possibleSubMeshes.size() > 0)
        {
            size_t maxMatchings = 0;
            size_t foundSubMeshNr = InvalidIndex32;
            const size_t numPossibleSubMeshes  = possibleSubMeshes.size();

            // iterate over all submeshes and find the one with the most similar bones
            for (size_t i = 0; i < numPossibleSubMeshes; ++i)
            {
                // get the number of matching bones from the current submesh and the given polygon
                const size_t currentNumMatches = possibleSubMeshes[i]->CalcNumSimilarJoints(polyJointList);

                // if the current submesh has more similar bones than the current maximum we found a better one
                if (currentNumMatches > maxMatchings)
                {
                    // check if this submesh already our perfect match
                    if (currentNumMatches == polyJointList.size())
                    {
                        // check if the submesh which has the most common bones with the given polygon can handle it
                        if (possibleSubMeshes[i]->CanHandlePolygon(orgVertexNumbers, materialIndex, polyJointList))
                        {
                            return possibleSubMeshes[i];
                        }
                    }

                    maxMatchings    = currentNumMatches;
                    foundSubMeshNr  = i;
                }
            }

            // if we cannot find a submesh return nullptr and create a new one
            if (foundSubMeshNr == MCORE_INVALIDINDEX32)
            {
                for (MeshBuilderSubMesh* possibleSubMesh : possibleSubMeshes)
                {
                    // check if the submesh which has the most common bones with the given polygon can handle it
                    if (possibleSubMesh->CanHandlePolygon(orgVertexNumbers, materialIndex, m_polyJointList))
                    {
                        return possibleSubMesh;
                    }
                }

                return nullptr;
            }

            // check if the submesh which has the most common bones with the given polygon can handle it
            if (possibleSubMeshes[foundSubMeshNr]->CanHandlePolygon(orgVertexNumbers, materialIndex, m_polyJointList))
            {
                return possibleSubMeshes[foundSubMeshNr];
            }

            // remove the found submesh from the possible submeshes directly so that we can don't find the same one in the next iteration again
            possibleSubMeshes.erase(possibleSubMeshes.begin() + foundSubMeshNr);
        }

        return nullptr;
    }

    void MeshBuilder::AddPolygon(const AZStd::vector<MeshBuilderVertexLookup>& indices, const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex)
    {
        // add the polygon to the list of poly vertex counts
        const size_t numPolyVerts = indices.size();
        AZ_Assert(numPolyVerts <= 255, "Polygon has too many vertices.");
        m_polyVertexCounts.push_back(static_cast<AZ::u8>(numPolyVerts));

        // try to find a submesh where we can add it
        MeshBuilderSubMesh* subMesh = FindSubMeshForPolygon(orgVertexNumbers, materialIndex);

        // if there is none where we can add it to, create a new one
        if (!subMesh)
        {
            subMesh = aznew MeshBuilderSubMesh(materialIndex, this);
            m_subMeshes.push_back(AZStd::unique_ptr<MeshBuilderSubMesh>(subMesh));
        }

        // add the polygon to the submesh
        ExtractBonesForPolygon(orgVertexNumbers, m_polyJointList);
        subMesh->AddPolygon(indices, m_polyJointList);
    }

    void MeshBuilder::OptimizeTriangleList()
    {
        if (!CheckIfIsTriangleMesh())
        {
            return;
        }

        for (const AZStd::unique_ptr<MeshBuilderSubMesh>& subMesh : m_subMeshes)
        {
            subMesh->Optimize();
        }
    }

    void MeshBuilder::LogContents()
    {
        AZ_Printf("EMotionFX", "---------------------------------------------------------------------------");
        AZ_Printf("EMotionFX", "Mesh for joint nr %d", m_jointIndex);
        const size_t numLayers = m_layers.size();
        AZ_Printf("EMotionFX", "Num org verts = %d", m_numOrgVerts);
        AZ_Printf("EMotionFX", "Num layers    = %d", numLayers);
        AZ_Printf("EMotionFX", "Num polys     = %d", GetNumPolygons());
        AZ_Printf("EMotionFX", "IsTriMesh     = %d", CheckIfIsTriangleMesh());
        AZ_Printf("EMotionFX", "IsQuaddMesh   = %d", CheckIfIsQuadMesh());

        for (size_t i = 0; i < numLayers; ++i)
        {
            MeshBuilderVertexAttributeLayer* layer = m_layers[i];
            AZ_Printf("EMotionFX", "Layer #%d:", i);
            AZ_Printf("EMotionFX", "  - Type type ID   = %d", layer->GetTypeID());
            AZ_Printf("EMotionFX", "  - Num vertices   = %d", layer->CalcNumVertices());
            AZ_Printf("EMotionFX", "  - Attribute size = %d bytes", layer->GetAttributeSizeInBytes());
            AZ_Printf("EMotionFX", "  - Layer size     = %d bytes", layer->CalcLayerSizeInBytes());
            AZ_Printf("EMotionFX", "  - Name           = %s", layer->GetName());
        }
        AZ_Printf("EMotionFX", "");

        const size_t numSubMeshes = m_subMeshes.size();
        AZ_Printf("EMotionFX", "Num Submeshes = %d", numSubMeshes);
        for (uint32 i = 0; i < numSubMeshes; ++i)
        {
            MeshBuilderSubMesh* subMesh = m_subMeshes[i].get();
            AZ_Printf("EMotionFX", "Submesh #%d:", i);
            AZ_Printf("EMotionFX", "  - Material    = %d", subMesh->GetMaterialIndex());
            AZ_Printf("EMotionFX", "  - Num vertices= %d", subMesh->GetNumVertices());
            AZ_Printf("EMotionFX", "  - Num indices = %d (%d polys)", subMesh->GetNumIndices(), subMesh->GetNumPolygons());
            AZ_Printf("EMotionFX", "  - Num joints  = %d", subMesh->GetNumJoints());
        }
    }

    void MeshBuilder::ExtractBonesForPolygon(const AZStd::vector<size_t>& orgVertexNumbers, AZStd::vector<size_t>& outPolyJointList) const
    {
        // get rid of existing data
        outPolyJointList.clear();

        // get the skinning info, if there is any
        MeshBuilderSkinningInfo* skinningInfo = GetSkinningInfo();
        if (skinningInfo == nullptr)
        {
            return;
        }

        // for all 3 vertices of the polygon
        for (size_t orgVtxNr : orgVertexNumbers)
        {
            // traverse all influences for this vertex
            const uint32 numInfluences = skinningInfo->GetNumInfluences(static_cast<uint32>(orgVtxNr));
            for (uint32 n = 0; n < numInfluences; ++n)
            {
                const size_t nodeNr = skinningInfo->GetInfluence(static_cast<uint32>(orgVtxNr), n).mNodeNr;

                // if it isn't yet in the output array with bones, add it
                if (AZStd::find(outPolyJointList.begin(), outPolyJointList.end(), nodeNr) == outPolyJointList.end())
                {
                    outPolyJointList.push_back(nodeNr);
                }
            }
        }
    }

    void MeshBuilder::OptimizeMemoryUsage()
    {
        for (MeshBuilderVertexAttributeLayer* layer : m_layers)
        {
            layer->OptimizeMemoryUsage();
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
            AZ::Job* job = AZ::CreateJobFunction([this, &subMesh]()
            {
                AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::Animation, "MeshBuilder::GenerateSubMeshVertexOrders::SubMeshJob");
                subMesh->GenerateVertexOrder();
            }, true, jobContext);

            job->SetDependent(&jobCompletion);               
            job->Start();
        }

        jobCompletion.StartAndWaitForCompletion();
    }

    bool MeshBuilder::CheckIfIsTriangleMesh() const
    {
        return AZStd::all_of(begin(m_polyVertexCounts), end(m_polyVertexCounts), [](AZ::u8 count) { return count == 3; });
    }

    bool MeshBuilder::CheckIfIsQuadMesh() const
    {
        return AZStd::all_of(begin(m_polyVertexCounts), end(m_polyVertexCounts), [](AZ::u8 count) { return count == 4; });
    }

    size_t MeshBuilder::FindRealVertexNr(MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr)
    {
        for (const SubMeshVertex& subMeshVertex : m_vertices[orgVtx])
        {
            if (subMeshVertex.m_subMesh == subMesh && subMeshVertex.m_dupeNr == dupeNr)
            {
                return subMeshVertex.m_realVertexNr;
            }
        }

        return InvalidIndex32;
    }

    MeshBuilder::SubMeshVertex* MeshBuilder::FindSubMeshVertex(MeshBuilderSubMesh* subMesh, size_t orgVtx, size_t dupeNr)
    {
        for (SubMeshVertex& subMeshVertex : m_vertices[orgVtx])
        {
            if (subMeshVertex.m_subMesh == subMesh && subMeshVertex.m_dupeNr == dupeNr)
            {
                return &subMeshVertex;
            }
        }

        return nullptr;
    }

    size_t MeshBuilder::CalcNumVertexDuplicates(MeshBuilderSubMesh* subMesh, size_t orgVtx) const
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

    size_t MeshBuilder::GetNumSubMeshVertices(size_t orgVtx)
    {
        return m_vertices[orgVtx].size();
    }

    void MeshBuilder::AddSubMeshVertex(size_t orgVtx, const SubMeshVertex& vtx)
    {
        m_vertices[orgVtx].push_back(vtx);
    }
} // namespace EMotionFX
