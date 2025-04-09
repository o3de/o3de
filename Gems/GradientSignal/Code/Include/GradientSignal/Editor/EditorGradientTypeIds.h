/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>

namespace GradientSignal
{
    // Gradients
    inline constexpr AZ::TypeId EditorConstantGradientComponentTypeId{ "{A878736F-39B0-482B-89B8-FD0F083E4D88}" };
    inline constexpr AZ::TypeId EditorGradientBakerComponentTypeId { "{69B2CD98-A503-4CA8-ADA2-05CE53D396CB}" };
    inline constexpr AZ::TypeId EditorImageGradientComponentTypeId { "{4E20A1C2-CEFF-4E9F-BEFF-C18257DFEC38}" };
    inline constexpr AZ::TypeId EditorPerlinGradientComponentTypeId { "{99C0B5DF-9BB4-44EE-9BDF-1141ECE26D19}" };
    inline constexpr AZ::TypeId EditorRandomGradientComponentTypeId { "{B590FB2E-33ED-4F7F-AA12-1FE76278D880}" };
    inline constexpr AZ::TypeId EditorShapeAreaFalloffGradientComponentTypeId { "{473AAC7F-990C-4761-866A-6F451C92DB3B}" };
    inline constexpr AZ::TypeId EditorSurfaceAltitudeGradientComponentTypeId { "{41A7009A-53E4-44B3-B23D-7D02266A3D9D}" };
    inline constexpr AZ::TypeId EditorSurfaceMaskGradientComponentTypeId { "{B5395FBC-FC19-4259-8A57-00C92FE1A2A2}" };
    inline constexpr AZ::TypeId EditorSurfaceSlopeGradientComponentTypeId { "{915D3430-BB7D-47F2-ABE0-3B562636F7A9}" };

    // Gradient Modifiers
    inline constexpr AZ::TypeId EditorDitherGradientComponentTypeId { "{D20E867C-692A-4056-B5AC-B372CFBAC524}" };
    inline constexpr AZ::TypeId EditorMixedGradientComponentTypeId { "{90642402-6C5F-4C4D-B1F0-8C0F242A2716}" };
    inline constexpr AZ::TypeId EditorInvertGradientComponentTypeId { "{BC63EA1C-E06B-4771-BF5D-4AE2B54AE494}" };
    inline constexpr AZ::TypeId EditorLevelsGradientComponentTypeId { "{227E7DB6-E1FE-438E-B837-2D4604DA1DEC}" };
    inline constexpr AZ::TypeId EditorPosterizeGradientComponentTypeId { "{CC21E213-19B6-46D6-B774-40F20B0950D7}" };
    inline constexpr AZ::TypeId EditorSmoothStepGradientComponentTypeId { "{3CDEA65E-519A-468E-884E-3F047FD7791F}" };
    inline constexpr AZ::TypeId EditorThresholdGradientComponentTypeId { "{1AB9BC5A-8EE3-4527-87AD-7E34AC438879}" };
}
