/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>

namespace AZ::Render
{
    // ! ENUM MUST MATCH DEBUG.AZSLI !
    // Options controlling various aspects of the rendering
    enum class RenderDebugOptions : u32
    {
        DebugEnabled,

        OverrideBaseColor,
        OverrideRoughness,
        OverrideMetallic,

        UseDebugLight,

        EnableNormalMaps,
        EnableDetailNormalMaps,

        EnableDiffuseLighting,
        EnableSpecularLighting,
        EnableDirectLighting,
        EnableIndirectLighting,

        CustomDebugOption01,
        CustomDebugOption02,
        CustomDebugOption03,
        CustomDebugOption04,

        Count
    };

    // ! ENUM MUST MATCH DEBUG.AZSLI !
    // Specifies what debug info to render to the view
    enum class RenderDebugViewMode : u32
    {
        None,

        Normal,
        Tangent,
        Bitangent,

        BaseColor,
        Albedo,
        Roughness,
        Metallic,

        CascadeShadows,

        Count
    };

    enum class RenderDebugLightingType : u32
    {
        DiffuseAndSpecular,
        Diffuse,
        Specular,
        Count
    };

    enum class RenderDebugLightingSource : u32
    {
        DirectAndIndirect,
        Direct,
        Indirect,
        DebugLight,
        Count
    };
}
