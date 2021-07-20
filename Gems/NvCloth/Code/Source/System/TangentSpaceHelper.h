/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/containers/array.h>

#include <NvCloth/ITangentSpaceHelper.h>

namespace NvCloth
{
    //! Implementation of the ITangentSpaceHelper interface.
    class TangentSpaceHelper
        : public AZ::Interface<ITangentSpaceHelper>::Registrar
    {
    public:
        AZ_RTTI(TangentSpaceHelper, "{2F8400BF-045A-49C3-B9D1-356011907E62}", ITangentSpaceHelper);

    protected:
        // ITangentSpace overrides ...
        bool CalculateNormals(
            const AZStd::vector<SimParticleFormat>& vertices,
            const AZStd::vector<SimIndexType>& indices,
            AZStd::vector<AZ::Vector3>& outNormals) override;
        bool CalculateTangentsAndBitagents(
            const AZStd::vector<SimParticleFormat>& vertices,
            const AZStd::vector<SimIndexType>& indices,
            const AZStd::vector<SimUVType>& uvs,
            const AZStd::vector<AZ::Vector3>& normals,
            AZStd::vector<AZ::Vector3>& outTangents,
            AZStd::vector<AZ::Vector3>& outBitangents) override;
        bool CalculateTangentSpace(
            const AZStd::vector<SimParticleFormat>& vertices,
            const AZStd::vector<SimIndexType>& indices,
            const AZStd::vector<SimUVType>& uvs,
            AZStd::vector<AZ::Vector3>& outTangents,
            AZStd::vector<AZ::Vector3>& outBitangents,
            AZStd::vector<AZ::Vector3>& outNormals) override;

    private:
        using TriangleIndices = AZStd::array<SimIndexType, 3>;
        using TrianglePositions = AZStd::array<AZ::Vector3, 3>;
        using TriangleUVs = AZStd::array<SimUVType, 3>;
        using TriangleEdges = AZStd::array<AZ::Vector3, 2>;

        void GetTriangleData(
            size_t triangleIndex,
            const AZStd::vector<SimIndexType>& indices,
            const AZStd::vector<SimParticleFormat>& vertices,
            TriangleIndices& triangleIndices,
            TrianglePositions& trianglePositions,
            TriangleEdges& triangleEdges);

        void GetTriangleData(
            size_t triangleIndex,
            const AZStd::vector<SimIndexType>& indices,
            const AZStd::vector<SimParticleFormat>& vertices,
            const AZStd::vector<SimUVType>& uvs,
            TriangleIndices& triangleIndices,
            TrianglePositions& trianglePositions,
            TriangleEdges& triangleEdges,
            TriangleUVs& triangleUVs);

        bool ComputeNormal(const TriangleEdges& triangleEdges, AZ::Vector3& normal);

        bool ComputeTangentAndBitangent(
            const TriangleUVs& triangleUVs, const TriangleEdges& triangleEdges,
            AZ::Vector3& tangent, AZ::Vector3& bitangent);

        void AdjustTangentAndBitangent(
            const AZ::Vector3& normal, AZ::Vector3& tangent, AZ::Vector3& bitangent);

        float GetVertexWeightInTriangle(AZ::u32 vertexIndexInTriangle, const TrianglePositions& trianglePositions);
    };
} // namespace NvCloth
