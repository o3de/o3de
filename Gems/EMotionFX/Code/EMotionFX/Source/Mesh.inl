/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

MCORE_INLINE uint32 Mesh::GetNumVertices() const
{
    return m_numVertices;
}


MCORE_INLINE uint32 Mesh::GetNumIndices() const
{
    return m_numIndices;
}


MCORE_INLINE uint32 Mesh::GetNumPolygons() const
{
    return m_numPolygons;
}


MCORE_INLINE size_t Mesh::GetNumSubMeshes() const
{
    return m_subMeshes.size();
}


MCORE_INLINE SubMesh* Mesh::GetSubMesh(size_t nr) const
{
    MCORE_ASSERT(nr < m_subMeshes.size());
    return m_subMeshes[nr];
}


MCORE_INLINE void Mesh::AddSubMesh(SubMesh* subMesh)
{
    m_subMeshes.emplace_back(subMesh);
}


MCORE_INLINE uint32* Mesh::GetIndices() const
{
    return m_indices;
}


MCORE_INLINE uint8* Mesh::GetPolygonVertexCounts() const
{
    return m_polyVertexCounts;
}


MCORE_INLINE uint32 Mesh::GetNumOrgVertices() const
{
    return m_numOrgVerts;
}
