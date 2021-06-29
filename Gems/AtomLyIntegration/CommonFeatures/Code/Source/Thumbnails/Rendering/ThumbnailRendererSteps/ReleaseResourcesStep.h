/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
            class ReleaseResourcesStep
                : public ThumbnailRendererStep
            {
            public:
                ReleaseResourcesStep(ThumbnailRendererContext* context);

                void Start() override;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

