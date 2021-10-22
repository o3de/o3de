/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>

namespace AzFramework::SurfaceData
{
    namespace Constants
    {
        static constexpr const char* s_unassignedTagName = "(unassigned)";
    }

    struct SurfaceTagWeight
    {
        AZ_TYPE_INFO(SurfaceTagWeight, "{EA14018E-E853-4BF5-8E13-D83BB99A54CC}");
        SurfaceTagWeight() = default;
        SurfaceTagWeight(AZ::Crc32 surfaceType, float weight)
            : m_surfaceType(surfaceType)
            , m_weight(weight)
        {
        }

        AZ::Crc32 m_surfaceType = AZ::Crc32(Constants::s_unassignedTagName);
        float m_weight = 0.0f; //! A Value in the range [0.0f .. 1.0f]

        static void Reflect(AZ::ReflectContext* context);
    };

    struct SurfaceTagWeightComparator
    {
        bool operator()(const SurfaceTagWeight& tagWeight1, const SurfaceTagWeight& tagWeight2) const
        {
            // Return a deterministic sort order for surface tags from highest to lowest weight, with the surface types sorted
            // in a predictable order when the weights are equal.  The surface type sort order is meaningless since it is sorting CRC
            // values, it's really just important for it to be stable.
            // For the floating-point weight comparisons we use exact instead of IsClose value comparisons for a similar reason - we
            // care about being sorted highest to lowest, but there's no inherent meaning in sorting surface types with *similar* weights
            // together.

            if (tagWeight1.m_weight != tagWeight2.m_weight)
            {
                return tagWeight1.m_weight > tagWeight2.m_weight;
            }
            else
            {
                return tagWeight1.m_surfaceType > tagWeight2.m_surfaceType;
            }
        }
    };

    using SurfaceTagWeightList = AZStd::vector<SurfaceTagWeight>;

    struct SurfacePoint final
    {
        AZ_TYPE_INFO(SurfacePoint, "{331A3D0E-BB1D-47BF-96A2-249FAA0D720D}");

        AZ::Vector3 m_position;
        AZ::Vector3 m_normal;
        SurfaceTagWeightList m_surfaceTags;

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace AzFramework::SurfaceData
