/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <System/TangentSpaceHelper.h>

#include <AzCore/Debug/Profiler.h>

namespace NvCloth
{
    namespace
    {
        const float Tolerance = 1e-7f;
    }

    bool TangentSpaceHelper::CalculateNormals(
        const AZStd::vector<SimParticleFormat>& vertices,
        const AZStd::vector<SimIndexType>& indices,
        AZStd::vector<AZ::Vector3>& outNormals)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        if ((indices.size() % 3) != 0)
        {
            AZ_Error("TangentSpaceHelper", false,
                "Size of list of indices (%zu) is not a multiple of 3.",
                indices.size());
            return false;
        }

        const size_t triangleCount = indices.size() / 3;
        const size_t vertexCount = vertices.size();

        // Reset results
        outNormals.resize(vertexCount);
        AZStd::fill(outNormals.begin(), outNormals.end(), AZ::Vector3::CreateZero());

        // calculate the normals per triangle
        for (size_t i = 0; i < triangleCount; ++i)
        {
            TriangleIndices triangleIndices;
            TrianglePositions trianglePositions;
            TriangleEdges triangleEdges;
            GetTriangleData(
                i, indices, vertices,
                triangleIndices, trianglePositions, triangleEdges);

            AZ::Vector3 normal;
            ComputeNormal(triangleEdges, normal);

            // distribute the normals to the vertices.
            for (AZ::u32 vertexIndexInTriangle = 0; vertexIndexInTriangle < 3; ++vertexIndexInTriangle)
            {
                const float weight = GetVertexWeightInTriangle(vertexIndexInTriangle, trianglePositions);

                const SimIndexType vertexIndex = triangleIndices[vertexIndexInTriangle];

                outNormals[vertexIndex] += normal * AZStd::max(weight, Tolerance);
            }
        }

        // adjust the normals per vertex
        for (auto& outNormal : outNormals)
        {
            outNormal.NormalizeSafe(Tolerance);

            // Safety check for situations where simulation gets out of control.
            // Particles' positions can have huge floating point values that
            // could lead to non-finite numbers when calculating tangent spaces.
            if (!outNormal.IsFinite())
            {
                outNormal = AZ::Vector3::CreateAxisZ();
            }
        }

        return true;
    }

    bool TangentSpaceHelper::CalculateTangentsAndBitagents(
        const AZStd::vector<SimParticleFormat>& vertices,
        const AZStd::vector<SimIndexType>& indices,
        const AZStd::vector<SimUVType>& uvs,
        const AZStd::vector<AZ::Vector3>& normals,
        AZStd::vector<AZ::Vector3>& outTangents,
        AZStd::vector<AZ::Vector3>& outBitangents)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        if ((indices.size() % 3) != 0)
        {
            AZ_Error("TangentSpaceHelper", false,
                "Size of list of indices (%zu) is not a multiple of 3.",
                indices.size());
            return false;
        }

        if (vertices.size() != uvs.size())
        {
            AZ_Error("TangentSpaceHelper", false,
                "Number of vertices (%zu) does not match the number of uvs (%zu).",
                vertices.size(), uvs.size());
            return false;
        }

        if (vertices.size() != normals.size())
        {
            AZ_Error("TangentSpaceHelper", false,
                "Number of vertices (%zu) does not match the number of normals (%zu).",
                vertices.size(), normals.size());
            return false;
        }

        const size_t triangleCount = indices.size() / 3;
        const size_t vertexCount = vertices.size();

        // Reset results
        outTangents.resize(vertexCount);
        outBitangents.resize(vertexCount);
        AZStd::fill(outTangents.begin(), outTangents.end(), AZ::Vector3::CreateZero());
        AZStd::fill(outBitangents.begin(), outBitangents.end(), AZ::Vector3::CreateZero());

        // calculate the base vectors per triangle
        for (size_t i = 0; i < triangleCount; ++i)
        {
            TriangleIndices triangleIndices;
            TrianglePositions trianglePositions;
            TriangleEdges triangleEdges;
            TriangleUVs triangleUVs;
            GetTriangleData(
                i, indices, vertices, uvs,
                triangleIndices, trianglePositions, triangleEdges, triangleUVs);

            AZ::Vector3 tangent, bitangent;
            ComputeTangentAndBitangent(triangleUVs, triangleEdges, tangent, bitangent);

            // distribute the uv vectors to the vertices.
            for (AZ::u32 vertexIndexInTriangle = 0; vertexIndexInTriangle < 3; ++vertexIndexInTriangle)
            {
                const float weight = GetVertexWeightInTriangle(vertexIndexInTriangle, trianglePositions);

                const SimIndexType vertexIndex = triangleIndices[vertexIndexInTriangle];

                outTangents[vertexIndex] += tangent * weight;
                outBitangents[vertexIndex] += bitangent * weight;
            }
        }

        // adjust the base vectors per vertex
        for (size_t i = 0; i < vertexCount; ++i)
        {
            AdjustTangentAndBitangent(normals[i], outTangents[i], outBitangents[i]);

            // Safety check for situations where simulation gets out of control.
            // Particles' positions can have huge floating point values that
            // could lead to non-finite numbers when calculating tangent spaces.
            if (!outTangents[i].IsFinite() ||
                !outBitangents[i].IsFinite())
            {
                outTangents[i] = AZ::Vector3::CreateAxisX();
                outBitangents[i] = AZ::Vector3::CreateAxisY();
            }
        }

        return true;
    }

    bool TangentSpaceHelper::CalculateTangentSpace(
        const AZStd::vector<SimParticleFormat>& vertices,
        const AZStd::vector<SimIndexType>& indices,
        const AZStd::vector<SimUVType>& uvs,
        AZStd::vector<AZ::Vector3>& outTangents,
        AZStd::vector<AZ::Vector3>& outBitangents,
        AZStd::vector<AZ::Vector3>& outNormals)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        if ((indices.size() % 3) != 0)
        {
            AZ_Error("TangentSpaceHelper", false,
                "Size of list of indices (%zu) is not a multiple of 3.",
                indices.size());
            return false;
        }

        if (vertices.size() != uvs.size())
        {
            AZ_Error("TangentSpaceHelper", false,
                "Number of vertices (%zu) does not match the number of uvs (%zu).",
                vertices.size(), uvs.size());
            return false;
        }

        const size_t triangleCount = indices.size() / 3;
        const size_t vertexCount = vertices.size();

        // Reset results
        outTangents.resize(vertexCount);
        outBitangents.resize(vertexCount);
        outNormals.resize(vertexCount);
        AZStd::fill(outTangents.begin(), outTangents.end(), AZ::Vector3::CreateZero());
        AZStd::fill(outBitangents.begin(), outBitangents.end(), AZ::Vector3::CreateZero());
        AZStd::fill(outNormals.begin(), outNormals.end(), AZ::Vector3::CreateZero());

        // calculate the base vectors per triangle
        for (size_t i = 0; i < triangleCount; ++i)
        {
            TriangleIndices triangleIndices;
            TrianglePositions trianglePositions;
            TriangleEdges triangleEdges;
            TriangleUVs triangleUVs;
            GetTriangleData(
                i, indices, vertices, uvs,
                triangleIndices, trianglePositions, triangleEdges, triangleUVs);

            AZ::Vector3 tangent, bitangent, normal;
            if (ComputeNormal(triangleEdges, normal))
            {
                ComputeTangentAndBitangent(triangleUVs, triangleEdges, tangent, bitangent);
            }
            else
            {
                // Use the identity base with low influence to leave other valid triangles to
                // affect these vertices. In case no other triangle affects the vertices the base
                // will still be valid with identity values as it gets normalized later.
                const float identityInfluence = 0.01f;
                tangent = AZ::Vector3::CreateAxisX(identityInfluence);
                bitangent = AZ::Vector3::CreateAxisY(identityInfluence);
            }

            // distribute the normals and uv vectors to the vertices.
            for (AZ::u32 vertexIndexInTriangle = 0; vertexIndexInTriangle < 3; ++vertexIndexInTriangle)
            {
                const float weight = GetVertexWeightInTriangle(vertexIndexInTriangle, trianglePositions);

                const SimIndexType vertexIndex = triangleIndices[vertexIndexInTriangle];

                outNormals[vertexIndex] += normal * AZStd::max(weight, Tolerance);
                outTangents[vertexIndex] += tangent * weight;
                outBitangents[vertexIndex] += bitangent * weight;
            }
        }

        // adjust the base vectors per vertex
        for (size_t i = 0; i < vertexCount; ++i)
        {
            outNormals[i].NormalizeSafe(Tolerance);

            AdjustTangentAndBitangent(outNormals[i], outTangents[i], outBitangents[i]);

            // Safety check for situations where simulation gets out of control.
            // Particles' positions can have huge floating point values that
            // could lead to non-finite numbers when calculating tangent spaces.
            if (!outNormals[i].IsFinite() ||
                !outTangents[i].IsFinite() ||
                !outBitangents[i].IsFinite())
            {
                outTangents[i] = AZ::Vector3::CreateAxisX();
                outBitangents[i] = AZ::Vector3::CreateAxisY();
                outNormals[i] = AZ::Vector3::CreateAxisZ();
            }
        }

        return true;
    }

    void TangentSpaceHelper::GetTriangleData(
        size_t triangleIndex,
        const AZStd::vector<SimIndexType>& indices,
        const AZStd::vector<SimParticleFormat>& vertices,
        TriangleIndices& triangleIndices,
        TrianglePositions& trianglePositions,
        TriangleEdges& triangleEdges)
    {
        triangleIndices =
        {{
            indices[triangleIndex * 3 + 0],
            indices[triangleIndex * 3 + 1],
            indices[triangleIndex * 3 + 2]
        }};
        trianglePositions =
        {{
            vertices[triangleIndices[0]].GetAsVector3(),
            vertices[triangleIndices[1]].GetAsVector3(),
            vertices[triangleIndices[2]].GetAsVector3()
        }};
        triangleEdges =
        {{
            trianglePositions[1] - trianglePositions[0],
            trianglePositions[2] - trianglePositions[0]
        }};
    }

    void TangentSpaceHelper::GetTriangleData(
        size_t triangleIndex,
        const AZStd::vector<SimIndexType>& indices,
        const AZStd::vector<SimParticleFormat>& vertices,
        const AZStd::vector<SimUVType>& uvs,
        TriangleIndices& triangleIndices,
        TrianglePositions& trianglePositions,
        TriangleEdges& triangleEdges,
        TriangleUVs& triangleUVs)
    {
        GetTriangleData(
            triangleIndex, indices, vertices,
            triangleIndices, trianglePositions, triangleEdges);
        triangleUVs =
        {{
            uvs[triangleIndices[0]],
            uvs[triangleIndices[1]],
            uvs[triangleIndices[2]]
        }};
    }

    bool TangentSpaceHelper::ComputeNormal(const TriangleEdges& triangleEdges, AZ::Vector3& normal)
    {
        normal = triangleEdges[0].Cross(triangleEdges[1]);

        // Avoid situations where the edges are parallel resulting in an invalid normal.
        // This can happen if the simulation moves particles of triangle to the same spot or very far away.
        if (normal.IsZero(Tolerance))
        {
            // Use the identity base with low influence to leave other valid triangles to
            // affect these vertices. In case no other triangle affects the vertices the base
            // will still be valid with identity values as it gets normalized later.
            const float identityInfluence = 0.01f;
            normal = AZ::Vector3::CreateAxisZ(identityInfluence);
            return false;
        }

        normal.Normalize();

        return true;
    }

    bool TangentSpaceHelper::ComputeTangentAndBitangent(
        const TriangleUVs& triangleUVs, const TriangleEdges& triangleEdges,
        AZ::Vector3& tangent, AZ::Vector3& bitangent)
    {
        const float deltaU1 = triangleUVs[1].GetX() - triangleUVs[0].GetX();
        const float deltaU2 = triangleUVs[2].GetX() - triangleUVs[0].GetX();
        const float deltaV1 = triangleUVs[1].GetY() - triangleUVs[0].GetY();
        const float deltaV2 = triangleUVs[2].GetY() - triangleUVs[0].GetY();

        const float div = (deltaU1 * deltaV2 - deltaU2 * deltaV1);

        if (AZ::IsClose(div, 0.0f, Tolerance))
        {
            tangent = AZ::Vector3::CreateAxisX();
            bitangent = AZ::Vector3::CreateAxisY();
            return false;
        }

        // 2D triangle area = (u1*v2-u2*v1)/2
        const float a = deltaV2; // /div was removed - no required because of normalize()
        const float b = -deltaV1;
        const float c = -deltaU2;
        const float d = deltaU1;

        const float signDiv = AZ::GetSign(div);

        // /fAreaMul2*fAreaMul2 was optimized away -> small triangles in UV should contribute less and
        // less artifacts (no divide and multiply)
        tangent = (triangleEdges[0] * a + triangleEdges[1] * b) * signDiv;
        bitangent = (triangleEdges[0] * c + triangleEdges[1] * d) * signDiv;

        return true;
    }

    void TangentSpaceHelper::AdjustTangentAndBitangent(
        const AZ::Vector3& normal, AZ::Vector3& tangent, AZ::Vector3& bitangent)
    {
        // Calculate handedness of the bitangent
        AZ::Vector3 bitangentReference = normal.Cross(tangent);
        const float handedness = (bitangentReference.Dot(bitangent) < 0.0f) ? -1.0f : 1.0f;

        // Apply Gram-Schmidt method to make tangent perpendicular to normal.
        tangent -= normal * normal.Dot(tangent);
        tangent.NormalizeSafe(Tolerance);

        bitangent = normal.Cross(tangent) * handedness;
    }

    float TangentSpaceHelper::GetVertexWeightInTriangle(AZ::u32 vertexIndexInTriangle, const TrianglePositions& trianglePositions)
    {
        // weight by angle to fix the L-Shape problem
        const AZ::Vector3 edgeA = trianglePositions[(vertexIndexInTriangle + 2) % 3] - trianglePositions[vertexIndexInTriangle];
        const AZ::Vector3 edgeB = trianglePositions[(vertexIndexInTriangle + 1) % 3] - trianglePositions[vertexIndexInTriangle];
        return edgeA.AngleSafe(edgeB);
    }
} // namespace NvCloth
