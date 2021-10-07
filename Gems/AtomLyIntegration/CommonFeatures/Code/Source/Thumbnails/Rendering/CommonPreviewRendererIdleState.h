/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Thumbnails/Rendering/CommonPreviewRendererState.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            //! CommonPreviewRendererIdleState checks whether there are any new thumbnails that need to be rendered every tick
            class CommonPreviewRendererIdleState
                : public CommonPreviewRendererState
                , private TickBus::Handler
            {
            public:
                CommonPreviewRendererIdleState(CommonPreviewRenderer* renderer);

                void Start() override;
                void Stop() override;

            private:

                //! AZ::TickBus::Handler interface overrides...
                void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ

