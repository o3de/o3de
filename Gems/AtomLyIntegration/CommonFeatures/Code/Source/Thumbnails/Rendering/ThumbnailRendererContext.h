/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <Thumbnails/Rendering/ThumbnailRendererSteps/ThumbnailRendererStep.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            struct ThumbnailRendererData;

            enum class Step
            {
                None,
                Initialize,
                FindThumbnailToRender,
                WaitForAssetsToLoad,
                Capture,
                ReleaseResources
            };

            //! An interface for ThumbnailRendererSteps to communicate with thumbnail renderer
            class ThumbnailRendererContext
            {
            public:
                virtual void SetStep(Step step) = 0;
                virtual Step GetStep() const = 0;
                virtual AZStd::shared_ptr<ThumbnailRendererData> GetData() const = 0;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
