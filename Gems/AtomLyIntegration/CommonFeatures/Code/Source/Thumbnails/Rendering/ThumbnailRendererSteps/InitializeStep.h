/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
                static constexpr float AspectRatio = 1.0f;
                static constexpr float NearDist = 0.1f;
                static constexpr float FarDist = 100.0f;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

