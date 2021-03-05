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

namespace ProjectSettingsTool
{
    enum class PlatformId
    {
        Base,
        Android,
        Ios,

        NumPlatformIds
    };

    enum class PlatformDataType
    {
        ProjectJson,
        Plist,

        NumPlatformDataTypes
    };

    struct Platform
    {
        PlatformId m_id;
        PlatformDataType m_type;
    };

    const Platform Platforms[static_cast<unsigned>(PlatformId::NumPlatformIds)]
    {
        Platform{ PlatformId::Base, PlatformDataType::ProjectJson },
        Platform{ PlatformId::Android, PlatformDataType::ProjectJson },
        Platform{ PlatformId::Ios, PlatformDataType::Plist }
    };
    
} // namespace ProjectSettingsTool