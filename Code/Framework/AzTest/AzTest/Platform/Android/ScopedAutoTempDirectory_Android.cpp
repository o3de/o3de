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
#include <stdlib.h>

#include <AzTest/Platform.h>

namespace AZ
{
    namespace Test
    {
        ScopedAutoTempDirectory::ScopedAutoTempDirectory()
        {
            char tempDirectoryTemplate[] = "/sdcard/Android/data/com.lumberyard.tests/files/UnitTest-XXXXXX";
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
