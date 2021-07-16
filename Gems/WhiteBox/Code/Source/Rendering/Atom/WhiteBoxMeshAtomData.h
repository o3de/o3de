/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
    };
} // namespace WhiteBox
