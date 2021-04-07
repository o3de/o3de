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
            //! FindThumbnailToRenderStep checks whether there are any new thumbnails that need to be rendered every tick
            class FindThumbnailToRenderStep
                : public ThumbnailRendererStep
                , private TickBus::Handler
            {
            public:
                FindThumbnailToRenderStep(ThumbnailRendererContext* context);

                void Start() override;
                void Stop() override;

            private:

                //! AZ::TickBus::Handler interface overrides...
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

                void PickNextThumbnail();
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

