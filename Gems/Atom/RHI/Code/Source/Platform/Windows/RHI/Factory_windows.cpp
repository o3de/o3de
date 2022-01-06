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
#include <AzCore/std/string/string.h>

namespace AZ::RHI::Platform
{
    bool IsPixDllInjected(const char* dllName)
    {
        bool isDllLoaded = false;

        wchar_t fileNameW[256];
        size_t numCharsConverted;
        errno_t wcharResult = mbstowcs_s(&numCharsConverted, fileNameW, dllName, AZ_ARRAY_SIZE(fileNameW) - 1);
        if (wcharResult == 0)
        {
            isDllLoaded = NULL != GetModuleHandleW(fileNameW);
        }
        return isDllLoaded;
    }

    AZStd::wstring GetLatestWinPixGpuCapturerPath()
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
            return L"";
        }

        std::wstring finalPath = pixInstallationPath / newestVersionFound / L"WinPixGpuCapturer.dll";
        return AZStd::wstring(finalPath.c_str());
    }
} 
