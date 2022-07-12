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
#include <AzCore/std/containers/fixed_vector.h>

namespace AzFramework::SurfaceData
{
    namespace Constants
    {
        static constexpr const char* UnassignedTagName = "(unassigned)";
        static constexpr AZ::Crc32 UnassignedTagCrc = AZ::Crc32(Constants::UnassignedTagName);

        //! The maximum number of surface weights that we can store.
        //! For performance reasons, we want to limit this so that we can preallocate the max size in advance.
        //! The current number is chosen to be higher than expected needs, but small enough to avoid being excessively wasteful.
        //! (Dynamic structures would end up taking more memory than what we're preallocating)
        static constexpr size_t MaxSurfaceWeights = 16;
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

        //! Equality comparison operator for SurfaceTagWeight.
        bool operator==(const SurfaceTagWeight& rhs) const
        {
            return (m_surfaceType == rhs.m_surfaceType) && (m_weight == rhs.m_weight);
        }

        //! Inequality comparison operator for SurfaceTagWeight.
        bool operator!=(const SurfaceTagWeight& rhs) const
        {
            return !(*this == rhs);
        }


        AZ::Crc32 m_surfaceType = Constants::UnassignedTagCrc;
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

    using SurfaceTagWeightList = AZStd::fixed_vector<SurfaceTagWeight, Constants::MaxSurfaceWeights>;

    struct SurfacePoint final
    {
        AZ_TYPE_INFO(SurfacePoint, "{331A3D0E-BB1D-47BF-96A2-249FAA0D720D}");

        AZ::Vector3 m_position;
        AZ::Vector3 m_normal;
        SurfaceTagWeightList m_surfaceTags;

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace AzFramework::SurfaceData
