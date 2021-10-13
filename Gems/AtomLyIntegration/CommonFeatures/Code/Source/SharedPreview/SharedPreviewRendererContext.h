/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SharedPreview/SharedPreviewRendererState.h>

namespace AZ
{
    namespace LyIntegration
    {
        namespace Thumbnails
        {
            struct SharedPreviewRendererData;

            enum class State
            {
                None,
                Init,
                Idle,
                Load,
                Capture,
                Release
            };

            //! An interface for SharedPreviewRendererStates to communicate with thumbnail renderer
            class SharedPreviewRendererContext
            {
            public:
                virtual void SetState(State state) = 0;
                virtual State GetState() const = 0;
                virtual AZStd::shared_ptr<SharedPreviewRendererData> GetData() const = 0;
            };
        } // namespace Thumbnails
    } // namespace LyIntegration
} // namespace AZ
