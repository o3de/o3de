/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "TriMesh.h"

// Editor
#include "Util/fastlib.h"
#include "Objects/SubObjSelection.h"


//////////////////////////////////////////////////////////////////////////
CTriMesh::CTriMesh()
{
    pFaces = nullptr;
    pVertices = nullptr;
    pWSVertices = nullptr;
    pUV = nullptr;
    pColors = nullptr;
    pEdges = nullptr;
    pWeights = nullptr;

    nFacesCount = 0;
    nVertCount = 0;
    nUVCount = 0;
    nEdgeCount = 0;

    selectionType = SO_ELEM_NONE;

    memset(m_streamSize, 0, sizeof(m_streamSize));
    memset(m_streamSel, 0, sizeof(m_streamSel));
    streamSelMask = 0;

    m_streamSel[VERTICES] = &vertSel;
    m_streamSel[EDGES] = &edgeSel;
    m_streamSel[FACES] = &faceSel;
}

//////////////////////////////////////////////////////////////////////////
CTriMesh::~CTriMesh()
{
    free(pFaces);
    free(pEdges);
    free(pVertices);
    free(pUV);
    free(pColors);
    free(pWSVertices);
    free(pWeights);
}

// Set stream size.
void CTriMesh::ReallocStream(int stream, int nNewCount)
{
    assert(stream >= 0 && stream < LAST_STREAM);
    if (stream < 0 || stream >= LAST_STREAM)
    {
        return;
    }
    if (m_streamSize[stream] == nNewCount)
    {
        return; // Stream already have required size.
    }
    void* pStream = nullptr;
    int nElementSize = 0;
    GetStreamInfo(stream, pStream, nElementSize);
    pStream = ReAllocElements(pStream, nNewCount, nElementSize);
    m_streamSize[stream] = nNewCount;

    switch (stream)
    {
    case VERTICES:
        pVertices = (CTriVertex*)pStream;
        nVertCount = nNewCount;
        vertSel.resize(nNewCount);
        break;
    case FACES:
        pFaces = (CTriFace*)pStream;
        nFacesCount = nNewCount;
        faceSel.resize(nNewCount);
        break;
    case EDGES:
        pEdges = (CTriEdge*)pStream;
        nEdgeCount = nNewCount;
        edgeSel.resize(nNewCount);
        break;
    case TEXCOORDS:
        pUV = (SMeshTexCoord*)pStream;
        nUVCount = nNewCount;
        break;
    case COLORS:
        pColors = (SMeshColor*)pStream;
        break;
    case WEIGHTS:
        pWeights = (float*)pStream;
        break;
    case LINES:
        pLines = (CTriLine*)pStream;
        break;
    case WS_POSITIONS:
        pWSVertices = (Vec3*)pStream;
        break;
    default:
        assert(0); // unknown stream.
    }
    m_streamSize[stream] = nNewCount;
}

// Set stream size.
void CTriMesh::GetStreamInfo(int stream, void*& pStream, int& nElementSize) const
{
    assert(stream >= 0 && stream < LAST_STREAM);
    switch (stream)
    {
    case VERTICES:
        pStream = pVertices;
        nElementSize = sizeof(CTriVertex);
        break;
    case FACES:
        pStream = pFaces;
        nElementSize = sizeof(CTriFace);
        break;
    case EDGES:
        pStream = pEdges;
        nElementSize = sizeof(CTriEdge);
        break;
    case TEXCOORDS:
        pStream = pUV;
        nElementSize = sizeof(SMeshTexCoord);
        break;
    case COLORS:
        pStream = pColors;
        nElementSize = sizeof(SMeshColor);
        break;
    case WEIGHTS:
        pStream = pWeights;
        nElementSize = sizeof(float);
        break;
    case LINES:
        pStream = pLines;
        nElementSize = sizeof(CTriLine);
        break;
    case WS_POSITIONS:
        pStream = pWSVertices;
        nElementSize = sizeof(Vec3);
        break;
    default:
        assert(0); // unknown stream.
    }
}

//////////////////////////////////////////////////////////////////////////
void* CTriMesh::ReAllocElements(void* old_ptr, int new_elem_num, int size_of_element)
{
    return realloc(old_ptr, new_elem_num * size_of_element);
}

/////////////////////////////////////////////////////////////////////////////////////
inline int FindVertexInHash(const Vec3& vPosToFind, const CTriVertex* pVectors, std::vector<int>& hash, float fEpsilon)
{
    for (uint32 i = 0; i < hash.size(); i++)
    {
        const Vec3& v0 = pVectors[hash[i]].pos;
        const Vec3& v1 = vPosToFind;
        if (fabsf(v0.y - v1.y) < fEpsilon &&  fabsf(v0.x - v1.x) < fEpsilon &&  fabsf(v0.z - v1.z) < fEpsilon)
        {
            return hash[i];
        }
    }
    return -1;
}

/////////////////////////////////////////////////////////////////////////////////////
inline int FindTexCoordInHash(const SMeshTexCoord& coordToFind, const SMeshTexCoord* pCoords, std::vector<int>& hash, float fEpsilon)
{
    for (uint32 i = 0; i < hash.size(); i++)
    {
        const SMeshTexCoord& t0 = pCoords[hash[i]];
        const SMeshTexCoord& t1 = coordToFind;

        if (t0.IsEquivalent(t1, fEpsilon))
        {
            return hash[i];
        }
    }
    return -1;
}


//////////////////////////////////////////////////////////////////////////
void CTriMesh::SharePositions()
{
    float fEpsilon = 0.0001f;
    float fHashScale = 256.0f / MAX(bbox.GetSize().GetLength(), fEpsilon);
    std::vector<int> arrHashTable[256];

    CTriVertex* pNewVerts = new CTriVertex[GetVertexCount()];
    SMeshColor* pNewColors = nullptr;
    if (pColors)
    {
        pNewColors = new SMeshColor[GetVertexCount()];
    }

    int nLastIndex = 0;
    for (int f = 0; f < GetFacesCount(); f++)
    {
        CTriFace& face = pFaces[f];
        for (int i = 0; i < 3; i++)
        {
            const Vec3& v = pVertices[face.v[i]].pos;
            uint8 nHash = static_cast<uint8>(RoundFloatToInt((v.x + v.y + v.z) * fHashScale));

            int find = FindVertexInHash(v, pNewVerts, arrHashTable[nHash], fEpsilon);
            if (find < 0)
            {
                pNewVerts[nLastIndex] = pVertices[face.v[i]];
                if (pColors)
                {
                    pNewColors[nLastIndex] = pColors[face.v[i]];
                }
                face.v[i] = nLastIndex;
                // Reserve some space already.
                arrHashTable[nHash].reserve(100);
                arrHashTable[nHash].push_back(nLastIndex);
                nLastIndex++;
            }
            else
            {
                face.v[i] = find;
            }
        }
    }

    SetVertexCount(nLastIndex);
    memcpy(pVertices, pNewVerts, nLastIndex * sizeof(CTriVertex));
    delete []pNewVerts;

    if (pColors)
    {
        SetColorsCount(nLastIndex);
        memcpy(pColors, pNewColors, nLastIndex * sizeof(SMeshColor));
        delete []pNewColors;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTriMesh::ShareUV()
{
    float fEpsilon = 0.0001f;
    float fHashScale = 256.0f;
    std::vector<int> arrHashTable[256];

    SMeshTexCoord* pNewUV = new SMeshTexCoord[GetUVCount()];

    int nLastIndex = 0;
    for (int f = 0; f < GetFacesCount(); f++)
    {
        CTriFace& face = pFaces[f];
        for (int i = 0; i < 3; i++)
        {
            const Vec2 uv = pUV[face.uv[i]].GetUV();
            uint8 nHash = static_cast<uint8>(RoundFloatToInt((uv.x + uv.y) * fHashScale));

            int find = FindTexCoordInHash(pUV[face.uv[i]], pNewUV, arrHashTable[nHash], fEpsilon);
            if (find < 0)
            {
                pNewUV[nLastIndex] = pUV[face.uv[i]];
                face.uv[i] = nLastIndex;
                arrHashTable[nHash].reserve(100);
                arrHashTable[nHash].push_back(nLastIndex);
                nLastIndex++;
            }
            else
            {
                face.uv[i] = find;
            }
        }
    }

    SetUVCount(nLastIndex);
    memcpy(pUV, pNewUV, nLastIndex * sizeof(SMeshTexCoord));
    delete []pNewUV;
}

//////////////////////////////////////////////////////////////////////////
void CTriMesh::CalcFaceNormals()
{
    for (int i = 0; i < nFacesCount; i++)
    {
        CTriFace& face = pFaces[i];
        Vec3 p1 = pVertices[face.v[0]].pos;
        Vec3 p2 = pVertices[face.v[1]].pos;
        Vec3 p3 = pVertices[face.v[2]].pos;
        face.normal = (p2 - p1).Cross(p3 - p1);
        face.normal.Normalize();
    }
}

#define TEX_EPS 0.001f
#define VER_EPS 0.001f

//////////////////////////////////////////////////////////////////////////
void CTriMesh::CopyStream(CTriMesh& fromMesh, int stream)
{
    void* pTrgStream = nullptr;
    void* pSrcStream = nullptr;
    int nElemSize = 0;
    fromMesh.GetStreamInfo(stream, pSrcStream, nElemSize);
    if (pSrcStream)
    {
        ReallocStream(stream, fromMesh.GetStreamSize(stream));
        GetStreamInfo(stream, pTrgStream, nElemSize);
        memcpy(pTrgStream, pSrcStream, nElemSize * fromMesh.GetStreamSize(stream));
    }
}

//////////////////////////////////////////////////////////////////////////
void CTriMesh::Copy(CTriMesh& fromMesh, int nCopyFlags)
{
    streamSelMask = fromMesh.streamSelMask;

    if (nCopyFlags & COPY_VERTICES)
    {
        CopyStream(fromMesh, VERTICES);
    }
    if (nCopyFlags & COPY_FACES)
    {
        CopyStream(fromMesh, FACES);
    }
    if (nCopyFlags & COPY_EDGES)
    {
        CopyStream(fromMesh, EDGES);
    }
    if (nCopyFlags & COPY_TEXCOORDS)
    {
        CopyStream(fromMesh, TEXCOORDS);
    }
    if (nCopyFlags & COPY_COLORS)
    {
        CopyStream(fromMesh, COLORS);
    }
    if (nCopyFlags & COPY_WEIGHTS)
    {
        CopyStream(fromMesh, WEIGHTS);
    }
    if (nCopyFlags & COPY_LINES)
    {
        CopyStream(fromMesh, LINES);
    }

    if (nCopyFlags & COPY_VERT_SEL)
    {
        vertSel = fromMesh.vertSel;
    }
    if (nCopyFlags & COPY_EDGE_SEL)
    {
        edgeSel = fromMesh.edgeSel;
    }
    if (nCopyFlags & COPY_FACE_SEL)
    {
        faceSel = fromMesh.faceSel;
    }
}

//////////////////////////////////////////////////////////////////////////
void CTriMesh::UpdateEdges()
{
    SetEdgeCount(GetFacesCount() * 3);

    std::map<CTriEdge, int> edgemap;

    int nEdges = 0;
    for (int i = 0; i < GetFacesCount(); i++)
    {
        CTriFace& face = pFaces[i];
        for (int j = 0; j < 3; j++)
        {
            int v0 = j;
            int v1 = (j != 2) ? j + 1 : 0;
            CTriEdge edge;
            edge.flags = 0;

            // First vertex index must always be smaller.
            if (face.v[v0] < face.v[v1])
            {
                edge.v[0] = face.v[v0];
                edge.v[1] = face.v[v1];
            }
            else
            {
                edge.v[0] = face.v[v1];
                edge.v[1] = face.v[v0];
            }
            edge.face[0] = i;
            edge.face[1] = -1;
            int nedge = stl::find_in_map(edgemap, edge, -1);
            if (nedge >= 0)
            {
                // Assign this face as a second member of the edge.
                if (pEdges[nedge].face[1] < 0)
                {
                    pEdges[nedge].face[1] = i;
                }

                face.edge[j] = nedge;
            }
            else
            {
                edgemap[edge] = nEdges;
                pEdges[nEdges] = edge;
                face.edge[j] = nEdges;
                nEdges++;
            }
        }
    }

    SetEdgeCount(nEdges);
}

//////////////////////////////////////////////////////////////////////////
void CTriMesh::SoftSelection(const SSubObjSelOptions& options)
{
    int i;
    int nVerts = GetVertexCount();
    CTriVertex* pVerts = pVertices;

    for (i = 0; i < nVerts; i++)
    {
        if (pWeights[i] == 1.0f)
        {
            const Vec3& vp = pVerts[i].pos;
            for (int j = 0; j < nVerts; j++)
            {
                if (pWeights[j] != 1.0f)
                {
                    if (vp.IsEquivalent(pVerts[j].pos, options.fSoftSelFalloff))
                    {
                        float fDist = vp.GetDistance(pVerts[j].pos);
                        if (fDist < options.fSoftSelFalloff)
                        {
                            float fWeight = 1.0f - (fDist / options.fSoftSelFalloff);
                            if (fWeight > pWeights[j])
                            {
                                pWeights[j] = fWeight;
                            }
                        }
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CTriMesh::UpdateSelection()
{
    bool bAnySelected = false;
    if (selectionType == SO_ELEM_VERTEX)
    {
        for (int i = 0; i < GetVertexCount(); i++)
        {
            if (vertSel[i])
            {
                bAnySelected = true;
                pWeights[i] = 1.0f;
            }
            else
            {
                pWeights[i] = 0;
            }
        }
    }
    if (selectionType == SO_ELEM_EDGE)
    {
        // Clear weights.
        for (int i = 0; i < GetVertexCount(); i++)
        {
            pWeights[i] = 0;
        }

        for (int i = 0; i < GetEdgeCount(); i++)
        {
            if (edgeSel[i])
            {
                bAnySelected = true;
                CTriEdge& edge = pEdges[i];
                for (int j = 0; j < 2; j++)
                {
                    pWeights[edge.v[j]] = 1.0f;
                }
            }
        }
    }
    else if (selectionType == SO_ELEM_FACE)
    {
        // Clear weights.
        for (int i = 0; i < GetVertexCount(); i++)
        {
            pWeights[i] = 0;
        }

        for (int i = 0; i < GetFacesCount(); i++)
        {
            if (faceSel[i])
            {
                bAnySelected = true;
                CTriFace& face = pFaces[i];
                for (int j = 0; j < 3; j++)
                {
                    pWeights[face.v[j]] = 1.0f;
                }
            }
        }
    }
    return bAnySelected;
}


//////////////////////////////////////////////////////////////////////////
bool CTriMesh::ClearSelection()
{
    bool bWasSelected = false;
    // Remove all selections.
    int i;
    for (i = 0; i < GetVertexCount(); i++)
    {
        pWeights[i] = 0;
    }
    streamSelMask = 0;
    for (int ii = 0; ii < LAST_STREAM; ii++)
    {
        if (m_streamSel[ii] && !m_streamSel[ii]->is_zero())
        {
            bWasSelected = true;
            m_streamSel[ii]->clear();
        }
    }
    return bWasSelected;
}

//////////////////////////////////////////////////////////////////////////
void CTriMesh::GetEdgesByVertex(MeshElementsArray& inVertices, MeshElementsArray& outEdges)
{
    // Brute force algorithm using binary search.
    // for every edge check if edge vertex is inside inVertices array.
    std::sort(inVertices.begin(), inVertices.end());
    for (int i = 0; i < GetEdgeCount(); i++)
    {
        if (stl::binary_find(inVertices.begin(), inVertices.end(), static_cast<int>(pEdges[i].v[0])) != inVertices.end())
        {
            outEdges.push_back(i);
        }
        else if (stl::binary_find(inVertices.begin(), inVertices.end(), static_cast<int>(pEdges[i].v[1])) != inVertices.end())
        {
            outEdges.push_back(i);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CTriMesh::GetFacesByVertex(MeshElementsArray& inVertices, MeshElementsArray& outFaces)
{
    // Brute force algorithm using binary search.
    // for every face check if face vertex is inside inVertices array.
    std::sort(inVertices.begin(), inVertices.end());
    for (int i = 0; i < GetFacesCount(); i++)
    {
        if (stl::binary_find(inVertices.begin(), inVertices.end(), static_cast<int>(pFaces[i].v[0])) != inVertices.end())
        {
            outFaces.push_back(i);
        }
        else if (stl::binary_find(inVertices.begin(), inVertices.end(), static_cast<int>(pFaces[i].v[1])) != inVertices.end())
        {
            outFaces.push_back(i);
        }
        else if (stl::binary_find(inVertices.begin(), inVertices.end(), static_cast<int>(pFaces[i].v[2])) != inVertices.end())
        {
            outFaces.push_back(i);
        }
    }
}
