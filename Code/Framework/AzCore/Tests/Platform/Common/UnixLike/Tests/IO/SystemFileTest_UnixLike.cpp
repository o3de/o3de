/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/Utils.h>

namespace UnitTest
{
    class SystemFilePlatformFixture
        : public LeakDetectionFixture
    {
    protected:
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;
    };

    TEST_F(SystemFilePlatformFixture, SkipCloseOnDestructionMode_DoesNotCloseOpenHandle_Succeeds)
    {
        constexpr AZStd::string_view testData{ "Testing 1 2 3\n"};
        auto testFileName = m_tempDirectory.GetDirectoryAsFixedMaxPath() / "TestFile.txt";
        {
            // Seed the test file with data
            AZ::IO::SystemFile testFile(testFileName.c_str(),
                AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
            ASSERT_TRUE(testFile.IsOpen());

            testFile.Write(testData.data(), testData.size());
        }

        AZ::IO::SystemFile::FileHandleType fileHandle;
        {
            // Open the test file in read mode and retrieve the file handle
            // The skip close on destruction option should skip closing the file
            AZ::IO::SystemFile testFile(testFileName.c_str(),
                AZ::IO::SystemFile::SF_OPEN_READ_ONLY | AZ::IO::SystemFile::SF_SKIP_CLOSE_ON_DESTRUCTION);
            ASSERT_TRUE(testFile.IsOpen());

            fileHandle = testFile.NativeHandle();
        }

        // Use the PosixInternal::Read function to read from from the open descriptor
        AZStd::string readData;
        auto ReadFromFile = [fileHandle](char* buffer, size_t capacity) -> size_t
        {
            return AZ::IO::PosixInternal::Read(fileHandle, buffer, capacity);
        };
        // Make sure the capacity can fit all the data written to the test file
        readData.resize_and_overwrite(testData.size(), AZStd::move(ReadFromFile));

        // Close the file descriptor
        AZ::IO::PosixInternal::Close(fileHandle);

        EXPECT_EQ(testData, readData);
    }
}   // namespace UnitTest
