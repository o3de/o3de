/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Input/NativeCursorCapture.h>

namespace AzToolsFramework
{
    // This platform uses QtEventToAzInputMapper's cursor-warp fallback.
    AZStd::unique_ptr<NativeCursorCapture> NativeCursorCapture::Create([[maybe_unused]] MotionDeltaFn motionDeltaFn)
    {
        return nullptr;
    }
} // namespace AzToolsFramework
