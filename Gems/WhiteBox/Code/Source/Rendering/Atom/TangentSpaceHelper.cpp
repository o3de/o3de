/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBox_precompiled.h"

#include "TangentSpaceHelper.h"

#include <AzCore/std/containers/array.h>

namespace WhiteBox
{
    void AZTangentSpaceCalculation::Calculate(
        const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<uint32_t>& indices,
        const AZStd::vector<AZ::Vector2>& uvs)
    {
        AZ_Error(
            "AZTangentSpaceCalculation", (indices.size() % 3) == 0,
            "Size of list of indices (%d) is not a multiple of 3.", indices.size());

        const size_t triangleCount = indices.size() / 3;
        const size_t vertexCount = vertices.size();

        // Reset results with the right number of elements
        m_baseVectors = AZStd::vector<Base33>(vertexCount);

        using TriangleIndices = AZStd::array<uint32_t, 3>;
        using TrianglePositions = AZStd::array<AZ::Vector3, 3>;
        using TriangleUVs = AZStd::array<AZ::Vector3, 3>;
        using TriangleEdges = AZStd::array<AZ::Vector3, 2>;

        AZStd::vector<TriangleIndices> trianglesIndices;
        AZStd::vector<TrianglePositions> trianglesPositions;
        AZStd::vector<TriangleUVs> trianglesUVs;
        AZStd::vector<TriangleEdges> trianglesEdges;

        trianglesIndices.reserve(triangleCount);
        trianglesPositions.reserve(triangleCount);
        trianglesUVs.reserve(triangleCount);
        trianglesEdges.reserve(triangleCount);

        // Precalculate triangles' indices, positions, UVs and edges.
        for (AZ::u32 i = 0; i < triangleCount; ++i)
        {
            const TriangleIndices triangleIndices = {{indices[i * 3 + 0], indices[i * 3 + 1], indices[i * 3 + 2]}};
            const TrianglePositions trianglePositions = {
                {(vertices[triangleIndices[0]]), (vertices[triangleIndices[1]]), (vertices[triangleIndices[2]])}};
            const TriangleUVs triangleUVs = {
                {AZ::Vector3(uvs[triangleIndices[0]].GetX(), uvs[triangleIndices[0]].GetY(), 0.0f),
                 AZ::Vector3(uvs[triangleIndices[1]].GetX(), uvs[triangleIndices[1]].GetY(), 0.0f),
                 AZ::Vector3(uvs[triangleIndices[2]].GetX(), uvs[triangleIndices[2]].GetY(), 0.0f)}};
            const TriangleEdges triangleEdges = {
                {trianglePositions[1] - trianglePositions[0], trianglePositions[2] - trianglePositions[0]}};
            trianglesIndices.push_back(AZStd::move(triangleIndices));
            trianglesPositions.push_back(AZStd::move(trianglePositions));
            trianglesUVs.push_back(AZStd::move(triangleUVs));
            trianglesEdges.push_back(AZStd::move(triangleEdges));
        }

        // base vectors per triangle
        AZStd::vector<Base33> triangleBases;
        triangleBases.reserve(triangleCount);

        // calculate the base vectors per triangle
        {
            const float identityInfluence = 0.01f;
            const Base33 identityBase(
                AZ::Vector3(identityInfluence, 0.0f, 0.0f), AZ::Vector3(0.0f, identityInfluence, 0.0f),
                AZ::Vector3(0.0f, 0.0f, identityInfluence));

            for (AZ::u32 i = 0; i < triangleCount; ++i)
            {
#if defined(AZ_ENABLE_TRACING)
                const auto& trianglePositions = trianglesPositions[i];
#endif
                const auto& triangleUVs = trianglesUVs[i];
                const auto& triangleEdges = trianglesEdges[i];

                // calculate tangent vectors

                AZ::Vector3 normal = triangleEdges[0].Cross(triangleEdges[1]);

                // Avoid situations where the edges are parallel resulting in an invalid normal.
                // This can happen if the simulation moves particles of triangle to the same spot or very far away.
                if (normal.IsZero(0.0001f))
                {
                    // Use the identity base with low influence to leave other valid triangles to
                    // affect these vertices. In case no other triangle affects the vertices the base
                    // will still be valid with identity values as it gets normalized later.
                    triangleBases.push_back(identityBase);
                    continue;
                }

                normal.Normalize();

                const float deltaU1 = triangleUVs[1].GetX() - triangleUVs[0].GetX();
                const float deltaU2 = triangleUVs[2].GetX() - triangleUVs[0].GetX();
                const float deltaV1 = triangleUVs[1].GetY() - triangleUVs[0].GetY();
                const float deltaV2 = triangleUVs[2].GetY() - triangleUVs[0].GetY();

                const float div = (deltaU1 * deltaV2 - deltaU2 * deltaV1);

                if (_isnan(div))
                {
                    AZ_Error(
                        "AZTangentSpaceCalculation", false,
                        "Vertices 0,1,2 have broken texture coordinates v0:(%f : %f : %f) v1:(%f : %f : %f) v2:(%f : "
                        "%f : %f)",
                        float(trianglePositions[0].GetX()), float(trianglePositions[0].GetY()),
                        float(trianglePositions[0].GetZ()), float(trianglePositions[1].GetX()),
                        float(trianglePositions[1].GetY()), float(trianglePositions[1].GetZ()),
                        float(trianglePositions[2].GetX()), float(trianglePositions[2].GetY()),
                        float(trianglePositions[2].GetZ()));
                    return;
                }

                AZ::Vector3 tangent, bitangent;

                if (div != 0.0f)
                {
                    // 2D triangle area = (u1*v2-u2*v1)/2
                    const float a = deltaV2; // /div was removed - no required because of normalize()
                    const float b = -deltaV1;
                    const float c = -deltaU2;
                    const float d = deltaU1;

                    // /fAreaMul2*fAreaMul2 was optimized away -> small triangles in UV should contribute less and
                    // less artifacts (no divide and multiply)
                    tangent = (triangleEdges[0] * a + triangleEdges[1] * b) * fsgnf(div);
                    bitangent = (triangleEdges[0] * c + triangleEdges[1] * d) * fsgnf(div);
                }
                else
                {
                    tangent = AZ::Vector3(1.0f, 0.0f, 0.0f);
                    bitangent = AZ::Vector3(0.0f, 1.0f, 0.0f);
                }

                triangleBases.push_back(Base33(tangent, bitangent, normal));
            }
        }

        // distribute the normals and uv vectors to the vertices
        {
            // we create a new tangent base for every vertex index that has a different normal (later we split further
            // for mirrored use) and sum the base vectors (weighted by angle and mirrored if necessary)
            for (AZ::u32 i = 0; i < triangleCount; ++i)
            {
                const auto& triangleIndices = trianglesIndices[i];
                const auto& trianglePositions = trianglesPositions[i];

                Base33& triBase = triangleBases[i];

                // for each triangle vertex
                for (AZ::u32 e = 0; e < 3; ++e)
                {
                    // weight by angle to fix the L-Shape problem
                    const float weight = CalcAngleBetween(
                        trianglePositions[(e + 2) % 3] - trianglePositions[e],
                        trianglePositions[(e + 1) % 3] - trianglePositions[e]);

                    triBase.m_normal *= AZStd::max(weight, 0.0001f);
                    triBase.m_tangent *= weight;
                    triBase.m_bitangent *= weight;

                    AddNormalToBase(triangleIndices[e], triBase.m_normal);
                    AddUVToBase(triangleIndices[e], triBase.m_tangent, triBase.m_bitangent);
                }
            }
        }

        // adjust the base vectors per vertex
        {
            for (auto& ref : m_baseVectors)
            {
                // rotate u and v in n plane

                AZ::Vector3 nOut = ref.m_normal;
                nOut.Normalize();

                // project u in n plane
                // project v in n plane
                AZ::Vector3 uOut = ref.m_tangent - nOut * (nOut.Dot(ref.m_tangent));
                AZ::Vector3 vOut = ref.m_bitangent - nOut * (nOut.Dot(ref.m_bitangent));

                ref.m_tangent = uOut;
                ref.m_tangent.Normalize();

                ref.m_bitangent = vOut;
                ref.m_bitangent.Normalize();

                ref.m_normal = nOut;
            }
        }

        AZ_Error(
            "AZTangentSpaceCalculation", GetBaseCount() == vertices.size(),
            "Number of tangent spaces (%d) doesn't match with the number of input vertices (%d).", GetBaseCount(),
            vertices.size());
    }

    size_t AZTangentSpaceCalculation::GetBaseCount() const
    {
        return m_baseVectors.size();
    }

    void AZTangentSpaceCalculation::GetBase(
        AZ::u32 index, AZ::Vector3& tangent, AZ::Vector3& bitangent, AZ::Vector3& normal) const
    {
        tangent = GetTangent(index);
        bitangent = GetBitangent(index);
        normal = GetNormal(index);
    }

    AZ::Vector3 AZTangentSpaceCalculation::GetTangent(AZ::u32 index) const
    {
        return m_baseVectors[index].m_tangent;
    }

    AZ::Vector3 AZTangentSpaceCalculation::GetBitangent(AZ::u32 index) const
    {
        return m_baseVectors[index].m_bitangent;
    }

    AZ::Vector3 AZTangentSpaceCalculation::GetNormal(AZ::u32 index) const
    {
        return m_baseVectors[index].m_normal;
    }

    void AZTangentSpaceCalculation::AddNormalToBase(AZ::u32 index, const AZ::Vector3& normal)
    {
        m_baseVectors[index].m_normal += normal;
    }

    void AZTangentSpaceCalculation::AddUVToBase(AZ::u32 index, const AZ::Vector3& u, const AZ::Vector3& v)
    {
        m_baseVectors[index].m_tangent += u;
        m_baseVectors[index].m_bitangent += v;
    }

    float AZTangentSpaceCalculation::CalcAngleBetween(const AZ::Vector3& a, const AZ::Vector3& b)
    {
        double lengthQ = sqrt(a.GetLengthSq() * b.GetLengthSq());

        // to prevent division by zero
        lengthQ = AZStd::max(lengthQ, 1e-8);

        double cosAngle = a.Dot(b) / lengthQ;

        // acosf is not available on every platform. acos_tpl clamps cosAngle to [-1,1].
        return static_cast<float>(acos_tpl(cosAngle));
    }

    AZTangentSpaceCalculation::Base33::Base33(
        const AZ::Vector3& tangent, const AZ::Vector3& bitangent, const AZ::Vector3& normal)
        : m_tangent(tangent)
        , m_bitangent(bitangent)
        , m_normal(normal)
    {
    }
} // namespace WhiteBox
