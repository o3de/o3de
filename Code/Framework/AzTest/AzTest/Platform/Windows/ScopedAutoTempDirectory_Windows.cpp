/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>

#include <AzCore/std/string/conversions.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/functional.h>
#include <AzCore/IO/SystemFile.h>
#include <AzTest/Utils.h>

namespace AZ::Test
{
    ScopedAutoTempDirectory::ScopedAutoTempDirectory()
    {
        constexpr DWORD bufferSize = static_cast<DWORD>(AZ::IO::MaxPathLength);

        wchar_t tempDirW[AZ::IO::MaxPathLength]{};
        GetTempPathW(bufferSize, tempDirW);

        AZ::IO::FixedMaxPath tempDirectoryRoot;
        AZStd::to_string(tempDirectoryRoot.Native(), tempDirW);

        constexpr int MaxAttempts = 255;
        for (int i = 0; i < MaxAttempts; ++i)
        {
            AZ::IO::FixedMaxPath testPath = tempDirectoryRoot /
                AZ::IO::FixedMaxPathString::format("UnitTest-%s",
                    AZ::Uuid::CreateRandom().ToFixedString().c_str());
            // Try to create the temp directory if it doesn't exist
            if (!AZ::IO::SystemFile::Exists(testPath.c_str()) && AZ::IO::SystemFile::CreateDir(testPath.c_str()))
            {
                azstrncpy(AZStd::data(m_tempDirectory), AZStd::size(m_tempDirectory),
                    testPath.c_str(), testPath.Native().size());
                break;
            }
        }

        AZ_Error("AzTest", m_tempDirectory[0] != '\0', "Unable to create temp path within directory %s after %d attempts",
            tempDirectoryRoot.c_str(), MaxAttempts);
    }
} // AZ::Test
