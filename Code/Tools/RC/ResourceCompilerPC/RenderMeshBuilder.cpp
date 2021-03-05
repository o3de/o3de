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

#include "ResourceCompilerPC_precompiled.h"
#include "RenderMeshBuilder.h"
#include "StlUtils.h"
#include "NvTriStrip/NvTriStrip.h"

// constructs everything for the render mesh out of the given mesh
void CRenderMeshBuilder::build (const CryChunkedFile::MeshDesc* pMeshDesc)
{
    clear();
    m_pMeshDesc = pMeshDesc;
    // build the tangent bases
    MeshProxy Proxy;
    Proxy.init(pMeshDesc);
    m_TangBaseBuilder.CalculateTangentSpace (Proxy);

    buildExtToIntMaps();

    // create the m_arrMtlFaces: the faces are sorted and perhaps
    // degraded faces are deleted
    buildMtlFaces();

    // create the indices and the array m_arrMaterials
    buildIndexBuffer();

    // optimize the final vertex buffer spacial locality
    remapIndicesForVBCache();

    //selfValidate();
}

// increases all indices of materials by the given offset
void CRenderMeshBuilder::addMaterialOffset (unsigned nOffset)
{
    for (MaterialGroupArray::iterator it = m_arrPrimGroups.begin(); it != m_arrPrimGroups.end(); ++it)
    {
        it->nMaterial += nOffset;
    }
}

// cleans up the object
void CRenderMeshBuilder::clear()
{
    m_arrIndices.clear();
    m_arrPrimGroups.clear();
    m_arrExtTangMap.clear();
    m_arrExtUVMap.clear();
    m_arrExtTangents.clear();
    m_arrMtlFaces.clear();
    m_arrExtFaces.clear();
    m_mapVUVP.clear();
    m_arrExtToTBBMap.clear();
    m_pMeshDesc = NULL;
}


// returns the number of vertices in the resulting vertex buffer
unsigned CRenderMeshBuilder::numVertices() const
{
    // the number of external tangent bases determine this, because the
    // tangent base calculation algorithm completely splits all the necessary
    // vertices, so that the vertex buffer can be formed
    return m_arrExtTangents.size();
}

// prepares the m_arrExtTangents, m_arrExtToTBBMap, m_arrExtTangMap and m_arrExtUVMap
void CRenderMeshBuilder::prepareExtToIntMapping ()
{
    unsigned numTBBTangents = m_TangBaseBuilder.GetBaseCount();
    unsigned numTBBTangents_Reserve = numTBBTangents * 9 / 7;
    // prepare ext->TBB and the table of actual tangents
    m_arrExtToTBBMap.reserve (numTBBTangents_Reserve);
    //m_arrExtToTBBMap.resize (numTBBTangents);
    m_arrExtTangents.reserve (numTBBTangents_Reserve);
    //m_arrExtTangents.resize (numTBBTangents);
    //for (i = 0; i < numTBBTangents; ++i)
    //{
    //  m_arrExtToTBBMap[i] = i;
    //  TangData& rBase = m_arrExtTangents[i];
    //  m_TangBaseBuilder.GetBase(i, &rBase.tangent.x, &rBase.binormal.x, &rBase.tnormal.x);
    //}

    // create the indexation map new->old
    m_arrExtTangMap.reserve(numTBBTangents_Reserve);
    //m_arrExtTangMap.resize (numTBBTangents,-1);
    if (m_pMeshDesc->numTexFaces())
    {
        m_arrExtUVMap.reserve(numTBBTangents_Reserve);
        //m_arrExtUVMap.resize (numTBBTangents, -1);
    }

    m_arrExtFaces.reserve (m_pMeshDesc->numFaces());
}


// adds an entry to all required maps - m_arrExtTangents, m_arrExtToTBBMap, m_arrExtTangMap and m_arrExtUVMap
bool CRenderMeshBuilder::addExtToIntMapEntry (DWORD FaceExt[3], const CryFace& FaceInt, const CryTexFace& TexFaceInt)
{
    unsigned numTBBTangents = m_TangBaseBuilder.GetBaseCount();
    assert (m_arrExtTangMap.size() == m_arrExtUVMap.size() || m_arrExtUVMap.empty());
    assert (m_arrExtTangMap.size() == m_arrExtTangents.size());

    CryFace NewExtFace = FaceInt;

    if (FaceExt[0] == FaceExt[1] || FaceExt[1] == FaceExt[2] || FaceExt[2] == FaceExt[0])
    {
        return false;
    }

    for (unsigned i = 0; i < 3; ++i)
    {
        VertexUVPair VUVPair((ushort)FaceInt[i], (ushort)TexFaceInt[i], (ushort)FaceExt[i]);
        VertexUVPairMap::iterator itVUVPair = m_mapVUVP.find (VUVPair);
        unsigned nExtEntry;
        if (itVUVPair == m_mapVUVP.end())
        {
            // no such pair, add a new one.
            nExtEntry = m_arrExtTangents.size();
            TangData rBase;
            m_TangBaseBuilder.GetBase (FaceExt[i], &rBase.tangent.x, &rBase.binormal.x, &rBase.tnormal.x);
            AdjustBase(rBase);
            m_arrExtTangents.push_back (rBase);

            m_arrExtTangMap.push_back(FaceInt[i]);
            m_arrExtToTBBMap.push_back((ushort)FaceExt[i]);
            if (m_pMeshDesc->numTexFaces())
            {
                m_arrExtUVMap.push_back(TexFaceInt[i]);
            }
            m_mapVUVP.insert (VertexUVPairMap::value_type(VUVPair, nExtEntry));
        }
        else
        {
            // there's already such a pair, use it
            nExtEntry = itVUVPair->second;
        }

        NewExtFace[i] = nExtEntry;
    }

    assert(!NewExtFace.isDegenerate());
    m_arrExtFaces.push_back(NewExtFace);
    return true;
}



// creates the mapping from the external to internal indices
void CRenderMeshBuilder::buildExtToIntMaps()
{
    unsigned i, numTBBTangents = m_TangBaseBuilder.GetBaseCount();
    prepareExtToIntMapping();

    unsigned numDegenerate = 0;

    CryTexFace TexFaceInt (0, 0, 0);
    DWORD FaceExt[3];
    for (i = 0; i < m_pMeshDesc->numFaces(); ++i)
    {
        // internal indexation face
        const CryFace& FaceInt = m_pMeshDesc->pFaces[i];
        // external indexation face

        m_TangBaseBuilder.GetTriangleBaseIndices(i, FaceExt);

        if (m_pMeshDesc->numTexFaces())
        {
            TexFaceInt = m_pMeshDesc->pTexFaces[i];
        }

        if (!addExtToIntMapEntry (FaceExt, FaceInt, TexFaceInt))
        {
            ++numDegenerate;
            continue; // degenerate face
        }

#ifdef _DEBUG
        CryFace& NewExtFace = m_arrExtFaces.back();
        for (int j = 0; j < 3; ++j)
        {
            assert (m_arrExtUVMap[NewExtFace[j]] == TexFaceInt[j]);
            assert (m_arrExtTangMap[NewExtFace[j]] == FaceInt[j]);
        }
#endif
    }
    if (numDegenerate)
    {
        LogWarning("%u degenerate faces (skipped)", numDegenerate);
    }
}


// calculate the number of elements == nEl


// create the indices and the array m_arrMaterials

// Create the m_arrMtlFaces
// degraded faces are deleted
void CRenderMeshBuilder::buildMtlFaces()
{
    unsigned nFace, numExtFaces = m_arrExtFaces.size();
    // pass 1: calculate each material's number of faces, and the number of materials
    // SKIPPED NOW
    const unsigned nMaxMatID = 0x400;
    // pass 2: create the face groups and reserve space for the faces
    m_arrMtlFaces.reserve (nMaxMatID / 4);

    unsigned numSkippedFaces = 0;

    // pass 3: create the faces in the face groups
    for (nFace = 0; nFace < numExtFaces; ++nFace)
    {
        // external indexation face
        const CryFace& rExtFace = m_arrExtFaces[nFace];

        int nMatID = rExtFace.MatID;
        // material id of the face to count
        if (nMatID < 0 || nMatID > nMaxMatID)
        {
            ++numSkippedFaces;
            continue;
        }
        assert (!rExtFace.isDegenerate());
        if (m_arrMtlFaces.size() <= (unsigned)nMatID)
        {
            m_arrMtlFaces.resize (nMatID + 1);
        }
        m_arrMtlFaces[nMatID].push_back (Face(rExtFace));
    }

    if (numSkippedFaces)
    {
        LogWarning ("%d faces skipped: no material or material id is out of range", numSkippedFaces);
    }
}


//////////////////////////////////////////////////////////////////////////
// create the indices and the array m_arrMaterials out of m_arrMtlFaces
void CRenderMeshBuilder::buildIndexBuffer()
{
    m_arrIndices.reserve (m_pMeshDesc->numFaces() * 3);
    SetListsOnly (true);

    for (unsigned nMaterial = 0; nMaterial < m_arrMtlFaces.size(); ++nMaterial)
    {
        const FaceArray& arrFaces = m_arrMtlFaces[nMaterial];
        if (arrFaces.empty())
        {
            continue;
        }

        PrimitiveGroup* pGroup = NULL;
        unsigned short numGroups = 0;
        GenerateStrips((unsigned short*)&arrFaces[0], arrFaces.size() * 3, &pGroup, &numGroups);

        for (unsigned nGroup = 0; nGroup < numGroups; ++nGroup)
        {
            appendNvidiaStrip (pGroup[nGroup], nMaterial);
        }

        delete[numGroups] pGroup;
    }
}


//////////////////////////////////////////////////////////////////////////
// remaps (transposes, permutates) the indices to improve spatial locality of the vertex buffer
void CRenderMeshBuilder::remapIndicesForVBCache()
{
    // this is the old->new indexation
    std::vector<unsigned> arrVCache;
    arrVCache.resize (m_arrExtTangents.size(), -1);
    unsigned nNextVertex = 0;
    for (unsigned i = 0; i < m_arrIndices.size(); ++i)
    {
        ushort nVertex = m_arrIndices[i];
        if (arrVCache[nVertex] == -1)
        {
            // we've met this vertex for the first time
            arrVCache[nVertex] = nNextVertex++;
        }
    }

    remapExtIndices(&arrVCache[0], nNextVertex);
}

// permutate the contents of the array with a permutation old->new
template <class T>
void Permutate (std::vector<T>& arrOld, unsigned* pPermutation, unsigned newSize)
{
    assert (newSize <= arrOld.size());
    std::vector<T> arrNew;
    arrNew.resize (newSize);
    for (unsigned nEntry = 0; nEntry < arrOld.size(); ++nEntry)
    {
        if (pPermutation[nEntry] < arrNew.size())
        {
            arrNew[pPermutation[nEntry]] = arrOld[nEntry];
        }
        else
        {
            assert (pPermutation[nEntry] == -1);
        }
    }
    arrOld.swap(arrNew);
}


// remaps external indices according to the given permutation old->new
void CRenderMeshBuilder::remapExtIndices (unsigned* pPermutation, unsigned numNewVertices)
{
    unsigned numVertices = this->numVertices();
    // remap the indices
    for (unsigned nIndex = 0; nIndex < m_arrIndices.size(); ++nIndex)
    {
        assert (m_arrIndices[nIndex] < numVertices);
        m_arrIndices[nIndex] = pPermutation[m_arrIndices[nIndex]];
        assert (m_arrIndices[nIndex] < numNewVertices);
    }

    for (unsigned nFace = 0; nFace < m_arrExtFaces.size(); ++nFace)
    {
        m_arrExtFaces[nFace].v0 = pPermutation[m_arrExtFaces[nFace].v0];
        m_arrExtFaces[nFace].v1 = pPermutation[m_arrExtFaces[nFace].v1];
        m_arrExtFaces[nFace].v2 = pPermutation[m_arrExtFaces[nFace].v2];
    }

    // remap the ExtToInt mappings
    assert (m_arrExtTangMap.size() == numVertices);
    Permutate(m_arrExtTangMap, pPermutation, numNewVertices);

    if (m_pMeshDesc->numTexFaces())
    {
        assert (m_arrExtUVMap.size() == numVertices);
        Permutate(m_arrExtUVMap, pPermutation, numNewVertices);
    }

    assert (m_arrExtTangents.size() == numVertices);
    // remap the tangent bases
    Permutate(m_arrExtTangents, pPermutation, numNewVertices);
    assert (m_arrExtTangents.size() == numNewVertices);
}


//////////////////////////////////////////////////////////////////////////
// add the primitive group(s) and indices (m_arrPrimGroups and m_arrIndices)
// from the given primitives generated by Nvidia Stripifier
void CRenderMeshBuilder::appendNvidiaStrip (const struct PrimitiveGroup& rGroup, unsigned nMaterial)
{
    int j;

    // in case we'll add this material group, collect info in it
    MaterialGroup MatGroup;
    MatGroup.nMaterial  = nMaterial;
    MatGroup.nIndexBase = m_arrIndices.size();
    MatGroup.numIndices = 0;

    for (int nIndex = 0; nIndex < (int)rGroup.numIndices - 2; )
    {
        int v[3];
        unsigned short* src = rGroup.indices + nIndex;
        switch (rGroup.type)
        {
        case PT_LIST:
            v[0] = src[0];
            v[1] = src[1];
            v[2] = src[2];
            nIndex += 3;
            break;
        case PT_STRIP:
            if (nIndex & 1)
            {
                v[0] = src[1];
                v[1] = src[0];
                v[2] = src[2];
            }
            else
            {
                v[0] = src[0];
                v[1] = src[1];
                v[2] = src[2];
            }
            nIndex += 1;
            break;
        case PT_FAN:
            v[0] = rGroup.indices[0];
            v[1] = src[1];
            v[2] = src[2];
            break;
        }
        if (v[0] == v[1] || v[1] == v[2] || v[2] == v[0])
        {
            continue;
        }

        MatGroup.numIndices += 3;
        for (j = 0; j < 3; ++j)
        {
            m_arrIndices.push_back(v[j]);
        }
    }

    if (MatGroup.numIndices)
    {
        // there were some triangles - add a new group or append those triangles to the previous group
        if (!m_arrPrimGroups.empty() && m_arrPrimGroups.back().nMaterial == nMaterial)
        {
            m_arrPrimGroups.back().numIndices += MatGroup.numIndices;
        }
        else
        {
            m_arrPrimGroups.push_back(MatGroup);
        }
    }
}


void CRenderMeshBuilder::selfValidate()
{
    assert (m_arrExtFaces.size() <= m_pMeshDesc->numFaces());
    unsigned numFaces = m_arrExtFaces.size();
    for (unsigned nFace = 0; nFace < numFaces; ++nFace)
    {
        CryFace ExtFace = m_arrExtFaces[nFace];
        CryFace IntFace = m_pMeshDesc->pFaces[nFace];
        CryTexFace TexFace = m_pMeshDesc->pTexFaces[nFace];

        for (int i = 0; i < 3; ++i)
        {
            // this is only applicable to a normal manifold mesh
            assert (m_arrExtUVMap[ExtFace[i]] == TexFace[i]);
            assert (m_arrExtTangMap[ExtFace[i]] == IntFace[i]);
        }
    }
}

// adjusts the base - converts from Martin's algorithm's requirements to the engine requirements
void CRenderMeshBuilder::AdjustBase(TangData& rBase)
{
    /*
        float fBinormal = rBase.binormal * rBase.tnormal;
        float fTangent = rBase.tangent * rBase.tnormal;
        assert (fabs(fBinormal) < 1e-2 && fabs(fTangent) < 1e-2);
    /*  // normalize the normal
        float fEpsilon = 0.0005f;
        float fSqrt1_2 = 0.70710678118654752440084436210485f; // square root of 1/2
        float fNormalLen = rBase.tnormal.GetLength();
        if (fNormalLen < fEpsilon)
            rBase.tnormal /= fNormalLen;
        rBase.tnormal /= fNormalLen;

        // make the bisect that
        Vec3 vBisect = rBase.binormal+rBase.tangent;
        vBisect -= (vBisect * rBase.tnormal) * rBase.tnormal; // make it orthogonal to the normal
        float fBisectLen = vBisect.GetLength();
        if (fBisectLen < fEpsilon)
            return;
        vBisect /= fBisectLen;

        Vec3 vBase = vBisect ^ rBase.tnormal;

        if (rBase.binormal * vBase > rBase.tangent * vBase)
        {
            assert (rBase.binormal * vBase > 0 && rBase.tangent * vBase < 0);
            rBase.binormal = (vBisect + vBase) * fSqrt1_2;
            rBase.tangent  = (vBisect - vBase) * fSqrt1_2;
        }
        else
        {
            assert (rBase.binormal * vBase < 0 && rBase.tangent * vBase > 0);
            rBase.binormal = (vBisect - vBase) * fSqrt1_2;
            rBase.tangent  = (vBisect + vBase) * fSqrt1_2;
        }
        */
    //std::swap(rBase.binormal, rBase.tangent);
    rBase.binormal = -rBase.binormal;
}

void CRenderMeshBuilder::MeshProxy::GetPos(const DWORD indwPos, float outfPos[3]) const
{
    const Vec3 ptPos = m_pMeshDesc->pVertices[indwPos].p;
    // unrotate the object
    for (int i = 0; i < 3; ++i)
    {
        outfPos[i] = m_tm(0, i) * ptPos.x + m_tm(1, i) * ptPos.y + m_tm(2, i) * ptPos.z;
    }
}
