/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        ProjectJson, //!< Data comes from global project.json file
        PlatformResource, //!< Data comes from another file, which can have different formats (json or plist)

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
        Platform{ PlatformId::Android, PlatformDataType::PlatformResource },
        Platform{ PlatformId::Ios, PlatformDataType::PlatformResource }
    };
    
} // namespace ProjectSettingsTool
