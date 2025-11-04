/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/Utils.h>

namespace UnitTest
{
    class SystemFileStreamTest
        : public LeakDetectionFixture
    {
    protected:
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;
    };

    TEST_F(SystemFileStreamTest, SystemFileStream_CanConstructFromSystemFile_Succeeds)
    {
        AZ::IO::Path testFile = m_tempDirectory.Resolve("TestFile.txt");
        {
            constexpr AZ::IO::OpenMode writeOpenMode = AZ::IO::OpenMode::ModeWrite;
            const int systemFileMode = AZ::IO::TranslateOpenModeToSystemFileMode(testFile.c_str(), writeOpenMode);
            AZ::IO::SystemFile systemFile(testFile.c_str(), systemFileMode);

            EXPECT_TRUE(systemFile.IsOpen());
            EXPECT_EQ(testFile, systemFile.Name());
            {
                // Write test output to the test file using SystemFile variable
                constexpr AZStd::string_view testOutput = "hello";
                systemFile.Write(testOutput.data(), testOutput.size());
            }

            // The SystemFile should be moved into the SystemFileStream
            AZ::IO::SystemFileStream systemFileStream(AZStd::move(systemFile));
            // The SystemFile instance should NOT refer to a file
            EXPECT_FALSE(systemFile.IsOpen());
            EXPECT_NE(testFile, systemFile.Name());
            // The SystemFileStream instance should refer to an open file
            EXPECT_TRUE(systemFileStream.IsOpen());
            EXPECT_EQ(testFile, systemFileStream.GetFilename());

            {
                // Write test output to the test file using SystemFileStream variable,
                // this should write to the same underlying file
                constexpr AZStd::string_view testOutput = " world";
                systemFileStream.Write(testOutput.size(), testOutput.data());
            }

            // Move the SystemFile instance in the stream back to the local systemFileVariable
            systemFile = AZStd::move(systemFileStream).MoveSystemFile();

            // Now the SystemFileStream instance should no longer refer to a file
            EXPECT_FALSE(systemFileStream.IsOpen());
            EXPECT_NE(testFile, systemFileStream.GetFilename());
            // Once again the SystmeFile instance should refer to the open file
            EXPECT_TRUE(systemFile.IsOpen());
            EXPECT_EQ(testFile, systemFile.Name());

            // Close the file for write. It will not be written to again in this test
            systemFile.Close();
        }

        // Finally validate that writing to the test file through both the SystemFile and SystemFileStream
        // outputs to the same underlying file
        // This will be done by reading the contents of the file
        {
            constexpr AZStd::string_view expectedOutput = "hello world";
            // Test the constructor which accepts a c-string path and a mode
            constexpr AZ::IO::OpenMode readOpenMode = AZ::IO::OpenMode::ModeRead;
            AZ::IO::SystemFileStream systemFileStream(testFile.c_str(), readOpenMode);
            ASSERT_TRUE(systemFileStream.IsOpen());
            ASSERT_EQ(expectedOutput.size(), systemFileStream.GetLength());
            // Resize the readString to fit the contents of the test file
            AZStd::string readString;
            auto readFile = [&systemFileStream](char* buffer, size_t size)
            {
                EXPECT_EQ(size, systemFileStream.Read(size, buffer));
                return size;
            };
            readString.resize_and_overwrite(systemFileStream.GetLength(), readFile);

            EXPECT_EQ(expectedOutput, readString);
        }
    }
    TEST_F(SystemFileStreamTest, SystemFileStream_CanReopenWhenSystemFileAndOpenModeConstructor_HasBeenUsed)
    {
        AZ::IO::Path testFile = m_tempDirectory.Resolve("EmptyFile.txt");

        // Create an file so that it exist on disk. This will be used to test the ReOpen functionality
        constexpr AZ::IO::OpenMode writeOpenMode = AZ::IO::OpenMode::ModeWrite;
        const int systemFileMode = AZ::IO::TranslateOpenModeToSystemFileMode(testFile.c_str(), writeOpenMode);
        [[maybe_unused]] AZ::IO::SystemFile systemFile(testFile.c_str(), systemFileMode);

        // Close the systemFileStream and re-open it to verify that ReOpen works when a mode is explicitly
        // provided to SystemFileStream constructor
        AZ::IO::SystemFileStream systemFileStream(AZStd::move(systemFile), writeOpenMode);
        EXPECT_TRUE(systemFileStream.IsOpen());
        systemFileStream.Close();
        EXPECT_FALSE(systemFileStream.IsOpen());
        EXPECT_TRUE(systemFileStream.ReOpen());
    }
}
