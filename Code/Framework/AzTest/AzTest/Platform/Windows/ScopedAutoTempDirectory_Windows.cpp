/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <stdlib.h>
#include <windows.h>
#include <sysinfoapi.h>
#include <fileapi.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/functional.h>
#include <AzCore/IO/SystemFile.h>
#include <AzTest/Platform.h>

namespace AZ
{
    namespace Test
    {
        ScopedAutoTempDirectory::ScopedAutoTempDirectory()
        {
            constexpr const DWORD bufferSize = static_cast<DWORD>(AZ::IO::MaxPathLength);

            char tempDir[bufferSize] = {0};
            GetTempPathA(bufferSize, tempDir);

            char workingTempPathBuffer[bufferSize] = {'\0'};

            int maxAttempts = 2000; // Prevent an infinite loop by setting an arbitrary maximum attempts at finding an available temp folder name
            while (maxAttempts > 0)
            {
                // Use the system's tick count to base the folder name
                DWORD currentTick = GetTickCount64();
                azsnprintf(workingTempPathBuffer, bufferSize, "%sUnitTest-%X", tempDir, aznumeric_cast<unsigned int>(currentTick));

                // Check if the requested directory name is available and re-generate if it already exists
                bool exists = AZ::IO::SystemFile::Exists(workingTempPathBuffer);
                if (exists)
                {
                    Sleep(1);
                    maxAttempts--;
                    continue;
                }
                break;
            }

            AZ_Error("AzTest", maxAttempts > 0, "Unable to determine a temp directory");

            if (maxAttempts > 0)
            {
                // Create the temp directory and track it for deletion
                bool tempDirectoryCreated = AZ::IO::SystemFile::CreateDir(workingTempPathBuffer);
                if (tempDirectoryCreated)
                {
                    azstrncpy(m_tempDirectory, AZ::IO::MaxPathLength, workingTempPathBuffer, AZ::IO::MaxPathLength);
                }
                else
                {
                    AZ_Error("AzTest", false, "Unable to create temp directory %s", workingTempPathBuffer);
                }
            }
        }
    } // Test
} // AZ
