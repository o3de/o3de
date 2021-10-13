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
        class SharedPreviewRendererReleaseState : public SharedPreviewRendererState
        {
        public:
            SharedPreviewRendererReleaseState(SharedPreviewRendererContext* context);

            void Start() override;
            void Stop() override;
        };
    } // namespace LyIntegration
} // namespace AZ
