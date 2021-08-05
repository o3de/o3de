/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Preprocessor/Enum.h>

namespace AZ
{
    namespace Render
    {
        namespace Shadow
        {
            AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(ShadowmapType, uint32_t,
                (Directional, 0),
                Projected);

            const Matrix4x4& GetClipToShadowmapTextureMatrix();
        } // namespace Shadow
    } // namespace Render
} // namespace AZ
