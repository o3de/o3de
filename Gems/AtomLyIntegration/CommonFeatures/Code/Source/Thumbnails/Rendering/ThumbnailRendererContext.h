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
