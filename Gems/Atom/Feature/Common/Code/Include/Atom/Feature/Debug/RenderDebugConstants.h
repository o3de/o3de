/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AZCore/Math/Vector3.h>

namespace AZ::Render
{
    // ! ENUM MUST MATCH DEBUG.AZSLI !
    // Options controlling various aspects of the rendering
    enum class RenderDebugOptions : u32
    {
        UseVertexNormal,
        IgnoreDetailNormal,

        OverrideAlbedo,
        OverrideRoughness,
        OverrideMetallic,

        UseDebugLight,

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

        Albedo,
        Roughness,
        Metallic,

        DiffuseLighting,
        SpecularLighting,

        Count
    };

    namespace RenderDebug
    {
        // static constexpr float DefaultStrength = 1.0f;
    }
}
