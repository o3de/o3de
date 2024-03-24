/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/conversions.h>

#include <stdlib.h>
#include <shlobj_core.h>

namespace AZ::Utils
{
    void NativeErrorMessageBox(const char* title, const char* message)
    {
        AZStd::wstring wtitle;
        AZStd::to_wstring(wtitle, title);
        AZStd::wstring wmessage;
        AZStd::to_wstring(wmessage, message);
        ::MessageBoxW(0, wmessage.c_str(), wtitle.c_str(), MB_OK | MB_ICONERROR);
    }

    AZ::IO::FixedMaxPathString GetHomeDirectory(AZ::SettingsRegistryInterface* settingsRegistry)
    {
        constexpr AZStd::string_view overrideHomeDirKey = "/Amazon/Settings/override_home_dir";
        AZ::IO::FixedMaxPathString overrideHomeDir;
        if (settingsRegistry == nullptr)
        {
            settingsRegistry = AZ::SettingsRegistry::Get();
        }

        if (settingsRegistry != nullptr)
        {
            if (settingsRegistry->Get(overrideHomeDir, overrideHomeDirKey))
            {
                AZ::IO::FixedMaxPath path{overrideHomeDir};
                return path.Native();
            }
        }

        wchar_t sysUserProfilePathW[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(0, CSIDL_PROFILE, 0, SHGFP_TYPE_DEFAULT, sysUserProfilePathW)))
        {
            AZ::IO::FixedMaxPathString sysUserProfilePathStr;
            AZStd::to_string(sysUserProfilePathStr, sysUserProfilePathW);
            return sysUserProfilePathStr;
        }

        char userProfileBuffer[AZ::IO::MaxPathLength]{};
        size_t variableSize = 0;
        auto err = getenv_s(&variableSize, userProfileBuffer, AZ::IO::MaxPathLength, "USERPROFILE");
        if (!err)
        {
            AZ::IO::FixedMaxPath path{ userProfileBuffer };
            return path.Native();
        }

        return {};
    }
} // namespace AZ::Utils
