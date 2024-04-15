/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <filesystem>
#include <shlobj.h>
#include <AzCore/base.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>

namespace AZ::RHI::Internal
{
    AZ::IO::FixedMaxPathString GetLatestWinPixGpuCapturerPath()
    {
        LPWSTR programFilesPath = nullptr;
        SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_DEFAULT, NULL, &programFilesPath);

        std::filesystem::path pixInstallationPath = programFilesPath;
        pixInstallationPath /= "Microsoft PIX";

        std::wstring newestVersionFound;

        for (auto const& directory_entry : std::filesystem::directory_iterator(pixInstallationPath))
        {
            if (directory_entry.is_directory())
            {
                if (newestVersionFound.empty() || newestVersionFound < directory_entry.path().filename().c_str())
                {
                    newestVersionFound = directory_entry.path().filename().c_str();
                }
            }
        }

        if (newestVersionFound.empty())
        {
            return "";
        }

        std::wstring finalPathW = pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
        AZ::IO::FixedMaxPathString finalPath;
        AZStd::to_string(finalPath, finalPathW.c_str());
        return finalPath;
    }
} 
