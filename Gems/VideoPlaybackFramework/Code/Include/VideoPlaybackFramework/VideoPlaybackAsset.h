/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>

namespace VideoPlaybackFramework
{
    class VideoPlaybackAsset
    {
    public:
        AZ_TYPE_INFO(VideoPlaybackAsset, "{DDFEE0B2-9E5A-412C-8C77-AB100E24C1DF}");
        static const char* GetFileFilter()
        {
            return "*.mp4;*.mkv;*.webm;*.mov";
        }
    };
} // namespace VideoPlaybackFramework
