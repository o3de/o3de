/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TangentSpaceHelper.h"
#include "WhiteBoxMeshAtomData.h"

#include <numeric>

namespace WhiteBox
{
    WhiteBoxMeshAtomData::WhiteBoxMeshAtomData(const WhiteBoxFaces& faceData)
    {
        const size_t faceCount = faceData.size();
        const size_t vertCount = faceCount * 3;

        // mesh vertex attribute data in host memory format
        AZStd::vector<AZ::Vector3> positions(vertCount);
        AZStd::vector<AZ::Vector3> normals(vertCount, AZ::Vector3::CreateZero()); // smoothed corners only
        AZStd::vector<AZ::Vector2> uvs(vertCount);

        m_indices.resize_no_construct(vertCount);
        m_positions.resize_no_construct(vertCount);
        m_normals.resize_no_construct(vertCount);
        m_tangents.resize_no_construct(vertCount);
        m_bitangents.resize_no_construct(vertCount);
        m_uvs.resize_no_construct(vertCount);
        m_colors.resize(vertCount, AZ::Vector4::CreateOne());
        m_lightmapUvs.resize_no_construct(vertCount);

        // populate the index vector with a [0, vertCount) sequence
        std::iota(std::begin(m_indices), std::end(m_indices), 0);

        for (size_t idxFace = 0; idxFace < faceCount; idxFace++)
        {
            const auto face = faceData[idxFace];

            // v1
            positions[idxFace * 3 + 0] = face.m_v1.m_position;
            uvs[idxFace * 3 + 0] = face.m_v1.m_uv;
            normals[idxFace * 3 + 0] = face.m_v1.m_normal;

            // v2
            positions[idxFace * 3 + 1] = face.m_v2.m_position;
            uvs[idxFace * 3 + 1] = face.m_v2.m_uv;
            normals[idxFace * 3 + 1] = face.m_v2.m_normal;

            // v3
            positions[idxFace * 3 + 2] = face.m_v3.m_position;
            uvs[idxFace * 3 + 2] = face.m_v3.m_uv;
            normals[idxFace * 3 + 2] = face.m_v3.m_normal;

            // UV1: the lightmap chart, else the texture UVs so a material reading UV1 sees what it did before.
            const WhiteBoxVertex* corners[3] = { &face.m_v1, &face.m_v2, &face.m_v3 };
            for (size_t c = 0; c < 3; ++c)
            {
                const AZ::Vector2 uv1 = face.m_hasLightmapUv ? corners[c]->m_lightmapUv : corners[c]->m_uv;
                m_lightmapUvs[idxFace * 3 + c] = { uv1.GetX(), uv1.GetY() };
            }

            // COLOR0: the per-face tint (per-layer colour), or the corners' vertex-blend weights for a custom material
            if (face.m_colorFromBlend)
            {
                m_colors[idxFace * 3 + 0] = AZ::Vector4::CreateFromVector3AndFloat(face.m_v1.m_blend, 1.0f);
                m_colors[idxFace * 3 + 1] = AZ::Vector4::CreateFromVector3AndFloat(face.m_v2.m_blend, 1.0f);
                m_colors[idxFace * 3 + 2] = AZ::Vector4::CreateFromVector3AndFloat(face.m_v3.m_blend, 1.0f);
            }
            else
            {
                m_colors[idxFace * 3 + 0] = face.m_color;
                m_colors[idxFace * 3 + 1] = face.m_color;
                m_colors[idxFace * 3 + 2] = face.m_color;
            }
        }

        // calculate the basis vectors for the TBN matrices
        AZTangentSpaceCalculation tangentSpaceCalculation;
        tangentSpaceCalculation.Calculate(positions, m_indices, uvs);

        for (size_t i = 0; i < vertCount; i++)
        {
            AZ::Vector3 normal = tangentSpaceCalculation.GetNormal(static_cast<AZ::u32>(i));
            AZ::Vector3 tangent = tangentSpaceCalculation.GetTangent(static_cast<AZ::u32>(i));
            AZ::Vector3 bitangent = tangentSpaceCalculation.GetBitangent(static_cast<AZ::u32>(i));
            // A smoothed corner replaces the flat normal; the UV tangents are re-projected onto its plane.
            if (!normals[i].IsZero())
            {
                normal = normals[i];
                tangent = (tangent - normal * normal.Dot(tangent)).GetNormalizedSafe();
                bitangent = (bitangent - normal * normal.Dot(bitangent)).GetNormalizedSafe();
            }

            m_aabb.AddPoint(positions[i]);

            // populate the mesh vertex attribute data in device memory format
            m_positions[i] = AZ::PackedVector3f(positions[i]);
            m_normals[i] = AZ::PackedVector3f(normal);
            m_tangents[i].Set(tangent, 1.0f);
            m_bitangents[i] = AZ::PackedVector3f(bitangent);
            m_uvs[i] = {uvs[i].GetX(), uvs[i].GetY()};
        }
    }

    const uint32_t WhiteBoxMeshAtomData::VertexCount() const
    {
        return static_cast<uint32_t>(m_indices.size());
    }

    const AZStd::vector<uint32_t>& WhiteBoxMeshAtomData::GetIndices() const
    {
        return m_indices;
    }

    const AZStd::vector<AZ::PackedVector3f>& WhiteBoxMeshAtomData::GetPositions() const
    {
        return m_positions;
    }

    const AZStd::vector<AZ::PackedVector3f>& WhiteBoxMeshAtomData::GetNormals() const
    {
        return m_normals;
    }

    const AZStd::vector<AZ::Vector4>& WhiteBoxMeshAtomData::GetTangents() const
    {
        return m_tangents;
    }

    const AZStd::vector<AZ::PackedVector3f>& WhiteBoxMeshAtomData::GetBitangents() const
    {
        return m_bitangents;
    }

    const AZStd::vector<PackedFloat2>& WhiteBoxMeshAtomData::GetUVs() const
    {
        return m_uvs;
    }

    const AZStd::vector<AZ::Vector4>& WhiteBoxMeshAtomData::GetColors() const
    {
        return m_colors;
    }

    const AZStd::vector<PackedFloat2>& WhiteBoxMeshAtomData::GetLightmapUVs() const
    {
        return m_lightmapUvs;
    }

    AZ::Aabb WhiteBoxMeshAtomData::GetAabb() const
    {
        return m_aabb;
    }
} // namespace WhiteBox
