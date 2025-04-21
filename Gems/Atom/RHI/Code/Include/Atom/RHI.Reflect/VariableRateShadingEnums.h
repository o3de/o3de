/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ::RHI
{       
    //! Shading rate types supported by the RHI
    enum class ShadingRateType : uint32_t
    {
        PerDraw,        // Allows the specification of a rate per-draw.
        PerPrimitive,   // Allows the specification of a rate per primitive, specified during shading.
        PerRegion,       // Allows the specification of a rate per-region of the framebuffer, specified in a specialized image attachment.
        Count,
        Invalid = Count
    };
    static const uint32_t ShadingRateTypeCount = static_cast<uint32_t>(ShadingRateType::Count);
        
    //! Flags for specifying supported modes for setting the rate shading.
    enum class ShadingRateTypeFlags : uint32_t
    {
        None = 0,
        PerDraw = AZ_BIT(static_cast<uint32_t>(ShadingRateType ::PerDraw)),
        PerPrimitive = AZ_BIT(static_cast<uint32_t>(ShadingRateType ::PerPrimitive)),
        PerRegion = AZ_BIT(static_cast<uint32_t>(ShadingRateType ::PerRegion)),
        All = PerDraw | PerPrimitive | PerRegion
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::ShadingRateTypeFlags)

    //! Defines constants that specify the shading rate value.
    enum class ShadingRate : uint32_t
    {
        Rate1x1 = 0,// Specifies no change to the shading rate.
        Rate1x2,    // Specifies that the shading rate should reduce vertical resolution 2x.
        Rate2x1,    // Specifies that the shading rate should reduce horizontal resolution 2x.
        Rate2x2,    // Specifies that the shading rate should reduce the resolution of both axes 2x.
        Rate2x4,    // Specifies that the shading rate should reduce horizontal resolution 2x, and reduce vertical resolution 4x.
        Rate4x2,    // Specifies that the shading rate should reduce horizontal resolution 4x, and reduce vertical resolution 2x.
        Rate4x1,    // Specifies that the shading rate should reduce horizontal resolution 4x, and reduce vertical resolution 1x.
        Rate1x4,    // Specifies that the shading rate should reduce horizontal resolution 1x, and reduce vertical resolution 4x.
        Rate4x4,    // Specifies that the shading rate should reduce the resolution of both axes 4x.
        Count
    };

    //! Flags for specifying supported shading rates.
    enum class ShadingRateFlags : uint32_t
    {
        None = 0,
        Rate1x1 = AZ_BIT(static_cast<uint32_t>(ShadingRate ::Rate1x1)),
        Rate1x2 = AZ_BIT(static_cast<uint32_t>(ShadingRate ::Rate1x2)),
        Rate2x1 = AZ_BIT(static_cast<uint32_t>(ShadingRate ::Rate2x1)),
        Rate2x2 = AZ_BIT(static_cast<uint32_t>(ShadingRate ::Rate2x2)),
        Rate2x4 = AZ_BIT(static_cast<uint32_t>(ShadingRate ::Rate2x4)),
        Rate4x2 = AZ_BIT(static_cast<uint32_t>(ShadingRate ::Rate4x2)),
        Rate4x1 = AZ_BIT(static_cast<uint32_t>(ShadingRate ::Rate4x1)),
        Rate1x4 = AZ_BIT(static_cast<uint32_t>(ShadingRate ::Rate1x4)),
        Rate4x4 = AZ_BIT(static_cast<uint32_t>(ShadingRate ::Rate4x4))
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::ShadingRateFlags)

    //! Defines the operations for combining shading rates.
    enum class ShadingRateCombinerOp : uint32_t
    {
        Passthrough = 0,// Specifies the combiner C.xy = A.xy, for combiner (C) and inputs (A and B).
        Override,       // Specifies the combiner C.xy = B.xy, for combiner (C) and inputs (A and B).
        Min,            // Specifies the combiner C.xy = max(A.xy, B.xy), for combiner (C) and inputs (A and B).
        Max,            // Specifies the combiner C.xy = min(A.xy, B.xy), for combiner (C) and inputs (A and B).
        Count
    };

    //! List of combination operations that are applied to get the final value.
    //! For ShadingRateCombinators = { Op1, Op2 }, the final value is calculated as Op2(Op1(PerDraw, PerPrimitive), PerRegion)
    using ShadingRateCombinators = AZStd::array<ShadingRateCombinerOp, 2>;

    //! Represents a texel value of a shading rate.
    //! Some implementations use a two component image format, others only one.
    struct ShadingRateImageValue
    {
        uint8_t m_x; // First component value.
        uint8_t m_y; // Second component value (may be 0 if not used).
    };
}
