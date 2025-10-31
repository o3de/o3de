/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Utils/Utils.h>

namespace UnitTest
{
    static constexpr int s_numTrialsToPerform = 20000; // as much we can do in about a second.  Increase for a deeper longer fuzz test
    /**
     * systemFile test
     */
    class SystemFileTest
        : public LeakDetectionFixture
    {
    protected:
        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;
    };

#if AZ_TRAIT_DISABLE_FAILED_SYSTEM_FILE_TESTS
    TEST_F(SystemFileTest, DISABLED_Test)
#else
    TEST_F(SystemFileTest, Can_CheckAttributesAndReadWriteFiles_Succeed)
#endif // AZ_TRAIT_DISABLE_FAILED_SYSTEM_FILE_TESTS
    {
        // Test Deleting a non-existent file
        AZ::IO::Path invalidFileName = m_tempDirectory.Resolve("InvalidFileName");
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(invalidFileName.c_str()));
        EXPECT_FALSE(AZ::IO::SystemFile::Delete(invalidFileName.c_str()));

        //Files that don't exist are not considered writable
        EXPECT_FALSE(AZ::IO::SystemFile::IsWritable(invalidFileName.c_str()));

        AZ::IO::Path testFile = m_tempDirectory.Resolve("SystemFileTest.txt");
        AZStd::string testString = "Hello";

        // If file exists start by deleting the file
        AZ::IO::SystemFile::Delete(testFile.c_str());
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(testFile.c_str()));

        // Test Writing a file
        {
            AZ::IO::SystemFile oFile;
            oFile.Open(testFile.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
            oFile.Write(testString.c_str(), testString.length() + 1);
            EXPECT_EQ(testString.length() + 1, oFile.Tell());
            EXPECT_TRUE(oFile.Eof());
            oFile.Flush();
            oFile.Close();
            EXPECT_TRUE(AZ::IO::SystemFile::Exists(testFile.c_str()));
        }
        // Test Reading from the file
        {
            AZ::IO::SystemFile iFile;
            iFile.Open(testFile.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
            EXPECT_TRUE(iFile.IsOpen());
            char* buffer = (char*)azmalloc(testString.length() + 1);
            AZ::IO::SystemFile::SizeType nRead = iFile.Read(testString.length() + 1, buffer);
            EXPECT_EQ(testString.length() + 1, nRead);
            EXPECT_EQ(0, strcmp(testString.c_str(), buffer));
            EXPECT_EQ(testString.length() + 1, iFile.Tell());
            EXPECT_TRUE(iFile.Eof());
            iFile.Close();
            EXPECT_FALSE(iFile.IsOpen());
            azfree(buffer);
        }

        //Test Appending to the file
        {
            AZ::IO::SystemFile oFile;
            oFile.Open(testFile.c_str(), AZ::IO::SystemFile::SF_OPEN_APPEND | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);
            AZ::IO::SystemFile::SizeType initialSize = oFile.Length();
            oFile.Write(testString.c_str(), testString.length() + 1);
            oFile.Write(testString.c_str(), testString.length() + 1);
            EXPECT_EQ(initialSize + (testString.length() + 1) * 2, oFile.Tell());
            EXPECT_TRUE(oFile.Eof());
            oFile.Flush();
            oFile.Close();
            EXPECT_TRUE(AZ::IO::SystemFile::Exists(testFile.c_str()));
        }

        // Test Reading from the file
        {
            AZ::IO::SystemFile iFile;
            iFile.Open(testFile.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
            EXPECT_TRUE(iFile.IsOpen());
            size_t len = testString.length() + 1;
            char* buffer = (char*)azmalloc(len * 3);
            AZ::IO::SystemFile::SizeType nRead = iFile.Read(len * 3, buffer);
            EXPECT_EQ(len * 3, nRead);
            EXPECT_EQ(0, strcmp(testString.c_str(), buffer));
            EXPECT_EQ(0, strcmp(testString.c_str(), buffer + len));
            EXPECT_EQ(0, strcmp(testString.c_str(), buffer + (len * 2)));
            EXPECT_EQ(nRead, iFile.Tell());
            EXPECT_TRUE(iFile.Eof());
            iFile.Close();
            EXPECT_FALSE(iFile.IsOpen());
            azfree(buffer);
        }


        //File should exist now, and be writable
        EXPECT_TRUE(AZ::IO::SystemFile::IsWritable(testFile.c_str()));
#if AZ_TRAIT_OS_CAN_SET_FILES_WRITABLE
        AZ::IO::SystemFile::SetWritable(testFile.c_str(), false);
        EXPECT_FALSE(AZ::IO::SystemFile::IsWritable(testFile.c_str()));
        AZ::IO::SystemFile::SetWritable(testFile.c_str(), true);
        EXPECT_TRUE(AZ::IO::SystemFile::IsWritable(testFile.c_str()));
#endif

        // Now that the file exists, verify that we can delete it
        bool deleted = AZ::IO::SystemFile::Delete(testFile.c_str());
        EXPECT_TRUE(deleted);
        EXPECT_FALSE(AZ::IO::SystemFile::Exists(testFile.c_str()));
    }

    TEST_F(SystemFileTest, Open_NullFileNames_DoesNotCrash)
    {
        AZ::IO::SystemFile oFile;
        EXPECT_FALSE(oFile.Open(nullptr, AZ::IO::SystemFile::SF_OPEN_READ_ONLY));
    }

    TEST_F(SystemFileTest, Open_EmptyFileNames_DoesNotCrash)
    {
        AZ::IO::SystemFile oFile;
        EXPECT_FALSE(oFile.Open("", AZ::IO::SystemFile::SF_OPEN_READ_ONLY));
    }

    TEST_F(SystemFileTest, Open_BadFileNames_DoesNotCrash)
    {
        AZStd::string randomJunkName;

        randomJunkName.resize(128, '\0');

        for (int trialNumber = 0; trialNumber < s_numTrialsToPerform; ++trialNumber)
        {
            for (int randomChar = 0; randomChar < randomJunkName.size(); ++randomChar)
            {
                // note that this is intentionally allowing null characters to generate.
                // note that this also puts characters AFTER the null, if a null appears in the mddle.
                // so that if there are off by one errors they could include cruft afterwards.

                if (randomChar > trialNumber % randomJunkName.size())
                {
                    // choose this point for the nulls to begin.  It makes sure we test every size of string.
                    randomJunkName[randomChar] = 0;
                }
                else
                {
                    randomJunkName[randomChar] = (char)(rand() % 256); // this will trigger invalid UTF8 decoding too
                }
            }
            AZ::IO::SystemFile oFile;
            oFile.Open(randomJunkName.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
        }
    }

#if AZ_TRAIT_DISABLE_FAILED_FILE_DESCRIPTOR_REDIRECTOR_TESTS
    TEST_F(SystemFileTest, DISABLED_FileDescriptorRedirector_MainFunctionality)
#else
    TEST_F(SystemFileTest, FileDescriptorRedirector_MainFunctionality)
#endif // AZ_TRAIT_DISABLE_FAILED_FILE_DESCRIPTOR_REDIRECTOR_TESTS
    {
        AZ::Test::ScopedAutoTempDirectory tempDir;

        auto srcFile = tempDir.Resolve("SystemFileTest_Source.txt");
        auto redirectFile = tempDir.Resolve("SystemFileTest_Redirected.txt");
        {
            AZ::IO::SystemFile oFile;
            oFile.Open(srcFile.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE);
            oFile.Close();
            oFile.Open(redirectFile.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE);
            oFile.Close();
        }

        auto stringInFile = [](const char* filename, const char* findStr)
        {
            char content[128];
            size_t bytes = AZ::IO::SystemFile::Read(filename, content, sizeof(content));
            return bytes > 0 ? strstr(content, findStr) != nullptr : false;
        };

        const char contentInSource[] = "SOURCE";
        const char contentInRedirect[] = "REDIRECT";

        using namespace AZ::IO;
        // Before redirection, source must be written like normal, nothing should be in the redirection
        {
            int sourceFd = PosixInternal::Open(srcFile.c_str(),
                PosixInternal::OpenFlags::Create | PosixInternal::OpenFlags::WriteOnly | PosixInternal::OpenFlags::Truncate,
                PosixInternal::PermissionModeFlags::Read | PosixInternal::PermissionModeFlags::Write);

            FileDescriptorRedirector redirector(sourceFd);
            PosixInternal::Write(sourceFd, contentInSource, sizeof(contentInSource));
            PosixInternal::Close(sourceFd);
        }
        EXPECT_TRUE(stringInFile(srcFile.c_str(), contentInSource));
        EXPECT_FALSE(stringInFile(redirectFile.c_str(), contentInSource));

        // After redirection, redirection must be written
        {
            int sourceFd = PosixInternal::Open(srcFile.c_str(),
                PosixInternal::OpenFlags::Create | PosixInternal::OpenFlags::WriteOnly | PosixInternal::OpenFlags::Truncate,
                PosixInternal::PermissionModeFlags::Read | PosixInternal::PermissionModeFlags::Write);

            FileDescriptorRedirector redirector(sourceFd);
            redirector.RedirectTo(redirectFile.Native(), FileDescriptorRedirector::Mode::Create);
            PosixInternal::Write(sourceFd, contentInRedirect, sizeof(contentInRedirect));
            PosixInternal::Close(sourceFd);
        }
        EXPECT_FALSE(stringInFile(srcFile.c_str(), contentInRedirect));
        EXPECT_TRUE(stringInFile(redirectFile.c_str(), contentInRedirect));

        // After removing redirection, source must be written
        {
            int sourceFd = AZ::IO::PosixInternal::Open(srcFile.c_str(),
                PosixInternal::OpenFlags::Create | PosixInternal::OpenFlags::WriteOnly | PosixInternal::OpenFlags::Truncate,
                PosixInternal::PermissionModeFlags::Read | PosixInternal::PermissionModeFlags::Write);

            FileDescriptorRedirector redirector(sourceFd);
            redirector.RedirectTo(redirectFile.Native(), FileDescriptorRedirector::Mode::Create);
            redirector.Reset();
            PosixInternal::Write(sourceFd, contentInSource, sizeof(contentInSource));
            PosixInternal::Close(sourceFd);
        }
        EXPECT_TRUE(stringInFile(srcFile.c_str(), contentInSource));
        EXPECT_FALSE(stringInFile(redirectFile.c_str(), contentInSource));

        // Check that bypassing output even after being redirected, writes to the original file
        {
            int sourceFd = AZ::IO::PosixInternal::Open(srcFile.c_str(),
                PosixInternal::OpenFlags::Create | PosixInternal::OpenFlags::WriteOnly | PosixInternal::OpenFlags::Truncate,
                PosixInternal::PermissionModeFlags::Read | PosixInternal::PermissionModeFlags::Write);

            FileDescriptorRedirector redirector(sourceFd);
            redirector.RedirectTo(redirectFile.Native(), FileDescriptorRedirector::Mode::Create);
            redirector.WriteBypassingRedirect(contentInSource, sizeof(contentInSource));
            PosixInternal::Close(sourceFd);
        }
        EXPECT_TRUE(stringInFile(srcFile.c_str(), contentInSource));
        EXPECT_FALSE(stringInFile(redirectFile.c_str(), contentInSource));

        AZ::IO::SystemFile::Delete(srcFile.c_str());
        AZ::IO::SystemFile::Delete(redirectFile.c_str());
    }

    TEST_F(SystemFileTest, FileDescriptorCapturer_DoesNotDeadlock_WhenMoreThanPipeSizeContent_IsCaptured)
    {
        using namespace AZ::IO;

        AZStd::string capturedData;
        auto StoreFromFile = [&capturedData](AZStd::span<const AZStd::byte> capturedBytes)
        {
            AZStd::string_view capturedStrView(reinterpret_cast<const char*>(capturedBytes.data()), capturedBytes.size());
            capturedData += capturedStrView;
        };

        // The +1 to make sure the value isn't a power of 2, to try to catch any edge cases
        // with reading from a buffered pipe
        constexpr size_t charCountToWrite = AZ::IO::FileDescriptorCapturer::DefaultPipeSize * 2 + 1;
        capturedData.reserve(charCountToWrite);

        // while it may be tempting to use stdout here, other processes and threads might be running
        // in the same test run and also outputting to stdout.  This would cause an intermittent failure.
        // Instead, write to a temp file.
        AZ::Test::ScopedAutoTempDirectory tempDir;
        auto srcFile = tempDir.Resolve("SystemFileTest_Source.txt");
        int sourceFd = PosixInternal::Open(srcFile.c_str(),
            PosixInternal::OpenFlags::Create | PosixInternal::OpenFlags::ReadWrite | PosixInternal::OpenFlags::Truncate,
            PosixInternal::PermissionModeFlags::Read | PosixInternal::PermissionModeFlags::Write);
        ASSERT_NE(sourceFd, -1);
        AZ::IO::FileDescriptorCapturer capturer(sourceFd);

        capturer.Start(StoreFromFile);
        const char* dataToWrite = "a";
        for (size_t i = 0; i < charCountToWrite; ++i)
        {
            // this should cause the write function to fill up any buffer and then block if the pipe is full
            // until the capturer reads from it.
            // filling the pipe should NOT cause blocking here, since the capturer reads on a different thread.
            PosixInternal::Write(sourceFd, dataToWrite, 1);
        }
        PosixInternal::Close(sourceFd);
        capturer.Stop();

        const AZStd::string expectedValue(charCountToWrite, 'a');

        // if the lengths are not equal, we DROPPED or invented data.
        EXPECT_EQ(expectedValue.size(), capturedData.size()) << "FileDescriptorCapturer dropped or invented some of the data.";
        
        if (expectedValue.size() == capturedData.size())
        {
            // if the lengths are equal, but the strings are different, then the data is corrupt in some way (full of the
            // wrong character, or nulls, or something).
            EXPECT_TRUE(expectedValue == capturedData) << "FileDescriptorCapturer corrupted some of the data";
        }
    }

    TEST_F(SystemFileTest, GetStdout_ReturnsHandle_ThatCanWriteToStdout_Succeeds)
    {
        AZStd::string stdoutData;
        auto StoreStdout = [&stdoutData](AZStd::span<const AZStd::byte> capturedBytes)
        {
            AZStd::string_view capturedStrView(reinterpret_cast<const char*>(capturedBytes.data()), capturedBytes.size());
            stdoutData += capturedStrView;
        };

        constexpr AZStd::string_view testData{ "Micro Macro Tera Zetta\n"};

        // Capture stdout using descriptor of 1
        constexpr int StdoutDescriptor = 1;
        AZ::IO::FileDescriptorCapturer capturer(StdoutDescriptor);
        capturer.Start(StoreStdout);
        {
            AZ::IO::SystemFile stdoutHandle = AZ::IO::SystemFile::GetStdout();
            stdoutHandle.Write(testData.data(), testData.size());
        }
        fflush(stdout);
        capturer.Stop();

        EXPECT_EQ(testData, stdoutData);
    }

    TEST_F(SystemFileTest, GetStderr_ReturnsHandle_ThatCanWriteToStderr_Succeeds)
    {
        AZStd::string stderrData;
        auto StoreStderr = [&stderrData](AZStd::span<const AZStd::byte> capturedBytes)
        {
            AZStd::string_view capturedStrView(reinterpret_cast<const char*>(capturedBytes.data()), capturedBytes.size());
            stderrData += capturedStrView;
        };

        constexpr AZStd::string_view testData{ "Micro Macro Tera Zetta\n"};

        // Capture stderr using descriptor of 2
        constexpr int StderrDescriptor = 2;
        AZ::IO::FileDescriptorCapturer capturer(StderrDescriptor);
        capturer.Start(StoreStderr);
        {
            AZ::IO::SystemFile stderrHandle = AZ::IO::SystemFile::GetStderr();
            stderrHandle.Write(testData.data(), testData.size());
        }
        fflush(stderr);
        capturer.Stop();

        EXPECT_EQ(testData, stderrData);
    }

    TEST_F(SystemFileTest, GetStdin_ReturnsHandle_ThatIsOpen_Succeeds)
    {
        char buffer[1];
        AZ::IO::SystemFile stdinHandle = AZ::IO::SystemFile::GetStdin();
        ASSERT_TRUE(stdinHandle.IsOpen());
        // Read 0 bytes to validate that the stdin handle is open
        // as well to avoid needing stdin to have data within it
        // This call should block.
        EXPECT_EQ(0, stdinHandle.Read(static_cast<AZ::IO::SizeType>(0), buffer));
    }

}   // namespace UnitTest
