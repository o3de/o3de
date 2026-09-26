/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Rendering/WhiteBoxRenderData.h>

namespace WhiteBox
{
    //! Default gap between lightmap charts, in UV units (about 2.5 texels on a 512 lightmap).
    inline constexpr float DefaultLightmapMargin = 0.005f;

    //! Generate a non-overlapping UV1 set for baked lighting: coplanar charts packed into the unit square
    //! at proportional sizes, @p margin apart.
    void GenerateLightmapUvs(WhiteBoxFaces& faces, float margin = DefaultLightmapMargin);
} // namespace WhiteBox
