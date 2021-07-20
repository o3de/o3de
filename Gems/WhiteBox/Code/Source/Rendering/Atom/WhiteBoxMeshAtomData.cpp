/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBox_precompiled.h"

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
        AZStd::vector<AZ::Vector3> normals(vertCount);
        AZStd::vector<AZ::Vector2> uvs(vertCount);

        m_indices.resize_no_construct(vertCount);
        m_positions.resize_no_construct(vertCount);
        m_normals.resize_no_construct(vertCount);
        m_tangents.resize_no_construct(vertCount);
        m_bitangents.resize_no_construct(vertCount);
        m_uvs.resize_no_construct(vertCount);
        m_colors.resize(vertCount, AZ::Vector4::CreateOne());

        // populate the index vector with a [0, vertCount) sequence
        std::iota(std::begin(m_indices), std::end(m_indices), 0);

        for (size_t idxFace = 0; idxFace < faceCount; idxFace++)
        {
            const auto face = faceData[idxFace];

            // v1
            positions[idxFace * 3 + 0] = face.m_v1.m_position;
            uvs[idxFace * 3 + 0] = face.m_v1.m_uv;

            // v2
            positions[idxFace * 3 + 1] = face.m_v2.m_position;
            uvs[idxFace * 3 + 1] = face.m_v2.m_uv;

            // v3
            positions[idxFace * 3 + 2] = face.m_v3.m_position;
            uvs[idxFace * 3 + 2] = face.m_v3.m_uv;
        }

        // calculate the basis vectors for the TBN matrices
        // TODO: LYN-782
        AZTangentSpaceCalculation tangentSpaceCalculation;
        tangentSpaceCalculation.Calculate(positions, m_indices, uvs);

        for (size_t i = 0; i < vertCount; i++)
        {
            const auto normal = tangentSpaceCalculation.GetNormal(i);
            const auto tangent = tangentSpaceCalculation.GetTangent(i);
            const auto bitangent = tangentSpaceCalculation.GetBitangent(i);

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
        return m_indices.size();
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

    AZ::Aabb WhiteBoxMeshAtomData::GetAabb() const
    {
        return m_aabb;
    }
} // namespace WhiteBox
