/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/conversions.h>

#include <stdlib.h>

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

    AZ::IO::FixedMaxPathString GetHomeDirectory()
    {
        constexpr AZStd::string_view overrideHomeDirKey = "/Amazon/Settings/override_home_dir";
        AZ::IO::FixedMaxPathString overrideHomeDir;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
        {
            if (settingsRegistry->Get(overrideHomeDir, overrideHomeDirKey))
            {
                AZ::IO::FixedMaxPath path{overrideHomeDir};
                return path.Native();
            }
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

    bool SetEnv(const char* envname, const char* envvalue, bool overwrite)
    {
        AZ_UNUSED(overwrite);

        return _putenv_s(envname, envvalue);
    }

    bool UnSetEnv(const char* envname)
    {
        return SetEnv(envname, "", 1);
    }
} // namespace AZ::Utils
