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

#pragma once

#include "PackedFloat2.h"

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/PackedVector3.h>
#include <Rendering/WhiteBoxRenderData.h>

namespace WhiteBox
{
    //! Class to hold the white box mesh data in Atom device memory format.
    class WhiteBoxMeshAtomData
    {
    public:
        explicit WhiteBoxMeshAtomData(const WhiteBoxFaces& faceData);
        ~WhiteBoxMeshAtomData() = default;

        const uint32_t VertexCount() const;
        const AZStd::vector<uint32_t>& GetIndices() const;
        const AZStd::vector<AZ::PackedVector3f>& GetPositions() const;
        const AZStd::vector<AZ::PackedVector3f>& GetNormals() const;
        const AZStd::vector<AZ::Vector4>& GetTangents() const;
        const AZStd::vector<AZ::PackedVector3f>& GetBitangents() const;
        const AZStd::vector<PackedFloat2>& GetUVs() const;
        const AZStd::vector<AZ::Vector4>& GetColors() const;
        AZ::Aabb GetAabb() const;

    private:
        AZStd::vector<uint32_t> m_indices;
        AZStd::vector<AZ::PackedVector3f> m_positions;
        AZStd::vector<AZ::PackedVector3f> m_normals;
        AZStd::vector<AZ::Vector4> m_tangents;
        AZStd::vector<AZ::PackedVector3f> m_bitangents;
        AZStd::vector<PackedFloat2> m_uvs;
        AZStd::vector<AZ::Vector4> m_colors;
        AZ::Aabb m_aabb;
    };
} // namespace WhiteBox
