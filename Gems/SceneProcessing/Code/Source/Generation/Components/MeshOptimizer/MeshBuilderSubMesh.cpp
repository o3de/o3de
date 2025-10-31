/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Casting/numeric_cast.h>
#include "MeshBuilder.h"
#include "MeshBuilderSkinningInfo.h"
#include "MeshBuilderSubMesh.h"
#include "MeshBuilderVertexAttributeLayers.h"

namespace AZ::MeshBuilder
{
    AZ_CLASS_ALLOCATOR_IMPL(MeshBuilderSubMesh, AZ::SystemAllocator)

    MeshBuilderSubMesh::MeshBuilderSubMesh(size_t materialIndex, MeshBuilder* mesh)
        : m_materialIndex(materialIndex)
        , m_mesh(mesh)
    {
    }

    // map the vertices to their original vertex and dupe numbers
    void MeshBuilderSubMesh::GenerateVertexOrder()
    {
        // create our vertex order array and allocate numVertices lookups
        m_vertexOrder.resize(m_numVertices);

        const size_t numOrgVertices = m_mesh->GetNumOrgVerts();
        for (size_t orgVertexNr = 0; orgVertexNr < numOrgVertices; ++orgVertexNr)
        {
            const size_t numSubMeshVertices = m_mesh->GetNumSubMeshVertices(orgVertexNr);
            for (size_t i = 0; i < numSubMeshVertices; ++i)
            {
                const MeshBuilder::SubMeshVertex& subMeshVertex = m_mesh->GetSubMeshVertex(orgVertexNr, i);

                if (subMeshVertex.m_subMesh == this && subMeshVertex.m_realVertexNr != InvalidIndex)
                {
                    const size_t realVertexNr = subMeshVertex.m_realVertexNr;
                    m_vertexOrder[realVertexNr].mOrgVtx = orgVertexNr;
                    m_vertexOrder[realVertexNr].mDuplicateNr = subMeshVertex.m_dupeNr;
                }
            }
        }
    }

    // add a polygon to the submesh
    void MeshBuilderSubMesh::AddPolygon(const AZStd::vector<MeshBuilderVertexLookup>& indices, const AZStd::vector<size_t>& jointList)
    {
        // pre-allocate
        if (m_indices.size() % 10000 == 0)
        {
            m_indices.reserve(m_indices.size() + 10000);
        }

        // for all vertices in the poly
        m_polyVertexCounts.emplace_back(aznumeric_caster(indices.size()));

        for (const MeshBuilderVertexLookup& index : indices)
        {
            // add unused vertices to the list
            if (CheckIfHasVertex(index) == false)
            {
                const size_t numDupes = m_mesh->CalcNumVertexDuplicates(this, index.mOrgVtx);
                const ptrdiff_t numToAdd = (aznumeric_cast<ptrdiff_t>(index.mDuplicateNr) - aznumeric_cast<ptrdiff_t>(numDupes)) + 1;
                // Create entries in the whole Mesh's vertices array to fill in any missing entries, up to the current
                // vertex duplicate number. It is possible that this is duplicate 2 of original vertex 0, and duplicate
                // 1 has not been encountered yet. In this case, fill in an invalid index for duplicate 1, since we do
                // not yet know which submesh that duplicate belongs to.
                for (ptrdiff_t i = 0; i < numToAdd; ++i)
                {
                    const size_t realVertexNr = ((numDupes + i) == index.mDuplicateNr) ? m_numVertices : InvalidIndex;
                    m_mesh->AddSubMeshVertex(index.mOrgVtx, {
                        /* .m_realVertexNr = */ realVertexNr,
                        /* .m_dupeNr       = */ numDupes + i,
                        /* .m_subMesh      = */ this
                    });
                }
                if (numToAdd <= 0)
                {
                    // If none were added, the submesh vertex was added as a placeholder with an invalid index in a
                    // previous iteration by a different submesh. We now know which submesh this duplicate belongs to,
                    // so reset the real vertex index to this mesh's current vertex count.
                    m_mesh->SetRealVertexNrForSubMeshVertex(this, index.mOrgVtx, index.mDuplicateNr, m_numVertices);
                }
                m_numVertices++;
            }

            // an index in the local vertices array
            m_indices.emplace_back(index);
        }

        // add the new joints
        for (size_t jointIndex : jointList)
        {
            if (AZStd::find(m_jointList.begin(), m_jointList.end(), jointIndex) == m_jointList.end())
            {
                m_jointList.emplace_back(jointIndex);
            }
        }
    }

    // check if we can handle a given poly inside the submesh
    bool MeshBuilderSubMesh::CanHandlePolygon(const AZStd::vector<size_t>& orgVertexNumbers, size_t materialIndex, AZStd::vector<size_t>& outJointList) const
    {
        // if the material isn't the same, we can't handle it
        if (m_materialIndex != materialIndex)
        {
            return false;
        }

        // check if there is still space for the poly vertices (worst case scenario), and if this won't go over the 16 bit index buffer limit
        const size_t numPolyVerts = orgVertexNumbers.size();
        if (m_numVertices + numPolyVerts > m_mesh->m_maxSubMeshVertices)
        {
            return false;
        }

        const MeshBuilderSkinningInfo* skinningInfo = m_mesh->GetSkinningInfo();
        if (skinningInfo)
        {
            // get the maximum number of allowed bones per submesh
            const size_t maxNumBones = m_mesh->GetMaxBonesPerSubMesh();

            // extract the list of bones used by this poly
            m_mesh->ExtractBonesForPolygon(orgVertexNumbers, outJointList);

            // check if worst case scenario would be allowed
            // this is when we have to add all triangle bones to the bone list
            if (m_jointList.size() + outJointList.size() > maxNumBones)
            {
                return false;
            }

            // calculate the real number of extra bones needed
            size_t numExtraNeeded = 0;
            const size_t numPolyBones = outJointList.size();
            for (size_t i = 0; i < numPolyBones; ++i)
            {
                if (AZStd::find(m_jointList.begin(), m_jointList.end(), outJointList.at(i)) == m_jointList.end())
                {
                    numExtraNeeded++;
                }
            }

            // if we can't add the extra required bones to the list, because it would result in more than the
            // allowed number of bones, then return that we can't add this triangle to this submesh
            if (m_jointList.size() + numExtraNeeded > maxNumBones)
            {
                return false;
            }
        }

        // yeah, we can add this triangle to the submesh
        return true;
    }

    bool MeshBuilderSubMesh::CheckIfHasVertex(const MeshBuilderVertexLookup& vertex)
    {
        if (m_mesh->CalcNumVertexDuplicates(this, vertex.mOrgVtx) <= vertex.mDuplicateNr)
        {
            return false;
        }

        return (m_mesh->FindRealVertexNr(this, vertex.mOrgVtx, vertex.mDuplicateNr) != InvalidIndex);
    }

    size_t MeshBuilderSubMesh::GetIndex(size_t index) const
    {
        return m_mesh->FindRealVertexNr(this, m_indices[index].mOrgVtx, m_indices[index].mDuplicateNr);
    }

    size_t MeshBuilderSubMesh::CalcNumSimilarJoints(const AZStd::vector<size_t>& jointList) const
    {
        // reset our similar bones counter and get the number of bones from the input and submesh bone lists
        size_t numMatches = 0;

        // iterate through all bones from the input bone list and find the matching bones
        for (size_t inputJointIndex : jointList)
        {
            for (size_t jointIndex : jointList)
            {
                if (jointIndex == inputJointIndex)
                {
                    numMatches++;
                    break;
                }
            }
        }

        return numMatches;
    }
} // namespace AZ::MeshBuilder
