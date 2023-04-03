/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Streamer/StorageDrive_Windows.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Utils/Utils.h>

#include <Tests/FileIOBaseTestTypes.h>
#include <Tests/Streamer/StreamStackEntryConformityTests.h>

namespace AZ::IO
{
    constexpr AZ::u32 TestMaxFileHandles = 1;
    constexpr AZ::u32 TestMaxMetaDataEntries = 16;
    constexpr size_t TestPhysicalSectorSize = 4_kib;
    constexpr size_t TestLogicalSectorSize = 512;
    constexpr AZ::u32 TestMaxIOChannels = 8;
    constexpr AZ::s32 TestOverCommit = 0;
    constexpr bool TestEnableUnbufferReads = true;
    constexpr bool TestEnableSharedReads = false;
    constexpr bool HasSeekPenalty = false;

    //
    // StreamStackEntry API Conformity
    //
    class StorageDriveWindowsTestDescription :
        public StreamStackEntryConformityTestsDescriptor<StorageDriveWin>
    {
    public:
        StorageDriveWin CreateInstance() override
        {
            StorageDriveWin::ConstructionOptions options;
            options.m_hasSeekPenalty = HasSeekPenalty;
            options.m_enableUnbufferedReads = TestEnableUnbufferReads;
            options.m_enableSharing = TestEnableSharedReads;
            options.m_minimalReporting = true;

            return StorageDriveWin({ "c:/" }, TestMaxFileHandles, TestMaxMetaDataEntries, TestPhysicalSectorSize,
                TestLogicalSectorSize, TestMaxIOChannels, TestOverCommit, options);
        }
    };

    INSTANTIATE_TYPED_TEST_CASE_P(
        Streamer_StorageDriveWindowsConformityTests, StreamStackEntryConformityTests, StorageDriveWindowsTestDescription);


    // Helper class to count the number of asserts / errors / warnings / printfs that have been triggered.
    class StreamerTraceBusDetector
        : public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        StreamerTraceBusDetector()
        {
            BusConnect();
        }

        ~StreamerTraceBusDetector() override
        {
            BusDisconnect();
        }

        bool OnAssert([[maybe_unused]] const char* message) override
        {
            m_assert++;
            return false;
        }

        bool OnError([[maybe_unused]] const char* window, [[maybe_unused]] const char* message) override
        {
            m_error++;
            return false;
        }

        bool OnWarning([[maybe_unused]] const char* window, [[maybe_unused]] const char* message) override
        {
            m_warning++;
            return false;
        }

        bool OnPrintf([[maybe_unused]] const char* window, [[maybe_unused]] const char* message) override
        {
            m_printf++;
            return false;
        }

        int m_assert{ 0 };
        int m_error{ 0 };
        int m_warning{ 0 };
        int m_printf{ 0 };
    };

    //
    // StorageDriveWin Tests
    //

    class Streamer_StorageDriveWindowsTestFixture
        : public UnitTest::LeakDetectionFixture
        , public UnitTest::SetRestoreFileIOBaseRAII
    {
    public:
        // Data...
        static constexpr char s_dummyFilename[] = "Dummy.bin";
        static constexpr char s_fileCharacter = 'F';
        static constexpr char s_beginCharacter = 'B';
        static constexpr char s_endCharacter = 'E';
        static constexpr char s_chunkCharacter = 'C';

        UnitTest::TestFileIOBase m_fileIO{};
        AZStd::string m_dummyFilepath;
        AZ::IO::RequestPath m_dummyRequestPath;
        AZStd::shared_ptr<StreamStackEntry> m_storageDriveWin{};
        AZ::IO::StreamerContext* m_context = nullptr;
        AZStd::vector<AZStd::string> m_dummyFiles;
        AZStd::vector<AZStd::unique_ptr<char[]>> m_dummyBuffers;
        StreamerTraceBusDetector m_traceDetector;
        StorageDriveWin::ConstructionOptions m_configurationOptions;

        // Methods...
        Streamer_StorageDriveWindowsTestFixture()
            : UnitTest::SetRestoreFileIOBaseRAII(m_fileIO)
        {
            PrepareTestFilepath();
        }

        void SetupStorageDrive(s32 overCommit)
        {
            if (m_context == nullptr)
            {
                m_context = new AZ::IO::StreamerContext();
            }

            ASSERT_FALSE(m_dummyFilepath.empty());

            // Get the drive letter of the dummy file...
            AZStd::string drive;
            AZ::StringFunc::Path::GetDrive(m_dummyFilepath.c_str(), drive);

            // Create a storage drive (windows) stack entry with sector size 16k to handle most cases.
            m_configurationOptions.m_hasSeekPenalty = HasSeekPenalty;
            m_configurationOptions.m_enableUnbufferedReads = TestEnableUnbufferReads;
            m_configurationOptions.m_enableSharing = TestEnableSharedReads;
            m_configurationOptions.m_minimalReporting = true;

            m_storageDriveWin = AZStd::make_shared<AZ::IO::StorageDriveWin>(AZStd::vector<AZStd::string_view>{drive}, TestMaxFileHandles,
                TestMaxMetaDataEntries, TestPhysicalSectorSize, TestLogicalSectorSize, TestMaxIOChannels, overCommit, m_configurationOptions);
            m_storageDriveWin->SetContext(*m_context);
        }

        void SetUp() override
        {
            m_dummyRequestPath = RequestPath(AZ::IO::PathView(m_dummyFilepath));

            SetupStorageDrive(TestOverCommit);
        }

        void TearDown() override
        {
            m_storageDriveWin.reset();
            delete m_context;
            m_context = nullptr;

            RemoveDummyFiles();
            m_dummyBuffers.clear();
            m_dummyBuffers.shrink_to_fit();
        }

        // Create a file filled with a single character.
        // If chunkOffset is non-zero, it will write in a specific character every chunkOffset bytes till the end of file.
        // If beginEndMarkers is true, it will write in specific bytes to mark the begin and end of the file.
        void CreateDummyFile(AZStd::string path, size_t fileSize, size_t chunkOffset = 0, bool beginEndMarkers = false)
        {
            using namespace AZ::IO;

            SystemFile file;
            bool fileCreated = file.Open(path.c_str(),
                SystemFile::OpenMode::SF_OPEN_CREATE | SystemFile::OpenMode::SF_OPEN_READ_WRITE);

            ASSERT_TRUE(fileCreated);

            m_dummyFiles.push_back(AZStd::move(path));

            char* buffer = new char[fileSize];
            ASSERT_NE(buffer, nullptr);

            ::memset(buffer, s_fileCharacter, fileSize);
            if (chunkOffset != 0)
            {
                for (size_t offset = 0; offset < fileSize; offset += chunkOffset)
                {
                    buffer[offset] = s_chunkCharacter;
                }
            }

            if (beginEndMarkers)
            {
                buffer[0] = s_beginCharacter;
                buffer[fileSize - 1] = s_endCharacter;
            }

            auto bytesWritten = file.Write(buffer, fileSize);
            file.Close();
            delete[] buffer;

            ASSERT_EQ(bytesWritten, fileSize);
        }

        void CreateDummyFile(size_t fileSize, size_t chunkOffset = 0, bool beginEndMarkers = false)
        {
            CreateDummyFile(m_dummyFilepath, fileSize, chunkOffset, beginEndMarkers);
        }

        void RemoveDummyFiles()
        {
            for (auto& dummyFile : m_dummyFiles)
            {
                AZ::IO::SystemFile::Delete(dummyFile.c_str());
            }
            m_dummyFiles.clear();
            m_dummyFiles.shrink_to_fit();
        }

        void WaitTillCompleted()
        {
            StreamStackEntry::Status status;
            auto startTime = AZStd::chrono::steady_clock::now();
            do
            {
                m_storageDriveWin->ExecuteRequests();
                m_context->FinalizeCompletedRequests();

                status.m_isIdle = true;
                m_storageDriveWin->UpdateStatus(status);

                if (AZStd::chrono::steady_clock::now() - startTime > AZStd::chrono::seconds(5))
                {
                    FAIL();
                }
            } while (!status.m_isIdle);
        }

        void DoSingleRead()
        {
            constexpr size_t fileSize = 16_kib;
            AZStd::unique_ptr<char[]> buffer(new char[fileSize]);

            CreateDummyFile(fileSize);

            AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
            request->CreateRead(nullptr, buffer.get(), fileSize, m_dummyRequestPath, 0, fileSize);
            m_storageDriveWin->QueueRequest(AZStd::move(request));

            m_dummyBuffers.push_back(AZStd::move(buffer));
        }

        void DoMetaDataRetrieval()
        {
            AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
            request->CreateFileMetaDataRetrieval(m_dummyRequestPath);
            m_storageDriveWin->QueueRequest(request);
        }

    private:
        void PrepareTestFilepath()
        {
            char exePath[AZ_MAX_PATH_LEN] = { 0 };
            auto result = AZ::Utils::GetExecutablePath(exePath, AZ_MAX_PATH_LEN);
            if (result.m_pathStored != AZ::Utils::ExecutablePathResult::Success)
            {
                return;
            }

            AZStd::string filePath(exePath);

            if (result.m_pathIncludesFilename)
            {
                AZ::StringFunc::Path::StripFullName(filePath);
            }

            AZ::StringFunc::Path::Join(filePath.c_str(), "TestFiles", filePath);

            // Create the "TestFiles" dir in the bin directory if it doesn't exist...
            if (!AZ::IO::SystemFile::Exists(filePath.c_str()))
            {
                if (!AZ::IO::SystemFile::CreateDir(filePath.c_str()))
                {
                    return;
                }
            }

            AZ::StringFunc::Path::Join(filePath.c_str(), s_dummyFilename, m_dummyFilepath);
        }
    };

    TEST_F(Streamer_StorageDriveWindowsTestFixture, SanityCheck)
    {
        // Just make sure the storage drive was set up...
        EXPECT_NE(m_storageDriveWin.get(), nullptr);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, Constructor_MultipleDrivePaths_AllPathsAreIncludedInTheName)
    {
        AZStd::vector<AZStd::string_view> drives;
        drives.push_back(R"(C:\)");
        drives.push_back(R"(D:\)");
        drives.push_back(R"(E:\)");
        m_storageDriveWin = AZStd::make_shared<AZ::IO::StorageDriveWin>(drives,
            TestMaxFileHandles, TestMaxMetaDataEntries, TestPhysicalSectorSize,
            TestLogicalSectorSize, TestMaxIOChannels, TestOverCommit, m_configurationOptions);

        const AZStd::string& name = m_storageDriveWin->GetName();
        EXPECT_GT(name.find("(C,"), 0);
        EXPECT_GT(name.find("D,"), 0);
        EXPECT_GT(name.find("E)"), 0);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, Constructor_InvalidSizes_ErrorsAreReported)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_storageDriveWin = AZStd::make_shared<AZ::IO::StorageDriveWin>(AZStd::vector<AZStd::string_view>{R"(C:\)"},
            TestMaxFileHandles, TestMaxMetaDataEntries, 0,
            0, TestMaxIOChannels, TestOverCommit, m_configurationOptions);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, Constructor_InvalidIoChannelCount_WarningIsReportedAndSizeAdjusted)
    {
        EXPECT_EQ(m_traceDetector.m_warning, 0);
        m_storageDriveWin = AZStd::make_shared<AZ::IO::StorageDriveWin>(AZStd::vector<AZStd::string_view>{R"(C:\)"},
            TestMaxFileHandles, TestMaxMetaDataEntries, TestPhysicalSectorSize,
            TestLogicalSectorSize, 0, TestOverCommit, m_configurationOptions);
        EXPECT_EQ(m_traceDetector.m_warning, 1);

        AZ::IO::StreamStackEntry::Status status{};
        m_storageDriveWin->UpdateStatus(status);
        EXPECT_GT(status.m_numAvailableSlots, 0);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, Constructor_InvalidOvercommit_ErrorIsReportedAndSizeAdjusted)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_storageDriveWin = AZStd::make_shared<AZ::IO::StorageDriveWin>(AZStd::vector<AZStd::string_view>{R"(C:\)"},
            TestMaxFileHandles, TestMaxMetaDataEntries, TestPhysicalSectorSize,
            TestLogicalSectorSize, TestMaxIOChannels, -(aznumeric_cast<s32>(TestMaxIOChannels) + 2), m_configurationOptions);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        AZ::IO::StreamStackEntry::Status status{};
        m_storageDriveWin->UpdateStatus(status);
        EXPECT_EQ(1, status.m_numAvailableSlots);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, GetAvailableRequestSlots_QueueAndExecuteRequests_RequestSlotCountAccurate)
    {
        AZ::IO::RequestPath path{ AZ::IO::PathView(m_dummyFilepath) };
        s32 currentRequestSlots = 0;
        constexpr int numRequests = 5;

        for (int i = 0; i < numRequests; ++i)
        {
            AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
            request->CreateFileExistsCheck(path);

            auto completionCallback = [this, &currentRequestSlots]([[maybe_unused]] const FileRequest& request)
            {
                AZ::IO::StreamStackEntry::Status status;
                m_storageDriveWin->UpdateStatus(status);
                EXPECT_EQ(status.m_numAvailableSlots, currentRequestSlots + 1);
                currentRequestSlots = status.m_numAvailableSlots;
            };

            request->SetCompletionCallback(completionCallback);

            AZ::IO::StreamStackEntry::Status statusBefore;
            m_storageDriveWin->UpdateStatus(statusBefore);

            m_storageDriveWin->QueueRequest(request);

            AZ::IO::StreamStackEntry::Status statusAfter;
            m_storageDriveWin->UpdateStatus(statusAfter);
            currentRequestSlots = statusAfter.m_numAvailableSlots;

            EXPECT_EQ(currentRequestSlots + 1, statusBefore.m_numAvailableSlots);
        }

        while (m_storageDriveWin->ExecuteRequests())
        {
            m_context->FinalizeCompletedRequests();
        }
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileMetaDataRetrievalRequest_InvalidPath_ReturnsFalse)
    {
        AZ::IO::RequestPath path("Invalid");

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileMetaDataRetrieval(path);
        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileMetaData = AZStd::get<Requests::FileMetaDataRetrievalData>(request.GetCommand());
                EXPECT_FALSE(fileMetaData.m_found);
                EXPECT_EQ(0, fileMetaData.m_fileSize);
            });

        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileMetaDataRetrievalRequest_FileExists_ReportsAccurateFileSize)
    {
        CreateDummyFile(4_kib);

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileMetaDataRetrieval(m_dummyRequestPath);

        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileMetaData = AZStd::get<Requests::FileMetaDataRetrievalData>(request.GetCommand());
                EXPECT_TRUE(fileMetaData.m_found);
                EXPECT_EQ(4_kib, fileMetaData.m_fileSize);
            });

        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileMetaDataRetrievalRequest_FileDoesntExist_ReturnsFalse)
    {
        AZ::IO::RequestPath path(AZ::IO::PathView(m_dummyFilepath + ".disappear"));

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileMetaDataRetrieval(path);
        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileMetaData = AZStd::get<Requests::FileMetaDataRetrievalData>(request.GetCommand());
                EXPECT_FALSE(fileMetaData.m_found);
                EXPECT_EQ(0, fileMetaData.m_fileSize);
            });

        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileMetaDataRetrievalRequest_UseStoredFileHandle_ReportsAccurateFileSize)
    {
        DoSingleRead();

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileMetaDataRetrieval(m_dummyRequestPath);

        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileMetaData = AZStd::get<Requests::FileMetaDataRetrievalData>(request.GetCommand());
                EXPECT_TRUE(fileMetaData.m_found);
                EXPECT_EQ(16_kib, fileMetaData.m_fileSize);
            });

        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileMetaDataRetrievalRequest_UseStoredMetaData_ReportsAccurateFileSize)
    {
        CreateDummyFile(4_kib);

        // Do the same request twice so it's in the cache.
        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileMetaDataRetrieval(m_dummyRequestPath);
        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();

        request = m_context->GetNewInternalRequest();
        request->CreateFileMetaDataRetrieval(m_dummyRequestPath);

        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileMetaData = AZStd::get<Requests::FileMetaDataRetrievalData>(request.GetCommand());
                EXPECT_TRUE(fileMetaData.m_found);
                EXPECT_EQ(4_kib, fileMetaData.m_fileSize);
            });

        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileExistsRequest_InvalidPath_ReturnsCompletedWithFileNotFound)
    {
        AZ::IO::RequestPath invalidPath("Invalid");

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileExistsCheck(invalidPath);
        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileExistsCheck = AZStd::get<Requests::FileExistsCheckData>(request.GetCommand());
                EXPECT_EQ(AZ::IO::IStreamerTypes::RequestStatus::Completed, request.GetStatus());
                EXPECT_FALSE(fileExistsCheck.m_found);
            });
        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileExistsRequest_FileDoesNotExist_ReturnsCompletedWithFileNotFound)
    {
        AZ::IO::RequestPath path(AZ::IO::PathView(m_dummyFilepath + ".disappear"));

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileExistsCheck(path);
        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileExistsCheck = AZStd::get<Requests::FileExistsCheckData>(request.GetCommand());
                EXPECT_EQ(AZ::IO::IStreamerTypes::RequestStatus::Completed, request.GetStatus());
                EXPECT_FALSE(fileExistsCheck.m_found);
            });
        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileExistsRequest_FileExists_ReturnsCompletedWithFileFound)
    {
        CreateDummyFile(4_kib);

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileExistsCheck(m_dummyRequestPath);
        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileExistsCheck = AZStd::get<Requests::FileExistsCheckData>(request.GetCommand());
                EXPECT_EQ(AZ::IO::IStreamerTypes::RequestStatus::Completed, request.GetStatus());
                EXPECT_TRUE(fileExistsCheck.m_found);
            });
        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileExistsRequest_UseStoredFileHandle_ReturnsCompletedWithFileFound)
    {
        DoSingleRead();

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileExistsCheck(m_dummyRequestPath);
        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileExistsCheck = AZStd::get<Requests::FileExistsCheckData>(request.GetCommand());
                EXPECT_EQ(AZ::IO::IStreamerTypes::RequestStatus::Completed, request.GetStatus());
                EXPECT_TRUE(fileExistsCheck.m_found);
            });
        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FileExistsRequest_UseStoredFileMetaData_ReturnsCompletedWithFileFound)
    {
        CreateDummyFile(4_kib);

        // Do the same request twice, so it's cached the second time.
        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        request->CreateFileExistsCheck(m_dummyRequestPath);
        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();

        request = m_context->GetNewInternalRequest();
        request->CreateFileExistsCheck(m_dummyRequestPath);
        request->SetCompletionCallback([](const FileRequest& request)
            {
                auto& fileExistsCheck = AZStd::get<Requests::FileExistsCheckData>(request.GetCommand());
                EXPECT_EQ(AZ::IO::IStreamerTypes::RequestStatus::Completed, request.GetStatus());
                EXPECT_TRUE(fileExistsCheck.m_found);
            });
        m_storageDriveWin->QueueRequest(request);
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, ReadDataRequest_QueueAndExecuteRequest_StorageDriveHandledRequest)
    {
        // Since StorageDriveWin is the only StreamerStack entry, we know that it's been used to handle
        // this Read request.
        constexpr size_t fileSize = 16_kib;
        // Create a buffer location for read data...
        AZStd::unique_ptr<char[]> buffer(new char[fileSize]);

        // Put begin and end markers in the file...
        CreateDummyFile(fileSize, 0, true);

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        AZ::IO::RequestPath path{ AZ::IO::PathView{ m_dummyFilepath } };

        request->CreateRead(nullptr, buffer.get(), fileSize, path, 0, fileSize);
        AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                  // capture. Newer versions issue unused warning
        auto callback = [&fileSize, this](const FileRequest& request)
        AZ_POP_DISABLE_WARNING
        {
            EXPECT_EQ(request.GetStatus(), AZ::IO::IStreamerTypes::RequestStatus::Completed);
            auto& readRequest = AZStd::get<AZ::IO::Requests::ReadData>(request.GetCommand());
            EXPECT_EQ(readRequest.m_size, fileSize);
            EXPECT_EQ(readRequest.m_path.GetAbsolutePath(), AZStd::string_view(m_dummyFilepath));
        };

        request->SetCompletionCallback(AZStd::move(callback));
        m_storageDriveWin->QueueRequest(AZStd::move(request));

        WaitTillCompleted();

        // Check the first and last characters in the buffer, make sure they are what we expect to have read from the file.
        EXPECT_EQ(buffer[0], s_beginCharacter);
        EXPECT_EQ(buffer[1], s_fileCharacter);
        EXPECT_EQ(buffer[fileSize - 2], s_fileCharacter);
        EXPECT_EQ(buffer[fileSize - 1], s_endCharacter);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, ReadDataRequest_UnalignedOffsetRead_ReturnsCorrectData)
    {
        constexpr AZ::u64 unalignedOffset = 40;     // read from unaligned offset 40
        constexpr AZ::u64 numChunksToRead = 7;      // read some # of 'offsets' worth of data
        constexpr AZ::u64 unalignedSize = unalignedOffset * numChunksToRead;
        constexpr size_t fileSize = 16_kib;         // full size of the file being created

        constexpr char unexpectedChar = 'Z';
        char* buffer = reinterpret_cast<char*>(azmalloc(unalignedSize + 4, TestPhysicalSectorSize)); // give the destination buffer a few extra bytes

        // Explicitly set the byte after the read size to be a predetermined value.
        // This will ensure that when the read completes it hasn't touched any bytes past the requested size.
        buffer[unalignedSize] = unexpectedChar;

        // Create the test file with regularly spaced markers, don't care about begin & end markers.
        CreateDummyFile(fileSize, unalignedOffset);

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        AZ::IO::RequestPath path{ AZ::IO::PathView{ m_dummyFilepath } };

        request->CreateRead(nullptr, buffer, unalignedSize + 4, path, unalignedOffset, unalignedSize);
        AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                  // capture. Newer versions issue unused warning
        auto callback = [unalignedOffset, unalignedSize, this](const FileRequest& request)
        AZ_POP_DISABLE_WARNING
        {
            EXPECT_EQ(request.GetStatus(), AZ::IO::IStreamerTypes::RequestStatus::Completed);
            auto& readRequest = AZStd::get<AZ::IO::Requests::ReadData>(request.GetCommand());
            EXPECT_EQ(readRequest.m_size, unalignedSize);
            EXPECT_EQ(readRequest.m_offset, unalignedOffset);
            EXPECT_EQ(readRequest.m_path.GetAbsolutePath(), AZStd::string_view(m_dummyFilepath));
        };

        request->SetCompletionCallback(AZStd::move(callback));
        m_storageDriveWin->QueueRequest(AZStd::move(request));

        WaitTillCompleted();

        EXPECT_EQ(buffer[0], s_chunkCharacter);
        for (size_t offset = 1; offset < numChunksToRead; ++offset)
        {
            EXPECT_EQ(buffer[(offset * unalignedOffset) - 1], s_fileCharacter);
            EXPECT_EQ(buffer[offset * unalignedOffset], s_chunkCharacter);
        }
        EXPECT_EQ(buffer[unalignedSize - 1], s_fileCharacter);

        // Check the byte that comes right after the requested data matches the unexpected char written before.
        EXPECT_EQ(buffer[unalignedSize], unexpectedChar);

        azfree(buffer);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, ReadDataRequest_UnalignedSizeRead_ReturnsCorrectDataAndDoesNotWriteMore)
    {
        constexpr AZ::u64 unalignedSize = 103630;
        // Don't give it too much extra size otherwise the extra space will be used over-read to the next alignment.
        constexpr size_t bufferSize = unalignedSize + 8;

        char* buffer = reinterpret_cast<char*>(azmalloc(bufferSize, TestPhysicalSectorSize));
        ::memset(buffer, 'Z', bufferSize);

        // Create the test file with regularly spaced markers, don't care about begin & end markers.
        CreateDummyFile(unalignedSize);

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        AZ::IO::RequestPath path{ AZ::IO::PathView{ m_dummyFilepath } };

        request->CreateRead(nullptr, buffer, bufferSize, path, 0, unalignedSize);
        auto callback = [](const FileRequest& request)
        {
            EXPECT_EQ(request.GetStatus(), AZ::IO::IStreamerTypes::RequestStatus::Completed);
        };

        request->SetCompletionCallback(AZStd::move(callback));
        m_storageDriveWin->QueueRequest(AZStd::move(request));

        WaitTillCompleted();

        for (size_t i = 0; i < unalignedSize; ++i)
        {
            ASSERT_EQ(s_fileCharacter, buffer[i]);
        }
        for (size_t i = unalignedSize; i < bufferSize; ++i)
        {
            ASSERT_EQ('Z', buffer[i]);
        }

        azfree(buffer);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, ReadDataRequest_UnalignedMemoryAllocation_ReturnsCorrectData)
    {
        constexpr AZ::u64 readSize = TestPhysicalSectorSize * 16;

        char* memory = reinterpret_cast<char*>(azmalloc(readSize + 16, TestPhysicalSectorSize));
        char* buffer = memory + 7;

        // Create the test file with regularly spaced markers, don't care about begin & end markers.
        CreateDummyFile(readSize);

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        AZ::IO::RequestPath path{ AZ::IO::PathView{ m_dummyFilepath } };

        request->CreateRead(nullptr, buffer, readSize + 16 - 7, path, 0, readSize);
        auto callback = [](const FileRequest& request)
        {
            EXPECT_EQ(request.GetStatus(), AZ::IO::IStreamerTypes::RequestStatus::Completed);
        };

        request->SetCompletionCallback(AZStd::move(callback));
        m_storageDriveWin->QueueRequest(AZStd::move(request));

        WaitTillCompleted();

        for (size_t i = 0; i < readSize; ++i)
        {
            ASSERT_EQ(s_fileCharacter, buffer[i]);
        }

        azfree(memory);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, ReadDataRequest_InvalidFilePath_ReportsFailure)
    {
        constexpr AZ::u64 readSize = TestPhysicalSectorSize;

        char buffer[readSize];

        auto mock = AZStd::make_shared<::testing::NiceMock<StreamStackEntryMock>>();
        m_storageDriveWin->SetNext(mock);

        AZ::IO::FileRequest* request = m_context->GetNewInternalRequest();
        AZ::IO::RequestPath path{ AZ::IO::PathView{ m_dummyFilepath + "/Broken/Path.txt" } };

        request->CreateRead(nullptr, buffer, readSize, path, 0, readSize);
        EXPECT_CALL(*mock, QueueRequest(request)).
            WillOnce([this](AZ::IO::FileRequest* request)
                {
                    m_context->MarkRequestAsCompleted(request);
                });

        m_storageDriveWin->QueueRequest(AZStd::move(request));
        WaitTillCompleted();
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, ReadDataRequest_ParallelReads_DataIsCorrect)
    {
        constexpr size_t chunkSize = TestPhysicalSectorSize;
        constexpr size_t numChunks = 5;
        static_assert(numChunks > 1, "Number of chunks for this test need to be 2 or more!");
        constexpr size_t fileSize = numChunks * chunkSize;
        AZStd::array<AZStd::unique_ptr<u8[]>, numChunks> buffers;
        AZStd::array<AZ::IO::FileRequest*, numChunks> requests;

        // Create a file with chunk markers and begin/end markers
        CreateDummyFile(fileSize, chunkSize, true);

        AZ::IO::RequestPath path{ AZ::IO::PathView{ m_dummyFilepath } };

        for (size_t i = 0; i < numChunks; ++i)
        {
            buffers[i].reset(new u8[chunkSize]);
            requests[i] = m_context->GetNewInternalRequest();

            requests[i]->CreateRead(nullptr, buffers[i].get(), chunkSize, path, i * chunkSize, chunkSize);
            AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                      // capture. Newer versions issue unused warning
            auto callback = [chunkSize, i](const FileRequest& request)
            AZ_POP_DISABLE_WARNING
            {
                EXPECT_EQ(request.GetStatus(), AZ::IO::IStreamerTypes::RequestStatus::Completed);
                auto& readRequest = AZStd::get<AZ::IO::Requests::ReadData>(request.GetCommand());
                EXPECT_EQ(readRequest.m_size, chunkSize);
                EXPECT_EQ(readRequest.m_offset, i * chunkSize);
            };

            requests[i]->SetCompletionCallback(AZStd::move(callback));

            m_storageDriveWin->QueueRequest(requests[i]);
        }

        WaitTillCompleted();

        // This is what the file looks like, assuming 4 chunks.
        // +-----+-----+-----+-----+
        // BFFFFFCFFFFFCFFFFFCFFFFFE
        // +-----+-----+-----+-----+

        // Check first & last bytes in first & last buffers/chunks.
        EXPECT_EQ(buffers[0][0], s_beginCharacter);
        EXPECT_EQ(buffers[0][chunkSize - 1], s_fileCharacter);
        EXPECT_EQ(buffers[numChunks - 1][0], s_chunkCharacter);
        EXPECT_EQ(buffers[numChunks - 1][chunkSize - 1], s_endCharacter);

        // Check first & last bytes in all interior buffers/chunks.
        for (size_t i = 1; i < numChunks - 1; ++i)
        {
            EXPECT_EQ(buffers[i][0], s_chunkCharacter);
            EXPECT_EQ(buffers[i][chunkSize - 1], s_fileCharacter);
        }
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, ReadDataRequest_NoMoreFileHandlesSlots_RequestIsDelayedAndThenCompleted)
    {
        size_t counter = 0;
        auto callback = [&counter](const FileRequest& request)
        {
            EXPECT_EQ(AZ::IO::IStreamerTypes::RequestStatus::Completed, request.GetStatus());
            counter++;
        };

        constexpr size_t fileSize = 16_kib;
        AZStd::unique_ptr<char[]> buffer0(new char[fileSize]);
        AZStd::unique_ptr<char[]> buffer1(new char[fileSize]);

        CreateDummyFile("dummyFile0.bin", fileSize);
        CreateDummyFile("dummyFile1.bin", fileSize);

        AZ::IO::RequestPath path0("dummyFile0.bin");
        AZ::IO::FileRequest* request0 = m_context->GetNewInternalRequest();
        request0->CreateRead(nullptr, buffer0.get(), fileSize, path0, 0, fileSize);
        request0->SetCompletionCallback(callback);

        AZ::IO::RequestPath path1("dummyFile1.bin");
        AZ::IO::FileRequest* request1 = m_context->GetNewInternalRequest();
        request1->CreateRead(nullptr, buffer1.get(), fileSize, path1, 0, fileSize);
        request1->SetCompletionCallback(callback);

        m_storageDriveWin->QueueRequest(AZStd::move(request0));
        m_storageDriveWin->QueueRequest(AZStd::move(request1));

        WaitTillCompleted();

        EXPECT_EQ(2, counter);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FlushCacheRequest_FlushPreviouslyReadFileAndMetaData_NoErrorsReported)
    {
        DoSingleRead();
        DoMetaDataRetrieval();
        // Wait here because normally the scheduler will only queue a flush when the stack is idle.
        WaitTillCompleted();

        AZ_TEST_START_TRACE_SUPPRESSION;
        AZ::IO::FileRequest* flushRequest = m_context->GetNewInternalRequest();
        flushRequest->CreateFlush(m_dummyRequestPath);
        m_storageDriveWin->QueueRequest(flushRequest);

        WaitTillCompleted();
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, FlushEntireCacheRequest_FlushPreviouslyReadFileAndMetaData_NoErrorsReported)
    {
        DoSingleRead();
        DoMetaDataRetrieval();
        // Wait here because normally the scheduler will only queue a flush when the stack is idle.
        WaitTillCompleted();

        AZ_TEST_START_TRACE_SUPPRESSION;
        AZ::IO::FileRequest* flushRequest = m_context->GetNewInternalRequest();
        flushRequest->CreateFlushAll();
        m_storageDriveWin->QueueRequest(flushRequest);

        WaitTillCompleted();
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, CollectStatistics_NoReadDone_NoStatisticsAreReturned)
    {
        AZStd::vector<Statistic> statistics;
        m_storageDriveWin->CollectStatistics(statistics);
        EXPECT_TRUE(statistics.empty());
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture, CollectStatistics_ReadDone_MoreThanZeroStatisticsReturned)
    {
        DoSingleRead();
        WaitTillCompleted();

        AZStd::vector<Statistic> statistics;
        m_storageDriveWin->CollectStatistics(statistics);
        EXPECT_FALSE(statistics.empty());
    }

    class Streamer_StorageDriveWindowsTestFixture_WithScheduler
        : public Streamer_StorageDriveWindowsTestFixture
    {
    public:
        void SetupStorageDrive(s32 overCommit)
        {
            Streamer_StorageDriveWindowsTestFixture::SetupStorageDrive(overCommit);

            if (m_streamer)
            {
                Interface<IStreamer>::Unregister(m_streamer);
                delete m_streamer;
            }
            AZStd::unique_ptr<Scheduler> stack = AZStd::make_unique<Scheduler>(m_storageDriveWin);
            m_streamer = aznew AZ::IO::Streamer(AZStd::thread_desc{}, AZStd::move(stack));
            ASSERT_NE(m_streamer, nullptr);
            Interface<IStreamer>::Register(m_streamer);
        }

        void SetUp() override
        {
            SetupStorageDrive(TestOverCommit);
        }

        void TearDown() override
        {
            Interface<IStreamer>::Unregister(m_streamer);
            delete m_streamer;

            Streamer_StorageDriveWindowsTestFixture::TearDown();
        }

    protected:
        Streamer* m_streamer{ nullptr };
    };

    TEST_F(Streamer_StorageDriveWindowsTestFixture_WithScheduler, ReadDataRequest_ParallelReadsUsingIStreamer_DataIsCorrect)
    {
        // Same test as above, but using IStreamer interface instead of directly targeting the 'StorageDriveWin' stack entry.
        constexpr size_t chunkSize = TestPhysicalSectorSize;
        constexpr size_t numChunks = 5;
        constexpr size_t fileSize = numChunks * chunkSize;
        AZStd::array<AZStd::unique_ptr<u8[]>, numChunks> buffers;
        AZStd::vector<AZ::IO::FileRequestPtr> requests;
        requests.reserve(numChunks);

        CreateDummyFile(fileSize, chunkSize, true);

        AZStd::binary_semaphore waitForReads;
        AZStd::atomic_size_t numCallbacks = 0;

        for (size_t i = 0; i < numChunks; ++i)
        {
            buffers[i].reset(new u8[chunkSize]);
            requests.push_back(m_streamer->Read(
                m_dummyFilepath,
                buffers[i].get(),
                chunkSize,
                chunkSize,
                IStreamerTypes::s_noDeadline,
                IStreamerTypes::s_priorityMedium,
                i * chunkSize
            ));

            AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                      // capture. Newer versions issue unused warning
            auto callback = [&numCallbacks, &waitForReads](FileRequestHandle request)
            AZ_POP_DISABLE_WARNING
            {
                IStreamer* streamer = Interface<IStreamer>::Get();
                if (streamer)
                {
                    auto result = streamer->GetRequestStatus(request);
                    EXPECT_EQ(result, IStreamerTypes::RequestStatus::Completed);
                }
                ++numCallbacks;
                if (numCallbacks == numChunks)
                {
                    waitForReads.release();
                }
            };

            m_streamer->SetRequestCompleteCallback(requests[i], AZStd::move(callback));
        }

        m_streamer->QueueRequestBatch(AZStd::move(requests));

        waitForReads.try_acquire_for(AZStd::chrono::seconds(5));

        // Check first & last bytes in first & last buffers/chunks.
        EXPECT_EQ(buffers[0][0], s_beginCharacter);
        EXPECT_EQ(buffers[0][chunkSize - 1], s_fileCharacter);
        EXPECT_EQ(buffers[numChunks - 1][0], s_chunkCharacter);
        EXPECT_EQ(buffers[numChunks - 1][chunkSize - 1], s_endCharacter);

        // Check first & last bytes in all interior buffers/chunks.
        for (size_t i = 1; i < numChunks - 1; ++i)
        {
            EXPECT_EQ(buffers[i][0], s_chunkCharacter);
            EXPECT_EQ(buffers[i][chunkSize - 1], s_fileCharacter);
        }
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture_WithScheduler, ReadDataRequest_CanceledParallelReads_ReadsAreCanceled)
    {
        constexpr size_t chunkSize = TestPhysicalSectorSize;
        constexpr size_t numChunks = 100;
        constexpr size_t fileSize = numChunks * chunkSize;

        AZStd::array<AZStd::unique_ptr<u8[]>, numChunks> buffers;
        AZStd::vector<AZ::IO::FileRequestPtr> requests;
        AZStd::vector<AZ::IO::FileRequestPtr> cancels;
        requests.reserve(numChunks);
        cancels.reserve(numChunks);

        CreateDummyFile(fileSize, chunkSize);
        
        AZStd::binary_semaphore waitForReads;
        AZStd::binary_semaphore waitForSingleRead;
        AZStd::atomic_size_t numReadCallbacks = 0;
        for (size_t i = 0; i < numChunks; ++i)
        {
            buffers[i].reset(new u8[chunkSize]);
            requests.push_back(m_streamer->Read(
                m_dummyFilepath,
                buffers[i].get(),
                chunkSize,
                chunkSize,
                IStreamerTypes::s_noDeadline,
                IStreamerTypes::s_priorityMedium,
                i * chunkSize
            ));

            AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                      // capture. Newer versions issue unused warning
            auto callback = [&waitForReads, &waitForSingleRead, &numReadCallbacks]([[maybe_unused]] FileRequestHandle request)
            AZ_POP_DISABLE_WARNING
            {
                numReadCallbacks++;
                if (numReadCallbacks == 1)
                {
                    waitForSingleRead.release();
                }
                else if (numReadCallbacks == numChunks)
                {
                    waitForReads.release();
                }
            };

            m_streamer->SetRequestCompleteCallback(requests[i], AZStd::move(callback));
        }

        AZStd::binary_semaphore waitForCancels;
        AZStd::atomic_size_t numCancelCallbacks = 0;
        for (size_t i = 0; i < numChunks; ++i)
        {
            cancels.push_back(m_streamer->Cancel(requests[numChunks - i - 1]));
            AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                      // capture. Newer versions issue unused warning
            auto callback = [&numCancelCallbacks, &waitForCancels](FileRequestHandle request)
            AZ_POP_DISABLE_WARNING
            {
                auto result = Interface<IStreamer>::Get()->GetRequestStatus(request);
                EXPECT_EQ(result, IStreamerTypes::RequestStatus::Completed);
                ++numCancelCallbacks;
                if (numCancelCallbacks == numChunks)
                {
                    waitForCancels.release();
                }
            };

            m_streamer->SetRequestCompleteCallback(cancels.back(), AZStd::move(callback));
        }

        m_streamer->QueueRequestBatch(AZStd::move(requests));
        waitForSingleRead.try_acquire_for(AZStd::chrono::seconds(1));
        m_streamer->QueueRequestBatch(AZStd::move(cancels));

        waitForCancels.try_acquire_for(AZStd::chrono::seconds(5));
        waitForReads.try_acquire_for(AZStd::chrono::seconds(5));

        EXPECT_GT(numCancelCallbacks, 0);
        EXPECT_EQ(numCancelCallbacks, numChunks);
        EXPECT_GT(numReadCallbacks, 0);
        EXPECT_EQ(numReadCallbacks, numChunks);
    }

    TEST_F(Streamer_StorageDriveWindowsTestFixture_WithScheduler, CancelRequest_CancelPendingRequest_PendingRequestCompletedWithCanceled)
    {
        constexpr size_t size = 16_kib;
        // This needs to be a large enough number so there are requests in the queue. Due to the aggressive completion and queue, faster
        // drives can prove to be able to read faster than requests can be queued.
        constexpr size_t numRequests = 1024;
        
        SetupStorageDrive(numRequests + 1); // Over commit so all request are queued in one go

        CreateDummyFile(size);

        char* buffers[size];
        AZStd::vector<AZ::IO::FileRequestPtr> requests;
        requests.reserve(numRequests);
        m_streamer->CreateRequestBatch(requests, numRequests);

        AZStd::atomic_int counter{ aznumeric_cast<int>(numRequests) };
        AZStd::binary_semaphore wait;
        auto callback = [&counter, &wait](FileRequestHandle)
        {
            if (--counter == 0)
            {
                wait.release();
            }
        };

        for (size_t i = 0; i < numRequests; ++i)
        {
            buffers[i] = reinterpret_cast<char*>(azmalloc(size, TestPhysicalSectorSize));
            m_streamer->Read(requests[i], m_dummyFilepath, buffers[i], size, size);
            m_streamer->SetRequestCompleteCallback(requests[i], callback);
        }

        AZ::IO::FileRequest* cancelRequest = m_context->GetNewInternalRequest();
        cancelRequest->CreateCancel(requests[numRequests - 1]);
        AZ::IO::FileRequestPtr sentinalRequest = m_streamer->Custom({});
        m_streamer->SetRequestCompleteCallback(sentinalRequest, [this, cancelRequest] (FileRequestHandle)
            {
                m_storageDriveWin->QueueRequest(cancelRequest);
            });
        
        // Suspend processing so all request are processed fully before reading begins.
        m_streamer->SuspendProcessing();
        m_streamer->QueueRequestBatch(requests);
        m_streamer->QueueRequest(sentinalRequest);
        m_streamer->ResumeProcessing();
       
        bool acquired = wait.try_acquire_for(AZStd::chrono::seconds(5));
        ASSERT_TRUE(acquired);

        ASSERT_EQ(0, counter);
        for (size_t i = 0; i < numRequests - 1; ++i)
        {
            EXPECT_EQ(AZ::IO::IStreamerTypes::RequestStatus::Completed, m_streamer->GetRequestStatus(requests[i]));
            azfree(buffers[i]);
        }
        EXPECT_EQ(AZ::IO::IStreamerTypes::RequestStatus::Canceled, m_streamer->GetRequestStatus(requests[numRequests - 1]));
        azfree(buffers[numRequests - 1]);
    }
} // namespace AZ::IO

#if defined(HAVE_BENCHMARK)

#include <benchmark/benchmark.h>

namespace Benchmark
{
    class StorageDriveWindowsFixture : public benchmark::Fixture
    {
        void internalTearDown()
        {
            using namespace AZ::IO;

            AZStd::string temp;
            m_absolutePath.swap(temp);

            delete m_streamer;
            m_streamer = nullptr;

            SystemFile::Delete(TestFileName);

            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_previousFileIO);
            delete m_fileIO;
            m_fileIO = nullptr;
        }
    public:
        constexpr static const char* TestFileName = "StreamerBenchmark.bin";
        constexpr static size_t FileSize = 64_mib;
            
        void SetupStreamer(bool enableFileSharing)
        {
            using namespace AZ::IO;

            m_fileIO = new UnitTest::TestFileIOBase();
            m_previousFileIO = AZ::IO::FileIOBase::GetInstance();
            AZ::IO::FileIOBase::SetInstance(nullptr);
            AZ::IO::FileIOBase::SetInstance(m_fileIO);

            SystemFile file;
            file.Open(TestFileName, SystemFile::OpenMode::SF_OPEN_CREATE | SystemFile::OpenMode::SF_OPEN_READ_WRITE);
            AZStd::unique_ptr<char[]> buffer(new char[FileSize]);
            ::memset(buffer.get(), 'c', FileSize);
            
            file.Write(buffer.get(), FileSize);
            file.Close();

            AZStd::optional<AZ::IO::FixedMaxPathString> absolutePath = AZ::Utils::ConvertToAbsolutePath(TestFileName);
            if (absolutePath.has_value())
            {
                AZStd::string drive;
                AZ::StringFunc::Path::GetDrive(absolutePath->c_str(), drive);

                m_absolutePath = *absolutePath;

                StorageDriveWin::ConstructionOptions options;
                options.m_hasSeekPenalty = false;
                options.m_enableUnbufferedReads = true; // Leave this on otherwise repeated loads will be using the Windows cache instead.
                options.m_enableSharing = enableFileSharing;
                options.m_minimalReporting = true;
                AZStd::shared_ptr<StreamStackEntry> storageDriveWin =
                    AZStd::make_shared<StorageDriveWin>(AZStd::vector<AZStd::string_view>{ drive }, 32, 32, 4_kib, 512, 8, 0, options);

                AZStd::unique_ptr<Scheduler> stack = AZStd::make_unique<Scheduler>(AZStd::move(storageDriveWin));
                m_streamer = aznew Streamer(AZStd::thread_desc{}, AZStd::move(stack));
            }
        }

        void TearDown(const benchmark::State&) override
        {
            internalTearDown();
        }
        void TearDown(benchmark::State&) override
        {
            internalTearDown();
        }

        void RepeatedlyReadFile(benchmark::State& state)
        {
            using namespace AZ::IO;
            using namespace AZStd::chrono;

            AZStd::unique_ptr<char[]> buffer(new char[FileSize]);
    
            for ([[maybe_unused]] auto _ : state)
            {
                AZStd::binary_semaphore waitForReads;
                AZStd::atomic<steady_clock::time_point> end;
                auto callback = [&end, &waitForReads]([[maybe_unused]] FileRequestHandle request)
                {
                    benchmark::DoNotOptimize(end = steady_clock::now());
                    waitForReads.release();
                };

                FileRequestPtr request = m_streamer->Read(m_absolutePath, buffer.get(), state.range(0), state.range(0));
                m_streamer->SetRequestCompleteCallback(request, callback);

                steady_clock::time_point start;
                benchmark::DoNotOptimize(start = steady_clock::now());
                m_streamer->QueueRequest(request);

                waitForReads.try_acquire_for(AZStd::chrono::seconds(5));
                auto durationInSeconds = duration_cast<duration<double>>(end.load() - start);

                state.SetIterationTime(durationInSeconds.count());

                m_streamer->QueueRequest(m_streamer->FlushCaches());
            }
        }

        AZStd::string m_absolutePath;
        AZ::IO::Streamer* m_streamer{};
        AZ::IO::FileIOBase* m_previousFileIO{};
        UnitTest::TestFileIOBase* m_fileIO{};
    };

    BENCHMARK_DEFINE_F(StorageDriveWindowsFixture, ReadsBaseline)(benchmark::State& state)
    {
        constexpr bool EnableFileSharing = false;
        SetupStreamer(EnableFileSharing);
        RepeatedlyReadFile(state);
    }

    BENCHMARK_DEFINE_F(StorageDriveWindowsFixture, ReadsWithFileReadSharingEnabled)(benchmark::State& state)
    {
        using namespace AZ::IO;

        constexpr bool EnableFileSharing = true;
        SetupStreamer(EnableFileSharing);
        RepeatedlyReadFile(state);
    }

    // For these benchmarks the CPU stat doesn't provide useful information because it uses GetThreadTimes on Window but since the main
    // thread is mostly sleeping while waiting for the read on the Streamer thread to complete this will report values (close to) zero.

    BENCHMARK_REGISTER_F(StorageDriveWindowsFixture, ReadsBaseline)
        ->RangeMultiplier(8)
        ->Range(1024, 64_mib)
        ->UseManualTime()
        ->Unit(benchmark::kMillisecond);

    BENCHMARK_REGISTER_F(StorageDriveWindowsFixture, ReadsWithFileReadSharingEnabled)
        ->RangeMultiplier(8)
        ->Range(1024, 64_mib)
        ->UseManualTime()
        ->Unit(benchmark::kMillisecond);

} // namespace Benchmark
#endif // HAVE_BENCHMARK
