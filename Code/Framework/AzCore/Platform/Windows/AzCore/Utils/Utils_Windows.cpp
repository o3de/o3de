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

    AZ::IO::FixedMaxPath overrideHomeDirectory;

    void OverrideHomeDirectory(const AZ::IO::PathView& path)
    {
        overrideHomeDirectory = path;
    }

    AZ::IO::FixedMaxPathString GetHomeDirectory()
    {
        if (!overrideHomeDirectory.empty())
        {
            return overrideHomeDirectory.Native();
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

    AZ::IO::FixedMaxPathString GetO3deManifestDirectory()
    {
        AZ::IO::FixedMaxPath path = GetHomeDirectory();
        path /= ".o3de";
        return path.Native();
    }
} // namespace AZ::Utils
