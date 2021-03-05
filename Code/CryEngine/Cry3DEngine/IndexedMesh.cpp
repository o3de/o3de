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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "Cry3DEngine_precompiled.h"

#include "IndexedMesh.h"
#include "MeshCompiler/MeshCompiler.h"

DEFINE_INTRUSIVE_LINKED_LIST(CIndexedMesh)

CIndexedMesh::CIndexedMesh()
{
}

CIndexedMesh::~CIndexedMesh()
{
}

void CIndexedMesh::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->AddObject(this, sizeof(*this));
    CMesh::GetMemoryUsage(pSizer);
}

void CIndexedMesh::RestoreFacesFromIndices()
{
    const int indexCount = GetIndexCount();
    SetFaceCount(indexCount / 3);
    memset(m_pFaces, 0, GetFaceCount() * sizeof(m_pFaces[0]));

    int nFaceId = 0;
    for (int i = 0; i < indexCount; i += 3)
    {
        if (m_pIndices[i] != (vtx_idx) - 1)  // deleted faces have -1 here
        {
            for (int v = 0; v < 3; ++v)
            {
                assert((int)m_pIndices[i + v] < GetVertexCount());
                m_pFaces[nFaceId].v[v] = m_pIndices[i + v];
            }
            ++nFaceId;
        }
    }

    SetFaceCount(nFaceId);
}

void CIndexedMesh::Optimize(const char* szComment)
{
    mesh_compiler::CMeshCompiler meshCompiler;

    if (szComment)
    {
        // mesh_compiler::MESH_COMPILE_OPTIMIZE is a bit expensive so we show a warning if it's used at run time
        Warning("CIndexedMesh::Optimize is called at run time by %s", szComment);
    }

    if (!meshCompiler.Compile(*this, (mesh_compiler::MESH_COMPILE_TANGENTS | mesh_compiler::MESH_COMPILE_OPTIMIZE)))
    {
        Warning("CIndexedMesh::Optimize failed: %s", meshCompiler.GetLastError());
    }
}

void CIndexedMesh::CalcBBox()
{
    const int vertexCount = GetVertexCount();

    if (vertexCount == 0 || !m_pPositions)
    {
        m_bbox = AABB(Vec3(0, 0, 0), Vec3(0, 0, 0));
        return;
    }

    assert(m_pPositionsF16 == 0);

    m_bbox.Reset();

    const int faceCount = GetFaceCount();

    if (faceCount > 0)
    {
        for (int i = 0; i < faceCount; ++i)
        {
            for (int v = 0; v < 3; ++v)
            {
                const int nIndex = m_pFaces[i].v[v];
                assert(nIndex >= 0 && nIndex < vertexCount);
                m_bbox.Add(m_pPositions[nIndex]);
            }
        }
    }
    else
    {
        const int indexCount = GetIndexCount();

        for (int i = 0; i < indexCount; ++i)
        {
            m_bbox.Add(m_pPositions[m_pIndices[i]]);
        }
    }
}


