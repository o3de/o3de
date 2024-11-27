/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    namespace Render
    {
        namespace FilmGrain
        {
            static constexpr float DefaultIntensity = 0.2f;
            static constexpr float DefaultLuminanceDampening = 0.0f;
            static constexpr float DefaultTilingScale = 1.0f;
            static const constexpr char* DefaultGrainPath = "textures/FilmGrain.jpg.streamingimage";
        } // namespace FilmGrain
    } // namespace Render
} // namespace AZ
