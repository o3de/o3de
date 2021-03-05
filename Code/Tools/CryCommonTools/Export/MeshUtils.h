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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MESHUTILS_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MESHUTILS_H
#pragma once


#include "BaseTypes.h"     // uint8
#include "Cry_Vector3.h"   // Vec3
#include "IIndexedMesh.h"  // CMesh
namespace MeshUtils
{
    struct Face
    {
        int vertexIndex[3];
    };

    struct Color
    {
        uint8 r;
        uint8 g;
        uint8 b;
    };

    // Stores linking of a vertex to bone(s)
    class VertexLinks
    {
    public:
        struct Link
        {
            int boneId;
            float weight;
            Vec3 offset;

            Link()
                : boneId(-1)
                , weight(-1.0f)
                , offset(0.0f, 0.0f, 0.0f)
            {
            }
        };

        enum ESort
        {
            eSort_ByWeight,
            eSort_ByBoneId,
        };

    public:
        std::vector<Link> links;

    public:
        // minWeightToDelete: links with weights <= minWeightToDelete will be deleted
        const char* Normalize(ESort eSort, const float minWeightToDelete, const int maxLinkCount)
        {
            if (minWeightToDelete < 0 || minWeightToDelete >= 1)
            {
                return "Bad minWeightToDelete passed";
            }
            if (maxLinkCount <= 0)
            {
                return "Bad maxLinkCount passed";
            }

            // Merging links with matching bone ids
            {
                DeleteByWeight(0.0f);

                if (links.empty())
                {
                    return "All bone links of a vertex have zero weight";
                }

                std::sort(links.begin(), links.end(), CompareLinksByBoneId);

                size_t dst = 0;
                for (size_t i = 1; i < links.size(); ++i)
                {
                    if (links[i].boneId == links[dst].boneId)
                    {
                        const float w0 = links[dst].weight;
                        const float w1 = links[i].weight;
                        const float a =  w0 / (w0 + w1);
                        links[dst].offset = links[dst].offset * a + links[i].offset * (1 - a);
                        links[dst].weight = w0 + w1;
                    }
                    else
                    {
                        links[++dst] = links[i];
                    }
                }

                links.resize(dst + 1);
            }

            // Deleting links, normalizing link weights.
            //
            // Note: we produce meaningful results even in cases like this:
            // input weights are { 0.03, 0.01 }, minWeightTodelete is 0.2.
            // Output weights produced are { 0.75, 0.25 }.
            {
                std::sort(links.begin(), links.end(), CompareLinksByWeight);

                if (links.size() > maxLinkCount)
                {
                    links.resize(maxLinkCount);
                }

                NormalizeWeights();

                const size_t oldSize = links.size();

                DeleteByWeight(minWeightToDelete);

                if (links.empty())
                {
                    return "All bone links of a vertex are deleted (minWeightToDelete is too big)";
                }

                if (links.size() != oldSize)
                {
                    NormalizeWeights();
                }
            }

            switch (eSort)
            {
            case eSort_ByWeight:
                // Do nothing because we already sorted links by weight (see above)
                break;
            case eSort_ByBoneId:
                std::sort(links.begin(), links.end(), CompareLinksByBoneId);
                break;
            default:
                assert(0);
                break;
            }

            return 0;
        }

    private:
        void DeleteByWeight(float minWeightToDelete)
        {
            for (size_t i = 0; i < links.size(); ++i)
            {
                if (links[i].weight <= minWeightToDelete)
                {
                    if (i < links.size() - 1)
                    {
                        links[i] = links[links.size() - 1];
                    }
                    links.resize(links.size() - 1);
                    --i;
                }
            }
        }

        void NormalizeWeights()
        {
            assert(!links.empty() && links[0].weight > 0);

            float w = 0;

            for (size_t i = 0; i < links.size(); ++i)
            {
                w += links[i].weight;
            }

            w = 1 / w;

            for (size_t i = 0; i < links.size(); ++i)
            {
                links[i].weight *= w;
            }
        }

        static bool CompareLinksByBoneId(const Link& left, const Link& right)
        {
            if (left.boneId != right.boneId)
            {
                return left.boneId < right.boneId;
            }
            if (left.weight != right.weight)
            {
                return left.weight < right.weight;
            }
            return memcmp(&left.offset, &right.offset, sizeof(left.offset)) < 0;
        }

        static bool CompareLinksByWeight(const Link& left, const Link& right)
        {
            if (left.weight != right.weight)
            {
                return left.weight > right.weight;
            }
            if (left.boneId != right.boneId)
            {
                return left.boneId < right.boneId;
            }
            return memcmp(&left.offset, &right.offset, sizeof(left.offset)) < 0;
        }
    };


    class Mesh
    {
    public:
        // Vertex data
        std::vector<Vec3> m_positions;
        std::vector<int> m_topologyIds;
        std::vector<Vec3> m_normals;
        std::vector<std::vector<Vec2>> m_texCoords;
        std::vector<Color> m_colors;
        std::vector<uint8> m_alphas;
        std::vector<VertexLinks> m_links;
        std::vector<int> m_vertexMatIds;
        size_t m_auxSizeof;
        std::vector<uint8> m_aux;

        // Face data
        std::vector<Face> m_faces;
        std::vector<int> m_faceMatIds;

        // Mappings computed and filled by ComputeVertexRemapping()
        std::vector<int> m_vertexOldToNew;
        std::vector<int> m_vertexNewToOld;

    public:
        Mesh()
            : m_auxSizeof(0)
        {
        }

        int GetVertexCount() const
        {
            return m_positions.size();
        }

        int GetFaceCount() const
        {
            return m_faces.size();
        }

        //////////////////////////////////////////////////////////////////////////
        // Setters

        void Clear()
        {
            m_positions.clear();
            m_topologyIds.clear();
            m_normals.clear();
            m_texCoords.clear();
            m_colors.clear();
            m_alphas.clear();
            m_links.clear();
            m_vertexMatIds.clear();
            m_aux.clear();

            m_faces.clear();
            m_faceMatIds.clear();

            m_vertexOldToNew.clear();
            m_vertexNewToOld.clear();
        }

        const char* SetPositions(const float* pVec3, int count, int stride, const float scale)
        {
            if (count <= 0)
            {
                return "bad position count";
            }
            if (stride < 0 || (stride > 0 && stride < sizeof(Vec3)))
            {
                return "bad position stride";
            }

            m_positions.resize(count);

            for (int i = 0; i < count; ++i)
            {
                const float* const p = (const float*)(((const char*)pVec3) + ((size_t)i * stride));
                if (!_finite(p[0]) || !_finite(p[1]) || !_finite(p[2]))
                {
                    m_positions.clear();
                    return "Illegal (NAN) vertex position. Fix the 3d Model.";
                }
                m_positions[i].x = p[0] * scale;
                m_positions[i].y = p[1] * scale;
                m_positions[i].z = p[2] * scale;
            }

            return 0;
        }

        const char* SetTopologyIds(const int* pTopo, int count, int stride)
        {
            if (count <= 0)
            {
                return "bad topologyId count";
            }
            if (stride < 0 || (stride > 0 && stride < sizeof(int)))
            {
                return "bad topologyId stride";
            }

            m_topologyIds.resize(count);

            for (int i = 0; i < count; ++i)
            {
                const int* const p = (const int*)(((const char*)pTopo) + ((size_t)i * stride));
                m_topologyIds[i] = p[0];
            }

            return 0;
        }

        const char* SetNormals(const float* pVec3, int count, int stride)
        {
            if (count <= 0)
            {
                return "bad normal count";
            }
            if (stride < 0 || (stride > 0 && stride < sizeof(Vec3)))
            {
                return "bad normal stride";
            }

            m_normals.resize(count);

            for (int i = 0; i < count; ++i)
            {
                const float* const p = (const float*)(((const char*)pVec3) + ((size_t)i * stride));
                if (!_finite(p[0]) || !_finite(p[1]) || !_finite(p[2]))
                {
                    m_normals.clear();
                    return "Illegal (NAN) vertex normal. Fix the 3d Model.";
                }
                m_normals[i].x = p[0];
                m_normals[i].y = p[1];
                m_normals[i].z = p[2];
                m_normals[i] = m_normals[i].GetNormalizedSafe(Vec3_OneZ);
            }

            return 0;
        }

        const char* SetTexCoords(const float* pVec2, int count, int stride, bool bFlipT, uint streamIndex)
        {
            if (count <= 0)
            {
                return "bad texCoord count";
            }
            if (stride < 0 || (stride > 0 && stride < sizeof(float) * 2))
            {
                return "bad texCoord stride";
            }
            if (m_texCoords.size() <= streamIndex)
            {
                m_texCoords.resize(streamIndex + 1);
            }

            m_texCoords[streamIndex].resize(count);

            for (int i = 0; i < count; ++i)
            {
                const float* const p = (const float*)(((const char*)pVec2) + ((size_t)i * stride));
                if (!_finite(p[0]) || !_finite(p[1]))
                {
                    m_texCoords[streamIndex].clear();
                    return "Illegal (NAN) texture coordinate. Fix the 3d Model.";
                }
                m_texCoords[streamIndex][i].x = p[0];
                m_texCoords[streamIndex][i].y = bFlipT ? 1 - p[1] : p[1];
            }

            return 0;
        }

        const char* SetColors(const uint8* pRgb, int count, int stride)
        {
            if (count <= 0)
            {
                return "bad color count";
            }
            if (stride < 0 || (stride > 0 && stride < 3))
            {
                return "bad color stride";
            }

            m_colors.resize(count);

            for (int i = 0; i < count; ++i)
            {
                const uint8* const p = (((const uint8*)pRgb) + ((size_t)i * stride));
                m_colors[i].r = p[0];
                m_colors[i].g = p[1];
                m_colors[i].b = p[2];
            }

            return 0;
        }

        const char* SetAlphas(const uint8* pAlpha, int count, int stride)
        {
            if (count <= 0)
            {
                return "bad alpha count";
            }
            if (stride < 0)
            {
                return "bad alpha stride";
            }

            m_alphas.resize(count);

            for (int i = 0; i < count; ++i)
            {
                const uint8* const p = (((const uint8*)pAlpha) + ((size_t)i * stride));
                m_alphas[i] = p[0];
            }

            return 0;
        }

        const char* SetFaces(const int* pVertIdx3, int count, int stride)
        {
            if (count <= 0)
            {
                return "bad face count";
            }
            if (stride < 0 || (stride > 0 && stride < 3 * sizeof(int)))
            {
                return "bad face stride";
            }

            m_faces.resize(count);

            for (int i = 0; i < count; ++i)
            {
                const int* const p = (const int*)(((const char*)pVertIdx3) + ((size_t)i * stride));
                for (int j = 0; j < 3; ++j)
                {
                    if (p[j] < 0 || p[j] >= m_positions.size())
                    {
                        return "bad vertex index found in a face";
                    }
                    m_faces[i].vertexIndex[j] = p[j];
                }
            }

            return 0;
        }

        const char* SetFaceMatIds(const int* pMatIds, int count, int stride, int maxMaterialId)
        {
            if (count <= 0)
            {
                return "bad face materialId count";
            }
            if (stride < 0 || (stride > 0 && stride < sizeof(int)))
            {
                return "bad face materialIdstride";
            }

            m_faceMatIds.resize(count);

            for (int i = 0; i < count; ++i)
            {
                const int* const p = (const int*)(((const char*)pMatIds) + ((size_t)i * stride));
                if (p[0] < 0)
                {
                    return "negative material ID found in a face";
                }
                if (p[0] >= maxMaterialId)
                {
                    return "material ID found in a face is outside of allowed ranges";
                }
                m_faceMatIds[i] = p[0];
            }

            return 0;
        }

        const char* SetAux(size_t auxSizeof,  const void* pData, int count, int stride)
        {
            if (auxSizeof <= 0)
            {
                return "bad aux sizeof";
            }
            if (count <= 0)
            {
                return "bad aux count";
            }
            if (stride < 0 || (stride > 0 && stride < auxSizeof))
            {
                return "bad aux stride";
            }

            m_auxSizeof = auxSizeof;

            m_aux.resize(count * m_auxSizeof);

            for (int i = 0; i < count; ++i)
            {
                const uint8* const p = (((const uint8*)pData) + ((size_t)i * stride));
                memcpy(&m_aux[i * m_auxSizeof], p, m_auxSizeof);
            }

            return 0;
        }

        //////////////////////////////////////////////////////////////////////////
        // Validation

        // Returns 0 if ok, or pointer to the error text
        const char* Validate() const
        {
            const int nVerts = (int)m_positions.size();
            if (nVerts <= 0)
            {
                return "No vertices";
            }

            const int nFaces = (int)m_faces.size();
            if (nFaces <= 0)
            {
                return "No faces";
            }

            if (!m_topologyIds.empty() && nVerts != (int)m_topologyIds.size())
            {
                return "Mismatch in the number of topology IDs";
            }

            if (!m_normals.empty() && nVerts != (int)m_normals.size())
            {
                return "Mismatch in the number of normals";
            }

            for (uint streamIndex = 0; streamIndex < m_texCoords.size(); ++streamIndex)
            {
                if (!m_texCoords[streamIndex].empty() && nVerts != (int)m_texCoords[streamIndex].size())
                {
                    return "Mismatch in the number of texture coordinates";
                }
            }

            if (!m_colors.empty() && nVerts != (int)m_colors.size())
            {
                return "Mismatch in the number of colors";
            }

            if (!m_alphas.empty() && nVerts != (int)m_alphas.size())
            {
                return "Mismatch in the number of alphas";
            }

            if (!m_links.empty() && nVerts != (int)m_links.size())
            {
                return "Mismatch in the number of vertex-bone links";
            }

            for (size_t i = 0; i < m_links.size(); ++i)
            {
                if (m_links[i].links.empty())
                {
                    return "Found a vertex without bone linking";
                }
            }

            if (!m_vertexMatIds.empty() && nVerts != (int)m_vertexMatIds.size())
            {
                return "Mismatch in the number of vertex materials";
            }

            if (!m_aux.empty() && nVerts != (int)(m_aux.size() / m_auxSizeof))
            {
                return "Mismatch in the number of auxiliary elements";
            }

            if (!m_faceMatIds.empty() && nFaces != (int)m_faceMatIds.size())
            {
                return "Mismatch in the number of face materials";
            }

            return 0;
        }

        //////////////////////////////////////////////////////////////////////////
        // Computation

        void RemoveDegenerateFaces()
        {
            int writePos = 0;
            for (int readPos = 0; readPos < (int)m_faces.size(); ++readPos)
            {
                const Face& face = m_faces[readPos];
                if (face.vertexIndex[0] != face.vertexIndex[1] &&
                    face.vertexIndex[1] != face.vertexIndex[2] &&
                    face.vertexIndex[0] != face.vertexIndex[2])
                {
                    m_faces[writePos] = m_faces[readPos];
                    if (!m_faceMatIds.empty())
                    {
                        m_faceMatIds[writePos] = m_faceMatIds[readPos];
                    }
                    ++writePos;
                }
            }
            m_faces.resize(writePos);
            if (!m_faceMatIds.empty())
            {
                m_faceMatIds.resize(writePos);
            }
        }

        int AddVertexCopy(int sourceVertexIndex)
        {
            if (sourceVertexIndex < 0 || sourceVertexIndex >= m_positions.size())
            {
                assert(0);
                return -1;
            }

            m_positions.push_back(m_positions[sourceVertexIndex]);
            if (!m_topologyIds.empty())
            {
                m_topologyIds.push_back(m_topologyIds[sourceVertexIndex]);
            }
            if (!m_normals.empty())
            {
                m_normals.push_back(m_normals[sourceVertexIndex]);
            }
            for (uint streamIndex = 0; streamIndex < m_texCoords.size(); ++streamIndex)
            {
                if (!m_texCoords[streamIndex].empty())
                {
                    m_texCoords[streamIndex].push_back(m_texCoords[streamIndex][sourceVertexIndex]);
                }
            }
            if (!m_colors.empty())
            {
                m_colors.push_back(m_colors[sourceVertexIndex]);
            }
            if (!m_alphas.empty())
            {
                m_alphas.push_back(m_alphas[sourceVertexIndex]);
            }
            if (!m_links.empty())
            {
                m_links.push_back(m_links[sourceVertexIndex]);
            }
            if (!m_vertexMatIds.empty())
            {
                m_vertexMatIds.push_back(m_vertexMatIds[sourceVertexIndex]);
            }
            if (!m_aux.empty())
            {
                m_aux.resize(m_aux.size() + m_auxSizeof);
                memcpy(&m_aux[m_aux.size() - m_auxSizeof], &m_aux[sourceVertexIndex * m_auxSizeof], m_auxSizeof);
            }
            return (int)m_positions.size() - 1;
        }

        // Note: might create new vertices and modify vertex indices in faces
        void SetVertexMaterialIdsFromFaceMaterialIds()
        {
            m_vertexMatIds.clear();
            if (m_faceMatIds.empty())
            {
                return;
            }
            m_vertexMatIds.resize(m_positions.size(), -1);

            for (size_t i = 0; i < m_faces.size(); ++i)
            {
                const int faceMatId = m_faceMatIds[i];
                for (int j = 0; j < 3; ++j)
                {
                    int v = m_faces[i].vertexIndex[j];
                    if (m_vertexMatIds[v] >= 0 && m_vertexMatIds[v] != faceMatId)
                    {
                        v = AddVertexCopy(v);
                        m_faces[i].vertexIndex[j] = v;
                    }
                    m_vertexMatIds[v] = faceMatId;
                }
            }
        }

        // Computes m_vertexOldToNew and m_vertexNewToOld by detecting duplicate vertices
        void ComputeVertexRemapping()
        {
            const size_t nVerts = m_positions.size();

            m_vertexNewToOld.resize(nVerts);
            for (size_t i = 0; i < nVerts; ++i)
            {
                m_vertexNewToOld[i] = i;
            }

            VertexLess less(*this);
            std::sort(m_vertexNewToOld.begin(), m_vertexNewToOld.end(), less);

            m_vertexOldToNew.resize(nVerts);

            int nVertsNew = 0;
            for (size_t i = 0; i < nVerts; ++i)
            {
                if (i == 0 || less(m_vertexNewToOld[i - 1], m_vertexNewToOld[i]))
                {
                    m_vertexNewToOld[nVertsNew++] = m_vertexNewToOld[i];
                }
                m_vertexOldToNew[m_vertexNewToOld[i]] = nVertsNew - 1;
            }
            m_vertexNewToOld.resize(nVertsNew);
        }

        // Changes order of vertices, number of vertices, vertex indices in faces
        void RemoveVerticesByUsingComputedRemapping()
        {
            CompactVertices(m_positions, m_vertexNewToOld);
            CompactVertices(m_topologyIds, m_vertexNewToOld);
            CompactVertices(m_normals, m_vertexNewToOld);
            for (uint streamIndex = 0; streamIndex < m_texCoords.size(); ++streamIndex)
            {
                CompactVertices(m_texCoords[streamIndex], m_vertexNewToOld);
            }
            CompactVertices(m_colors, m_vertexNewToOld);
            CompactVertices(m_alphas, m_vertexNewToOld);
            CompactVertices(m_links, m_vertexNewToOld);
            CompactVertices(m_vertexMatIds, m_vertexNewToOld);
            CompactVerticesRaw(m_aux, m_auxSizeof, m_vertexNewToOld);

            for (size_t i = 0, count = m_faces.size(); i < count; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    const int oldVertedIdx = m_faces[i].vertexIndex[j];
                    assert(oldVertedIdx >= 0 && (size_t)oldVertedIdx < m_vertexOldToNew.size());
                    const int newVertexIndex = m_vertexOldToNew[oldVertedIdx];
                    m_faces[i].vertexIndex[j] = newVertexIndex;
                }
            }
        }

        // Deleting degraded faces (faces with two or more vertices
        // sharing same position in space)
        void RemoveDegradedFaces()
        {
            size_t j = 0;

            for (size_t i = 0, count = m_faces.size(); i < count; ++i)
            {
                const Vec3& p0 = m_positions[m_faces[i].vertexIndex[0]];
                const Vec3& p1 = m_positions[m_faces[i].vertexIndex[1]];
                const Vec3& p2 = m_positions[m_faces[i].vertexIndex[2]];
                if (p0 != p1 && p1 != p2 && p2 != p0)
                {
                    m_faces[j] = m_faces[i];
                    if (!m_faceMatIds.empty())
                    {
                        m_faceMatIds[j] = m_faceMatIds[i];
                    }
                    ++j;
                }
            }

            m_faces.resize(j);
            if (!m_faceMatIds.empty())
            {
                m_faceMatIds.resize(j);
            }
        }

    private:
        //////////////////////////////////////////////////////////////////////////
        // Internal helpers

        template<class T>
        static void CompactVertices(std::vector<T>& arr, const std::vector<int>& newToOld)
        {
            if (arr.empty())
            {
                return;
            }

            const size_t newCount = newToOld.size();

            std::vector<T> tmp;
            tmp.reserve(newCount);

            for (size_t i = 0; i < newCount; ++i)
            {
                tmp.push_back(arr[newToOld[i]]);
            }

            arr.swap(tmp);
        }

        static void CompactVerticesRaw(std::vector<uint8>& arr, size_t elemSizeof, const std::vector<int>& newToOld)
        {
            if (arr.empty())
            {
                return;
            }

            const size_t newCount = newToOld.size();

            std::vector<uint8> tmp;
            tmp.resize(newCount * elemSizeof);

            for (size_t i = 0; i < newCount; ++i)
            {
                memcpy(&tmp[i * elemSizeof], &arr[newToOld[i] * elemSizeof], elemSizeof);
            }

            arr.swap(tmp);
        }

        struct VertexLess
        {
            const Mesh& m;

            VertexLess(const Mesh& mesh)
                : m(mesh)
            {
            }

            bool operator()(int a, int b) const
            {
                if (!m.m_topologyIds.empty())
                {
                    const int res = m.m_topologyIds[a] - m.m_topologyIds[b];
                    if (res != 0)
                    {
                        return res < 0;
                    }
                }

                {
                    const int res = memcmp(&m.m_positions[a], &m.m_positions[b], sizeof(m.m_positions[0]));
                    if (res != 0)
                    {
                        return res < 0;
                    }
                }

                int res = 0;

                if (res == 0 && !m.m_normals.empty())
                {
                    res = memcmp(&m.m_normals[a], &m.m_normals[b], sizeof(m.m_normals[0]));
                }

                for (uint streamIndex = 0; streamIndex < m.m_texCoords.size(); ++streamIndex)
                {
                    if (res == 0 && !m.m_texCoords[streamIndex].empty())
                    {
                        res = memcmp(&m.m_texCoords[streamIndex][a], &m.m_texCoords[streamIndex][b], sizeof(m.m_texCoords[streamIndex][0]));
                    }
                }

                if (res == 0 && !m.m_colors.empty())
                {
                    res = memcmp(&m.m_colors[a], &m.m_colors[b], sizeof(m.m_colors[0]));
                }

                if (res == 0 && !m.m_alphas.empty())
                {
                    res = (int)m.m_alphas[a] - (int)m.m_alphas[b];
                }

                if (res == 0 && !m.m_links.empty())
                {
                    if (m.m_links[a].links.size() != m.m_links[b].links.size())
                    {
                        res = (m.m_links[a].links.size() < m.m_links[b].links.size()) ? -1 : +1;
                    }
                    else
                    {
                        res = memcmp(&m.m_links[a].links[0], &m.m_links[b].links[0], sizeof(m.m_links[a].links[0]) * m.m_links[a].links.size());
                    }
                }

                if (res == 0 && !m.m_vertexMatIds.empty())
                {
                    res = m.m_vertexMatIds[a] - m.m_vertexMatIds[b];
                }

                if (res == 0 && !m.m_aux.empty())
                {
                    res = memcmp(&m.m_aux[a * m.m_auxSizeof], &m.m_aux[b * m.m_auxSizeof], m.m_auxSizeof);
                }

                return res < 0;
            }
        };
    };
} // namespace MeshUtils

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MESHUTILS_H
