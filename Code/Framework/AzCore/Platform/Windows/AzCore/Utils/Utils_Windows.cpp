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

#include <AzCore/Utils/Utils.h>
#include <AzCore/PlatformIncl.h>

#include <stdlib.h>

namespace AZ::Utils
{
    void NativeErrorMessageBox(const char* title, const char* message)
    {
        ::MessageBox(0, message, title, MB_OK | MB_ICONERROR);
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
} // namespace AZ::Utils
