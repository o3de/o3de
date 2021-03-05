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

// This is the class that makes a renderable (stripified) vertex/index buffers
// and tangent bases out of a CGF mesh

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_RENDERMESHBUILDER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_RENDERMESHBUILDER_H
#pragma once


#include "CryChunkedFile.h"
#include "TangentSpaceCalculation.h"
#include "CryCompiledFile.h"

// Calculates tangent spaces
// Builds index buffer (stripifies), material group array, ext-to-int map.
class CRenderMeshBuilder
{
public:
    // constructs everything for the render mesh out of the given mesh
    void build (const CryChunkedFile::MeshDesc* pMeshDesc);
    // increases all indices of materials by the given offset
    void addMaterialOffset (unsigned nOffset);
    // cleans up the object
    void clear();

    // this error class is thrown from the constructor when the object can't be constructed
    class Error
    {
    public:
        Error(const char* szDesc)
            : m_szDesc(szDesc){}
        const char* c_str() const{return m_szDesc; }
    protected:
        const char* m_szDesc;
    };

    // returns the number of vertices in the resulting vertex buffer
    unsigned numVertices() const;

    // face/vertex index type
    typedef unsigned short ushort;

    // this is the index buffer (external indexation)
    std::vector<ushort> m_arrIndices;

    // this array represents the groups of indices in the index buffer:
    // each group has its own material id and number of elements (indices, i.e. number of faces * 3 in case of strip stripification)
    typedef CCFMaterialGroup MaterialGroup;
    typedef std::vector<MaterialGroup> MaterialGroupArray;
    MaterialGroupArray m_arrPrimGroups;

    // this is the mapping from new indices to original
    std::vector<ushort> m_arrExtTangMap;

    // this is the mapping from new indices to original UV indices
    // in the original CGF, texture and geometric mesh (faces/tex faces) are
    // different (tex face indices are not necessarily the same as face indices),
    // this is why the ExtToInt map doesn't coincide with this ExtUVMap
    std::vector<ushort> m_arrExtUVMap;

    // these are the tangent bases (external indexation)
    std::vector<TangData> m_arrExtTangents;

#pragma pack(push,2)
    // this is a group of faces, as they will be inside a group (material)
    struct Face
    {
        ushort v[3];
        Face(){}
        Face(ushort v0, ushort v1, ushort v2)
        {
            v[0] = v0;
            v[1] = v1;
            v[2] = v2;
        }
        Face (const CryFace& rFace)
        {
            v[0] = rFace.v0;
            v[1] = rFace.v1;
            v[2] = rFace.v2;
        }
        Face (DWORD src[3])
        {
            v[0] = (ushort)src[0];
            v[1] = (ushort)src[1];
            v[2] = (ushort)src[2];
        }
        bool isDegenerate() const
        {
            return v[0] == v[1] || v[1] == v[2] || v[2] == v[0];
        }
    };
#pragma pack(pop)

    // WARNING: this array must be binary-compatible with the array of unsigned shorts
    // passed to Nvidia Stripifier. external indexation
    typedef std::vector<Face> FaceArray;

    // this is the array of faces for each material
    std::vector<FaceArray> m_arrMtlFaces;

    // this is the actual array of faces, but in the final external indexation
    std::vector<CryFace> m_arrExtFaces;
protected:
    // adjusts the base - converts from Martin's algorithm's requirements to the engine requirements
    static void AdjustBase(TangData& rBase);

    // creates the mapping from the external to internal indices
    void buildExtToIntMaps();

    // create m_arrMtlFaces; the degenerated faces are not included
    void buildMtlFaces();

    // create the indices and the array m_arrMaterials out of m_arrMtlFaces
    void buildIndexBuffer();

    // remaps (transposes, permutates) the indices to improve spatial locality of the vertex buffer
    void remapIndicesForVBCache();

    // remaps external indices according to the given permutation old->new
    void remapExtIndices (unsigned* pPermutation, unsigned numNewTargets);

    // prepares the m_arrExtTangents, m_arrExtToTBBMap, m_arrExtTangMap and m_arrExtUVMap
    void prepareExtToIntMapping();

    // adds an entry to all required maps - m_arrExtTangents, m_arrExtToTBBMap, m_arrExtTangMap and m_arrExtUVMap
    bool addExtToIntMapEntry (DWORD FaceExt[3], const CryFace&FaceInt, const CryTexFace &TexFaceInt);

protected:
    // add the primitive group(s) and indices (m_arrPrimGroups and m_arrIndices)
    // from the given primitives generated by Nvidia Stripifier
    void appendNvidiaStrip (const struct PrimitiveGroup& Group, unsigned nMaterial);

    void selfValidate();
protected:

    const CryChunkedFile::MeshDesc* m_pMeshDesc;

    struct VertexUVPair
    {
        VertexUVPair(){}
        VertexUVPair(ushort _nVertex, ushort _nTexVertex, ushort _nExtTangent)
            : nVertex(_nVertex)
            , nTexVertex (_nTexVertex)
            , nExtTangent(_nExtTangent){}

        bool operator <  (const VertexUVPair& right) const
        {
            return
                nVertex < right.nVertex ? true:
                nVertex > right.nVertex ? false :
                nTexVertex < right.nTexVertex? true:
                nTexVertex > right.nTexVertex ? false :
                nExtTangent < right.nExtTangent;
        }

        ushort nVertex; // vertex in internal indexation
        ushort nTexVertex; // texture vertex (UV) in internal indexation
        ushort nExtTangent; // vertex in TBB indexation, or tangent in the array of tangents generated by TBB
    };

    // this is the map from the vertex-uv pair to the index of the
    // temporary vertex mapping in this->arrVertMap;
    typedef std::map <VertexUVPair, ushort> VertexUVPairMap;

    //////////////////////////////////////////////////////////////////////////
    // a proxy structure that gets passed to the tangent space calculation algorithm
    struct MeshProxy
    {
    public:
        MeshProxy ()
        {
            m_pMeshDesc = NULL;
        }

        // creates temporary mapping for splitting the vertices
        // with different UVs
        void init (const CryChunkedFile::MeshDesc* pMeshDesc)
        {
            m_pMeshDesc = pMeshDesc;
            Matrix34 tm;
            const float* pMat = &m_pMeshDesc->pNode->pDesc->tm[0][0];
            tm.SetFromVectors(
                Vec3(pMat[ 0], pMat[ 1], pMat[ 2]),
                Vec3(pMat[ 4], pMat[ 5], pMat[ 6]),
                Vec3(pMat[ 8], pMat[ 9], pMat[10]),
                Vec3(pMat[12], pMat[13], pMat[14]));
            m_tm = tm;
        }

        DWORD GetTriangleCount(void) const
        {
            return m_pMeshDesc->numFaces();
        }

        void GetTriangleIndices(const DWORD indwTriNo, DWORD outdwPos[3], DWORD outdwNorm[3], DWORD outdwUV[3]) const
        {
            const CryFace& rFace = m_pMeshDesc->pFaces[indwTriNo];
            outdwNorm[0] = outdwPos[0] = rFace.v0;
            outdwNorm[1] = outdwPos[1] = rFace.v1;
            outdwNorm[2] = outdwPos[2] = rFace.v2;
            if (m_pMeshDesc->numTexFaces())
            {
                const CryTexFace& rTexFace = m_pMeshDesc->pTexFaces[indwTriNo];
                outdwUV[0] = rTexFace.t0;
                outdwUV[1] = rTexFace.t1;
                outdwUV[2] = rTexFace.t2;
            }
            else
            {
                outdwUV[0] = outdwUV[1] = outdwUV[2] = 0;
            }
        }

        void GetPos(const DWORD indwPos, float outfPos[3]) const;

        void GetUV (const DWORD indwPos, float outfUV[2]) const
        {
            const CryUV& uv = m_pMeshDesc->pUVs[indwPos];
            outfUV[0] = uv.u;
            outfUV[1] = uv.v;
        }

        std::vector<VertexUVPair> arrVertMap;
    protected:
        const CryChunkedFile::MeshDesc* m_pMeshDesc;
        Matrix34 m_tm;
    };

    CTangentSpaceCalculation<MeshProxy> m_TangBaseBuilder;

    // this mapping gives the ext->tang ext mapping, i.e.
    // maps from the final external indexation (that's found in
    // m_arrExtTangMap, m_arrExtUVMap, m_arrExtTangents)
    // to the tangent base indexation in m_TangBaseBuilder
    // (GetBaseCount, GetTriangleBaseIndices, GetBase)
    std::vector<ushort> m_arrExtToTBBMap;

    // this is used during construction of the external maps to quickly find
    // corresponding vertex-uv pairs and avoid collisions
    // the vertex-uv indices are in internal indexations of vertices and UVs
    VertexUVPairMap m_mapVUVP;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERPC_RENDERMESHBUILDER_H
