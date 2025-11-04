/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "SubMesh.h"
#include "SkinningInfoVertexAttributeLayer.h"
#include "Mesh.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SubMesh, MeshAllocator)


    // constructor
    SubMesh::SubMesh(Mesh* parentMesh, uint32 startVertex, uint32 startIndex, uint32 startPolygon, uint32 numVerts, uint32 numIndices, uint32 numPolygons, size_t numBones)
    {
        m_parentMesh     = parentMesh;
        m_numVertices    = numVerts;
        m_numIndices     = numIndices;
        m_numPolygons    = numPolygons;
        m_startIndex     = startIndex;
        m_startVertex    = startVertex;
        m_startPolygon   = startPolygon;

        SetNumBones(numBones);
    }


    // destructor
    SubMesh::~SubMesh()
    {
    }


    // create
    SubMesh* SubMesh::Create(Mesh* parentMesh, uint32 startVertex, uint32 startIndex, uint32 startPolygon, uint32 numVerts, uint32 numIndices, uint32 numPolygons, size_t numBones)
    {
        return aznew SubMesh(parentMesh, startVertex, startIndex, startPolygon, numVerts, numIndices, numPolygons, numBones);
    }


    // clone the submesh
    SubMesh* SubMesh::Clone(Mesh* newParentMesh)
    {
        SubMesh* clone = aznew SubMesh(newParentMesh, m_startVertex, m_startIndex, m_startPolygon, m_numVertices, m_numIndices, m_numPolygons, m_bones.size());
        clone->m_bones = m_bones;
        return clone;
    }


    // remap bone (oldNodeNr) to bone (newNodeNr)
    void SubMesh::RemapBone(size_t oldNodeNr, size_t newNodeNr)
    {
        AZStd::replace(m_bones.begin(), m_bones.end(), oldNodeNr, newNodeNr);
    }


    // reinitialize the bones
    void SubMesh::ReinitBonesArray(SkinningInfoVertexAttributeLayer* skinLayer)
    {
        // clear the bones array
        m_bones.clear();

        // get shortcuts to the original vertex numbers
        const uint32* orgVertices = (uint32*)m_parentMesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

        // for all vertices in the submesh
        const uint32 startVertex = GetStartVertex();
        const uint32 numVertices = GetNumVertices();
        for (uint32 v = 0; v < numVertices; ++v)
        {
            const uint32 vertexIndex = startVertex + v;
            const uint32 orgVertex  = orgVertices[vertexIndex];

            // for all skinning influences of the vertex
            const size_t numInfluences = skinLayer->GetNumInfluences(orgVertex);
            for (size_t i = 0; i < numInfluences; ++i)
            {
                // if the bone is disabled
                SkinInfluence*  influence   = skinLayer->GetInfluence(orgVertex, i);
                const uint16    nodeNr      = influence->GetNodeNr();

                // put the node index in the bones array in case it isn't in already
                if (AZStd::find(begin(m_bones), end(m_bones), nodeNr) == end(m_bones))
                {
                    m_bones.emplace_back(nodeNr);
                }
            }
        }
    }


    // calculate how many triangles it would take to draw this submesh
    uint32 SubMesh::CalcNumTriangles() const
    {
        uint32 numTriangles = 0;

        const uint8* polyVertexCounts = GetPolygonVertexCounts();
        const uint32 numPolygons = GetNumPolygons();
        for (uint32 i = 0; i < numPolygons; ++i)
        {
            numTriangles += (polyVertexCounts[i] - 2); // 3 verts=1 triangle, 4 verts=2 triangles, 5 verts=3 triangles, etc
        }
        return numTriangles;
    }



    uint32 SubMesh::GetStartIndex() const
    {
        return m_startIndex;
    }


    uint32 SubMesh::GetStartVertex() const
    {
        return m_startVertex;
    }


    uint32 SubMesh::GetStartPolygon() const
    {
        return m_startPolygon;
    }


    uint32* SubMesh::GetIndices() const
    {
        return (uint32*)(((uint8*)m_parentMesh->GetIndices()) + m_startIndex * sizeof(uint32));
    }


    uint8* SubMesh::GetPolygonVertexCounts() const
    {
        uint8* polyVertCounts = m_parentMesh->GetPolygonVertexCounts();
        return &polyVertCounts[m_startPolygon];
    }


    uint32 SubMesh::GetNumVertices() const
    {
        return m_numVertices;
    }


    uint32 SubMesh::GetNumIndices() const
    {
        return m_numIndices;
    }


    uint32 SubMesh::GetNumPolygons() const
    {
        return m_numPolygons;
    }


    Mesh* SubMesh::GetParentMesh() const
    {
        return m_parentMesh;
    }


    void SubMesh::SetParentMesh(Mesh* mesh)
    {
        m_parentMesh = mesh;
    }

    void SubMesh::SetStartIndex(uint32 indexOffset)
    {
        m_startIndex = indexOffset;
    }


    void SubMesh::SetStartPolygon(uint32 polygonNumber)
    {
        m_startPolygon = polygonNumber;
    }


    void SubMesh::SetStartVertex(uint32 vertexOffset)
    {
        m_startVertex = vertexOffset;
    }


    void SubMesh::SetNumIndices(uint32 numIndices)
    {
        m_numIndices = numIndices;
    }


    void SubMesh::SetNumVertices(uint32 numVertices)
    {
        m_numVertices = numVertices;
    }


    size_t SubMesh::FindBoneIndex(size_t nodeNr) const
    {
        const auto foundBone = AZStd::find(m_bones.begin(), m_bones.end(), nodeNr);
        return foundBone != m_bones.end() ? AZStd::distance(m_bones.begin(), foundBone) : InvalidIndex;
    }


    // remove the given bone
    void SubMesh::RemoveBone(size_t index)
    {
        m_bones.erase(AZStd::next(begin(m_bones), index));
    }


    void SubMesh::SetNumBones(size_t numBones)
    {
        if (numBones == 0)
        {
            m_bones.clear();
        }
        else
        {
            m_bones.resize(numBones);
        }
    }


    void SubMesh::SetBone(size_t index, size_t nodeIndex)
    {
        m_bones[index] = nodeIndex;
    }
} // namespace EMotionFX
