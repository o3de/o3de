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

#include <stdint.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/Debug/Trace.h>

namespace AZ
{
    const char* GetPlatformName(PlatformID platform)
    {
        switch (platform)
        {
        case PlatformID::PLATFORM_WINDOWS_64:
            return "Win64";
        case PlatformID::PLATFORM_LINUX_64:
            return "Linux";
        case PlatformID::PLATFORM_ANDROID_64:
            return "Android64";
        case PlatformID::PLATFORM_APPLE_IOS:
            return "iOS";
        case PlatformID::PLATFORM_APPLE_MAC:
            return "Mac";
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        case PlatformID::PLATFORM_##PUBLICNAME:\
            return #PublicName;
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM)
            AZ_EXPAND_FOR_RESTRICTED_PLATFORM
#else
            AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
        default:
            AZ_Assert(false, "Platform %u is unknown.", static_cast<uint32_t>(platform));
            return "";
        }
    }

} // namespace AZ

#include <AzCore/PlatformId/PlatformId_Platform.h>
