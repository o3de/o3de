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
