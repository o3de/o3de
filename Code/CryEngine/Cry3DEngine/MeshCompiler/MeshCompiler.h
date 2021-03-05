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

#ifndef CRYINCLUDE_CRY3DENGINE_MESHCOMPILER_MESHCOMPILER_H
#define CRYINCLUDE_CRY3DENGINE_MESHCOMPILER_MESHCOMPILER_H
#pragma once

#include "IIndexedMesh.h"

namespace mesh_compiler
{
    enum EMeshCompileFlags
    {
        MESH_COMPILE_OPTIMIZE = BIT(0),
        MESH_COMPILE_TANGENTS = BIT(1),
        MESH_COMPILE_USECUSTOMNORMALS = BIT(3),
        MESH_COMPILE_VALIDATE = BIT(4),

        // Optimizes a mesh using PowerVR SDK's optimizer.
        // This should only be set with "OptimizedPrimitiveType=1" when compiling mobile assets outside the editor.
        MESH_COMPILE_PVR_STRIPIFY = BIT(5),

        MESH_COMPILE_VALIDATE_FAIL_ON_DEGENERATE_FACES = BIT(6),
    };

    //////////////////////////////////////////////////////////////////////////
    class CMeshCompiler
    {
    public:
        CMeshCompiler();
        ~CMeshCompiler();

        // for flags see EMeshCompilerFlags
        bool Compile(CMesh& mesh, int flags);

        void SetVertexRemapping(std::vector<int>* pVertexMap)
        {
            m_pVertexMap = pVertexMap;
        }

        void SetIndexRemapping(std::vector<int>* pIndexMap)
        {
            m_pIndexMap = pIndexMap;
        }

        inline bool IsEquivalentVec3dCheckYFirst(const Vec3& v0, const Vec3& v1, float fEpsilon)
        {
            if (fabsf(v0.y - v1.y) < fEpsilon)
            {
                if (fabsf(v0.x - v1.x) < fEpsilon)
                {
                    if (fabsf(v0.z - v1.z) < fEpsilon)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        _inline const Vec3& ToVec3(const Vec3& vec)    { return vec; }
        _inline const Vec3  ToVec3(const Vec3f16& vec) { return vec.ToVec3(); }

        template<class T>
        inline int FindInPosBuffer_VF_P3X(const Vec3& vPosToFind, const T* pVertBuff, std::vector<int>* pHash, float fEpsilon)
        {
            for (uint32 i = 0; i < pHash->size(); i++)
            {
                if (IsEquivalentVec3dCheckYFirst(ToVec3(pVertBuff[(*pHash)[i]].xyz), vPosToFind, fEpsilon))
                {
                    return (*pHash)[i];
                }
            }

            return -1;
        }

        template<class V, class I>
        void WeldPos_VF_P3X(
            PodArray<V>& vertices,
            PodArray<SPipTangents>& tangents,
            PodArray<SPipNormal>& normals,
            PodArray<I>& indices,
            float fEpsilon,
            const AABB& boxBoundary)
        {
            const int numVertices = vertices.Count();
            V* pTmpVerts    = new V[numVertices];
            SPipTangents* pTmpTangents = new SPipTangents[tangents.Count()];

#if ENABLE_NORMALSTREAM_SUPPORT
            SPipNormal* pTmpNormals = NULL;
            if (normals.Count() > 0)
            {
                pTmpNormals = new SPipNormal[normals.Count()];
            }

            SPipNormal emptyNormal;
#endif

            int nCurVertex = 0;
            PodArray<I> newIndices;
            std::vector<int> arrHashTable[256];
            newIndices.reserve(indices.size());
            std::vector<int>* pHash = 0;

            float fHashElemSize = 256.0f / max(boxBoundary.max.x - boxBoundary.min.x, 0.01f);

            for (uint32 i = 0; i < indices.size(); i++)
            {
                int v = indices[i];

                assert(v < vertices.Count());

                V&            vPos  = vertices[v];
                SPipTangents& vTang = tangents[v];
#if ENABLE_NORMALSTREAM_SUPPORT
                SPipNormal&   vNorm = pTmpNormals ? normals[v] : emptyNormal;
#endif

                bool bInRange(
                    vPos.xyz.x > boxBoundary.min.x && vPos.xyz.y > boxBoundary.min.y && vPos.xyz.z > boxBoundary.min.z &&
                    vPos.xyz.x < boxBoundary.max.x && vPos.xyz.y < boxBoundary.max.y && vPos.xyz.z < boxBoundary.max.z);

                int nHashValue = int((vPos.xyz.x - boxBoundary.min.x) * fHashElemSize);

                pHash = &arrHashTable[(unsigned char)(nHashValue)];
                int nFind = FindInPosBuffer_VF_P3X(ToVec3(vPos.xyz), pTmpVerts, pHash, bInRange ? fEpsilon : 0.01f);
                if (nFind < 0)
                {
                    pHash->push_back(nCurVertex);

                    // make sure neighbor hashes also have this vertex
                    if (bInRange && fEpsilon > 0.01f)
                    {
                        pHash = &arrHashTable[(unsigned char)(nHashValue + 1)];
                        if (FindInPosBuffer_VF_P3X(ToVec3(vPos.xyz), pTmpVerts, pHash, fEpsilon) < 0)
                        {
                            pHash->push_back(nCurVertex);
                        }

                        pHash = &arrHashTable[(unsigned char)(nHashValue - 1)];
                        if (FindInPosBuffer_VF_P3X(ToVec3(vPos.xyz), pTmpVerts, pHash, fEpsilon) < 0)
                        {
                            pHash->push_back(nCurVertex);
                        }
                    }

                    PREFAST_ASSUME(nCurVertex < numVertices);

                    // add new vertex
                    pTmpVerts[nCurVertex] = vPos;
                    pTmpTangents[nCurVertex] = vTang;
#if ENABLE_NORMALSTREAM_SUPPORT
                    if (pTmpNormals)
                    {
                        pTmpNormals[nCurVertex] = vNorm;
                    }
#endif
                    newIndices.push_back(nCurVertex);
                    nCurVertex++;
                }
                else
                {
                    newIndices.push_back(nFind);
                }
            }

            indices.Clear();
            indices.AddList(newIndices);

            vertices.Clear();
            vertices.AddList(pTmpVerts, nCurVertex);

            tangents.Clear();
            tangents.AddList(pTmpTangents, nCurVertex);

#if ENABLE_NORMALSTREAM_SUPPORT
            if (pTmpNormals)
            {
                normals.Clear();
                normals.AddList(pTmpNormals, nCurVertex);

                delete[] pTmpNormals;
            }
#endif

            delete [] pTmpVerts;
            delete [] pTmpTangents;
        }

        static bool CompareMeshes(const CMesh& mesh1, const CMesh& mesh2);

        const char* GetLastError() const
        {
            return m_LastError;
        }

    private:
        bool CreateIndicesAndDeleteDuplicateVertices(CMesh& mesh);
        bool StripifyMesh_Forsyth(CMesh& mesh);
        bool StripifyMesh_PVRTriStripList(CMesh& mesh);

    public:
        static bool CheckForDegenerateFaces(const CMesh& mesh);

    private:
        static void FindVertexRanges(CMesh& mesh);

    private:
        struct SBasisFace
        {
            int v[3];
        };
        std::vector<const SMeshFace*> m_vhash_table[MAX_SUB_MATERIALS];
        std::vector<SBasisFace> m_thash_table[MAX_SUB_MATERIALS];

        std::vector<int>* m_pVertexMap;
        std::vector<int>* m_pIndexMap;

        string m_LastError;
    };
} // namespace mesh_compiler

#endif // CRYINCLUDE_CRY3DENGINE_MESHCOMPILER_MESHCOMPILER_H
