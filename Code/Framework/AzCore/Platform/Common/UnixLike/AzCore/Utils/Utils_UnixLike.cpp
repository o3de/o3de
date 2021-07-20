/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>

#include <cstdlib>

namespace AZ
{
    namespace Utils
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
            return {};
        }

        AZStd::optional<AZ::IO::FixedMaxPathString> ConvertToAbsolutePath(AZStd::string_view path)
        {
            AZ::IO::FixedMaxPathString absolutePath;
            AZ::IO::FixedMaxPathString srcPath{ path };

            if (char* result = realpath(srcPath.c_str(), absolutePath.data()); result)
            {
                // Fix the size value of the fixed string by calculating the c-string length using char traits
                absolutePath.resize_no_construct(AZStd::char_traits<char>::length(absolutePath.data()));
                return absolutePath;
            }

            return AZStd::nullopt;
        }
    } // namespace Utils
} // namespace AZ
