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
            //! SharedPreviewRendererIdleState checks whether there are any new thumbnails that need to be rendered every tick
            class SharedPreviewRendererIdleState
                : public SharedPreviewRendererState
                , private TickBus::Handler
            {
            public:
                SharedPreviewRendererIdleState(SharedPreviewRendererContext* context);

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

