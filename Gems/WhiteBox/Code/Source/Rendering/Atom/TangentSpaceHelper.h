/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/std/containers/vector.h>

// TODO: to be removed (LYN-782)

namespace WhiteBox
{
    class AZTangentSpaceCalculation
    {
    public:
        void Calculate(
            const AZStd::vector<AZ::Vector3>& vertices, const AZStd::vector<uint32_t>& indices,
            const AZStd::vector<AZ::Vector2>& uvs);

        size_t GetBaseCount() const;

        // Returns an orthogonal base (perpendicular and normalized)
        void GetBase(AZ::u32 index, AZ::Vector3& tangent, AZ::Vector3& bitangent, AZ::Vector3& normal) const;
        AZ::Vector3 GetTangent(AZ::u32 index) const;
        AZ::Vector3 GetBitangent(AZ::u32 index) const;
        AZ::Vector3 GetNormal(AZ::u32 index) const;

    private:
        struct Base33
        {
            Base33() = default;
            Base33(const AZ::Vector3& tangent, const AZ::Vector3& bitangent, const AZ::Vector3& normal);

            AZ::Vector3 m_tangent = AZ::Vector3(0.0f, 0.0f, 0.0f);
            AZ::Vector3 m_bitangent = AZ::Vector3(0.0f, 0.0f, 0.0f);
            AZ::Vector3 m_normal = AZ::Vector3(0.0f, 0.0f, 0.0f);
        };

        void AddNormalToBase(AZ::u32 index, const AZ::Vector3& normal);
        void AddUVToBase(AZ::u32 index, const AZ::Vector3& u, const AZ::Vector3& v);
        float CalcAngleBetween(const AZ::Vector3& a, const AZ::Vector3& b);

        AZStd::vector<Base33> m_baseVectors;
    };
} // namespace WhiteBox
