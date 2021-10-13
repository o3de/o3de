/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SharedPreview/SharedPreviewRendererState.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! SharedPreviewRendererInitState sets up RPI system and scene and prepares it for rendering thumbnail entities
            //! This state is only called once when SharedPreviewRenderer begins rendering its first thumbnail
            class SharedPreviewRendererInitState
                : public SharedPreviewRendererState
            {
            public:
                SharedPreviewRendererInitState(SharedPreviewRendererContext* context);

                void Start() override;

            private:
                static constexpr float AspectRatio = 1.0f;
                static constexpr float NearDist = 0.1f;
                static constexpr float FarDist = 100.0f;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

