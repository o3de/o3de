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
    AZ_CLASS_ALLOCATOR_IMPL(SubMesh, MeshAllocator, 0)


    // constructor
    SubMesh::SubMesh(Mesh* parentMesh, uint32 startVertex, uint32 startIndex, uint32 startPolygon, uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 materialIndex, size_t numBones)
    {
        mParentMesh     = parentMesh;
        mNumVertices    = numVerts;
        mNumIndices     = numIndices;
        mNumPolygons    = numPolygons;
        mStartIndex     = startIndex;
        mStartVertex    = startVertex;
        mStartPolygon   = startPolygon;
        mMaterial       = materialIndex;

        SetNumBones(numBones);
    }


    // destructor
    SubMesh::~SubMesh()
    {
    }


    // create
    SubMesh* SubMesh::Create(Mesh* parentMesh, uint32 startVertex, uint32 startIndex, uint32 startPolygon, uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 materialIndex, size_t numBones)
    {
        return aznew SubMesh(parentMesh, startVertex, startIndex, startPolygon, numVerts, numIndices, numPolygons, materialIndex, numBones);
    }


    // clone the submesh
    SubMesh* SubMesh::Clone(Mesh* newParentMesh)
    {
        SubMesh* clone = aznew SubMesh(newParentMesh, mStartVertex, mStartIndex, mStartPolygon, mNumVertices, mNumIndices, mNumPolygons, mMaterial, mBones.size());
        clone->mBones = mBones;
        return clone;
    }


    // remap bone (oldNodeNr) to bone (newNodeNr)
    void SubMesh::RemapBone(size_t oldNodeNr, size_t newNodeNr)
    {
        AZStd::replace(mBones.begin(), mBones.end(), oldNodeNr, newNodeNr);
    }


    // reinitialize the bones
    void SubMesh::ReinitBonesArray(SkinningInfoVertexAttributeLayer* skinLayer)
    {
        // clear the bones array
        mBones.clear();

        // get shortcuts to the original vertex numbers
        const uint32* orgVertices = (uint32*)mParentMesh->FindOriginalVertexData(Mesh::ATTRIB_ORGVTXNUMBERS);

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
                if (AZStd::find(begin(mBones), end(mBones), nodeNr) == end(mBones))
                {
                    mBones.emplace_back(nodeNr);
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
        return mStartIndex;
    }


    uint32 SubMesh::GetStartVertex() const
    {
        return mStartVertex;
    }


    uint32 SubMesh::GetStartPolygon() const
    {
        return mStartPolygon;
    }


    uint32* SubMesh::GetIndices() const
    {
        return (uint32*)(((uint8*)mParentMesh->GetIndices()) + mStartIndex * sizeof(uint32));
    }


    uint8* SubMesh::GetPolygonVertexCounts() const
    {
        uint8* polyVertCounts = mParentMesh->GetPolygonVertexCounts();
        return &polyVertCounts[mStartPolygon];
    }


    uint32 SubMesh::GetNumVertices() const
    {
        return mNumVertices;
    }


    uint32 SubMesh::GetNumIndices() const
    {
        return mNumIndices;
    }


    uint32 SubMesh::GetNumPolygons() const
    {
        return mNumPolygons;
    }


    Mesh* SubMesh::GetParentMesh() const
    {
        return mParentMesh;
    }


    void SubMesh::SetParentMesh(Mesh* mesh)
    {
        mParentMesh = mesh;
    }


    void SubMesh::SetMaterial(uint32 materialIndex)
    {
        mMaterial = materialIndex;
    }


    uint32 SubMesh::GetMaterial() const
    {
        return mMaterial;
    }


    void SubMesh::SetStartIndex(uint32 indexOffset)
    {
        mStartIndex = indexOffset;
    }


    void SubMesh::SetStartPolygon(uint32 polygonNumber)
    {
        mStartPolygon = polygonNumber;
    }


    void SubMesh::SetStartVertex(uint32 vertexOffset)
    {
        mStartVertex = vertexOffset;
    }


    void SubMesh::SetNumIndices(uint32 numIndices)
    {
        mNumIndices = numIndices;
    }


    void SubMesh::SetNumVertices(uint32 numVertices)
    {
        mNumVertices = numVertices;
    }


    size_t SubMesh::FindBoneIndex(size_t nodeNr) const
    {
        const auto foundBone = AZStd::find(mBones.begin(), mBones.end(), nodeNr);
        return foundBone != mBones.end() ? AZStd::distance(mBones.begin(), foundBone) : InvalidIndex;
    }


    // remove the given bone
    void SubMesh::RemoveBone(size_t index)
    {
        mBones.erase(AZStd::next(begin(mBones), index));
    }


    void SubMesh::SetNumBones(size_t numBones)
    {
        if (numBones == 0)
        {
            mBones.clear();
        }
        else
        {
            mBones.resize(numBones);
        }
    }


    void SubMesh::SetBone(size_t index, size_t nodeIndex)
    {
        mBones[index] = nodeIndex;
    }
} // namespace EMotionFX
