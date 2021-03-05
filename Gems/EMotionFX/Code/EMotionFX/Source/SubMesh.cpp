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
#include "SubMesh.h"
#include "SkinningInfoVertexAttributeLayer.h"
#include "Mesh.h"
#include <EMotionFX/Source/Allocators.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SubMesh, MeshAllocator, 0)


    // constructor
    SubMesh::SubMesh(Mesh* parentMesh, uint32 startVertex, uint32 startIndex, uint32 startPolygon, uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 materialIndex, uint32 numBones)
    {
        mParentMesh     = parentMesh;
        mNumVertices    = numVerts;
        mNumIndices     = numIndices;
        mNumPolygons    = numPolygons;
        mStartIndex     = startIndex;
        mStartVertex    = startVertex;
        mStartPolygon   = startPolygon;
        mMaterial       = materialIndex;

        mBones.SetMemoryCategory(EMFX_MEMCATEGORY_GEOMETRY_MESHES);
        SetNumBones(numBones);
    }


    // destructor
    SubMesh::~SubMesh()
    {
    }


    // create
    SubMesh* SubMesh::Create(Mesh* parentMesh, uint32 startVertex, uint32 startIndex, uint32 startPolygon, uint32 numVerts, uint32 numIndices, uint32 numPolygons, uint32 materialIndex, uint32 numBones)
    {
        return aznew SubMesh(parentMesh, startVertex, startIndex, startPolygon, numVerts, numIndices, numPolygons, materialIndex, numBones);
    }


    // clone the submesh
    SubMesh* SubMesh::Clone(Mesh* newParentMesh)
    {
        SubMesh* clone = aznew SubMesh(newParentMesh, mStartVertex, mStartIndex, mStartPolygon, mNumVertices, mNumIndices, mNumPolygons, mMaterial, mBones.GetLength());
        clone->mBones = mBones;
        return clone;
    }


    // remap bone (oldNodeNr) to bone (newNodeNr)
    void SubMesh::RemapBone(uint16 oldNodeNr, uint16 newNodeNr)
    {
        // get the number of bones stored inside the submesh
        const uint32 numBones = mBones.GetLength();

        // iterate through all bones and remap the bones
        for (uint32 i = 0; i < numBones; ++i)
        {
            // remap the bone
            if (mBones[i] == oldNodeNr)
            {
                mBones[i] = newNodeNr;
            }
        }
    }


    // reinitialize the bones
    void SubMesh::ReinitBonesArray(SkinningInfoVertexAttributeLayer* skinLayer)
    {
        // clear the bones array
        mBones.Clear(false);

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
            const uint32 numInfluences = skinLayer->GetNumInfluences(orgVertex);
            for (uint32 i = 0; i < numInfluences; ++i)
            {
                // if the bone is disabled
                SkinInfluence*  influence   = skinLayer->GetInfluence(orgVertex, i);
                const uint32    nodeNr      = influence->GetNodeNr();

                // put the node index in the bones array in case it isn't in already
                if (mBones.Contains(nodeNr) == false)
                {
                    mBones.Add(nodeNr);
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


    uint32 SubMesh::FindBoneIndex(uint32 nodeNr) const
    {
        const uint32 numBones = mBones.GetLength();
        for (uint32 i = 0; i < numBones; ++i)
        {
            if (mBones[i] == nodeNr)
            {
                return i;
            }
        }

        return MCORE_INVALIDINDEX32;
    }


    // remove the given bone
    void SubMesh::RemoveBone(uint16 index)
    {
        mBones.Remove(index);
    }


    void SubMesh::SetNumBones(uint32 numBones)
    {
        if (numBones == 0)
        {
            mBones.Clear();
        }
        else
        {
            mBones.Resize(numBones);
        }
    }


    void SubMesh::SetBone(uint32 index, uint32 nodeIndex)
    {
        mBones[index] = nodeIndex;
    }
} // namespace EMotionFX
