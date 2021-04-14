/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
