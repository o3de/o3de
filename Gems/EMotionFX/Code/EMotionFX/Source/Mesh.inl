/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

MCORE_INLINE uint32 Mesh::GetNumVertices() const
{
    return mNumVertices;
}


MCORE_INLINE uint32 Mesh::GetNumIndices() const
{
    return mNumIndices;
}


MCORE_INLINE uint32 Mesh::GetNumPolygons() const
{
    return mNumPolygons;
}


MCORE_INLINE uint32 Mesh::GetNumSubMeshes() const
{
    return mSubMeshes.GetLength();
}


MCORE_INLINE SubMesh* Mesh::GetSubMesh(uint32 nr) const
{
    MCORE_ASSERT(nr < mSubMeshes.GetLength());
    return mSubMeshes[nr];
}


MCORE_INLINE void Mesh::AddSubMesh(SubMesh* subMesh)
{
    mSubMeshes.Add(subMesh);
}


MCORE_INLINE uint32* Mesh::GetIndices() const
{
    return mIndices;
}


MCORE_INLINE uint8* Mesh::GetPolygonVertexCounts() const
{
    return mPolyVertexCounts;
}

/*
MCORE_INLINE void Mesh::SetFace(const uint32 faceNr, const uint32 a, const uint32 b, const uint32 c)
{
    MCORE_ASSERT(faceNr < mNumIndices * 3);

    uint32 startIndex = faceNr * 3;
    mIndices[startIndex++]  = a;
    mIndices[startIndex++]  = b;
    mIndices[startIndex]    = c;
}
*/

MCORE_INLINE uint32 Mesh::GetNumOrgVertices() const
{
    return mNumOrgVerts;
}
