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

#include <Thumbnails/Rendering/ThumbnailRendererSteps/ThumbnailRendererStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! InitializeStep sets up RPI system and scene and prepares it for rendering thumbnail entities
            //! This step is only called once when CommonThumbnailRenderer begins rendering its first thumbnail
            class InitializeStep
                : public ThumbnailRendererStep
            {
            public:
                InitializeStep(ThumbnailRendererContext* context);

                void Start() override;

            private:
                static constexpr const char* LightingPresetPath = "lightingpresets/default.lightingpreset.azasset";
                static constexpr float AspectRatio = 1.0f;
                static constexpr float NearDist = 0.1f;
                static constexpr float FarDist = 100.0f;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

