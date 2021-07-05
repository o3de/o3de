/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <stdlib.h>

#include <AzTest/Platform.h>
namespace AZ
{
    namespace Test
    {
        ScopedAutoTempDirectory::ScopedAutoTempDirectory()
        {
            auto tempDirRoot = ::testing::TempDir();
            char tempDirectoryTemplate[AZ::IO::MaxPathLength] = { '\0' };
            azsnprintf(tempDirectoryTemplate, AZ::IO::MaxPathLength, "%sUnitTest-XXXXXX", tempDirRoot.c_str());

            const char* tempDir = mkdtemp(tempDirectoryTemplate);    
            AZ_Error("AzTest", tempDir, "Unable to create temp directory %s", tempDirectoryTemplate);
            memset(m_tempDirectory, '\0', sizeof(m_tempDirectory));
            if (tempDir)
            {
                azstrncpy(m_tempDirectory, AZ::IO::MaxPathLength, tempDirectoryTemplate, strlen(tempDirectoryTemplate) + 1);
            }
        }
    } // Test
} // AZ
