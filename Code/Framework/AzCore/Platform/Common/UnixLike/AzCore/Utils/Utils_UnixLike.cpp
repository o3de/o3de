/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>

#include <cstdlib>
#include <pwd.h>

namespace AZ::Utils
{
    void RequestAbnormalTermination()
    {
        abort();
    }

    void NativeErrorMessageBox(const char*, const char*) {}

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

        if (const char* homePath = std::getenv("HOME"); homePath != nullptr)
        {
            AZ::IO::FixedMaxPath path{homePath};
            return path.Native();
        }

        struct passwd* pass = getpwuid(getuid());
        if (pass)
        {
            AZ::IO::FixedMaxPath path{pass->pw_dir};
            return path.Native();
        }

        return {};
    }

    bool ConvertToAbsolutePath(const char* path, char* absolutePath, AZ::u64 maxLength)
    {
#ifdef PATH_MAX
        static constexpr size_t UnixMaxPathLength = PATH_MAX;
#else
        // Fallback to 4096 if the PATH_MAX macro isn't defined on the Unix System
        static constexpr size_t UnixMaxPathLength = 4096;
#endif
        if (!AZ::IO::PathView(path).IsAbsolute())
        {
            // note that realpath fails if the path does not exist and actually changes the return value
            // to be the actual place that FAILED, which we don't want.
            // if we fail, we'd prefer to fall through and at least use the original path.
            char absolutePathBuffer[UnixMaxPathLength];
            if (const char* result = realpath(path, absolutePathBuffer); result != nullptr)
            {
                azstrcpy(absolutePath, maxLength, absolutePathBuffer);
                return true;
            }
        }
        azstrcpy(absolutePath, maxLength, path);
        return AZ::IO::PathView(absolutePath).IsAbsolute();
    }
} // namespace AZ::Utils
