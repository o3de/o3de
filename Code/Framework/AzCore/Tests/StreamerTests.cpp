/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <FileIOBaseTestTypes.h>
#include <AzCore/IO/CompressionBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/string/string.h>
#include <AzTest/GemTestEnvironment.h>

namespace AZ::IO
{
    namespace Utils
    {
        //! Create a test file that stores 4 byte integers starting at 0 and incrementing.
        //! @filename The name of the file to write to.
        //! @filesize The size the new file needs to be in bytes. The stored values will continue till fileSize / 4.
        //! @paddingSize The amount of data to insert before and after the file. In total paddingSize / 4 integers
        //!              will be added. The prefix will be marked with "0xdeadbeef" and the postfix with "0xd15ea5ed".
        static void CreateTestFile(const AZStd::string& name, size_t fileSize, size_t paddingSize)
        {
            constexpr size_t bufferByteSize = 1_mib;
            constexpr size_t bufferSize = bufferByteSize / sizeof(u32);
            u32* buffer = new u32[bufferSize];

            AZ_Assert(paddingSize < bufferByteSize, "Padding can't currently be larger than %i bytes.", bufferByteSize);
            size_t paddingCount = paddingSize / sizeof(u32);

            FileIOStream stream(name.c_str(), OpenMode::ModeWrite | OpenMode::ModeBinary);

            // Write pre-padding
            for (size_t i = 0; i < paddingCount; ++i)
            {
                buffer[i] = 0xdeadbeef;
            }
            stream.Write(paddingSize, buffer);

            // Write content
            u32 startIndex = 0;
            while (fileSize > bufferByteSize)
            {
                for (u32 i = 0; i < bufferSize; ++i)
                {
                    buffer[i] = startIndex + i;
                }
                startIndex += bufferSize;

                stream.Write(bufferByteSize, buffer);
                fileSize -= bufferByteSize;
            }
            for (u32 i = 0; i < bufferSize; ++i)
            {
                buffer[i] = startIndex + i;
            }
            stream.Write(fileSize, buffer);

            // Write post-padding
            for (size_t i = 0; i < paddingCount; ++i)
            {
                buffer[i] = 0xd15ea5ed;
            }
            stream.Write(paddingSize, buffer);

            delete[] buffer;
        }
    }

    struct DedicatedCache_Uncompressed {};
    struct GlobalCache_Uncompressed {};
    struct DedicatedCache_Compressed {};
    struct GlobalCache_Compressed {};

    enum class PadArchive : bool
    {
        Yes,
        No
    };

    class MockFileBase
    {
    public:
        virtual ~MockFileBase() = default;

        virtual void CreateTestFile(AZ::IO::PathView filePath, size_t fileSize, PadArchive padding) = 0;
        virtual AZ::IO::PathView GetFileName() const = 0;
    };

    class MockUncompressedFile
        : public MockFileBase
    {
    public:
        ~MockUncompressedFile() override
        {
            if (m_hasFile)
            {
                FileIOBase::GetInstance()->DestroyPath(m_filePath.c_str());
            }
        }

        void CreateTestFile(AZ::IO::PathView filePath, size_t fileSize, PadArchive) override
        {
            m_fileSize = fileSize;
            m_filePath = filePath;
            Utils::CreateTestFile(m_filePath.Native(), m_fileSize, 0);
            m_hasFile = true;
        }

        AZ::IO::PathView GetFileName() const override
        {
            return m_filePath;
        }

    private:
        AZ::IO::Path m_filePath;
        size_t m_fileSize = 0;
        bool m_hasFile = false;
    };

    class MockCompressedFile
        : public MockFileBase
        , public CompressionBus::Handler
    {
    public:
        static constexpr uint32_t s_tag = static_cast<uint32_t>('T') << 24 | static_cast<uint32_t>('E') << 16 | static_cast<uint32_t>('S') << 8 | static_cast<uint32_t>('T');
        static constexpr uint32_t s_paddingSize = 512; // Use this amount of bytes before and after a generated file as padding.

        ~MockCompressedFile() override
        {
            if (m_hasFile)
            {
                BusDisconnect();
                FileIOBase::GetInstance()->DestroyPath(m_filePath.c_str());
            }
        }

        void CreateTestFile(AZ::IO::PathView filePath, size_t fileSize, PadArchive padding) override
        {
            m_fileSize = fileSize;
            m_filePath = filePath;
            m_hasPadding = (padding == PadArchive::Yes);
            uint32_t paddingSize = s_paddingSize;
            Utils::CreateTestFile(m_filePath.Native(), m_fileSize / 2, m_hasPadding ? paddingSize : 0);

            m_hasFile = true;

            BusConnect();
        }

        AZ::IO::PathView GetFileName() const override
        {
            return m_filePath;
        }

        void Decompress(const AZ::IO::CompressionInfo& info, const void* compressed, size_t compressedSize,
            void* uncompressed, size_t uncompressedSize)
        {
            constexpr uint32_t tag = s_tag;
            ASSERT_EQ(info.m_compressionTag.m_code, tag);
            ASSERT_EQ(info.m_compressedSize, m_fileSize / 2);
            ASSERT_TRUE(info.m_isCompressed);
            uint32_t paddingSize = s_paddingSize;
            ASSERT_EQ(info.m_offset, m_hasPadding ? paddingSize : 0);
            ASSERT_EQ(info.m_uncompressedSize, m_fileSize);

            // Check the input
            ASSERT_EQ(compressedSize, m_fileSize / 2);
            const u32* values = reinterpret_cast<const u32*>(compressed);
            const size_t numValues = compressedSize / sizeof(u32);
            for (size_t i = 0; i < numValues; ++i)
            {
                EXPECT_EQ(values[i], i);
            }

            // Create the fake uncompressed data.
            ASSERT_EQ(uncompressedSize, m_fileSize);
            u32* output = reinterpret_cast<u32*>(uncompressed);
            size_t outputSize = uncompressedSize / sizeof(u32);
            for (size_t i = 0; i < outputSize; ++i)
            {
                output[i] = static_cast<u32>(i);
            }
        }

        //@{ CompressionBus Handler implementation.
        void FindCompressionInfo(bool& found, AZ::IO::CompressionInfo& info, const AZ::IO::PathView filePath) override
        {
            if (m_hasFile && m_filePath == filePath)
            {
                found = true;
                info.m_archiveFilename = RequestPath(m_filePath);
                ASSERT_TRUE(info.m_archiveFilename.IsValid());
                info.m_compressedSize = m_fileSize / 2;
                const uint32_t tag = s_tag;
                info.m_compressionTag.m_code = tag;
                info.m_isCompressed = true;
                uint32_t paddingSize = s_paddingSize;
                info.m_offset = m_hasPadding ? paddingSize : 0;
                info.m_uncompressedSize = m_fileSize;

                info.m_decompressor =
                    [this](const AZ::IO::CompressionInfo& info, const void* compressed,
                    size_t compressedSize, void* uncompressed, size_t uncompressedSize) -> bool
                {
                    Decompress(info, compressed, compressedSize, uncompressed, uncompressedSize);
                    return true;
                };
            }
        }
        //@}

    private:
        AZ::IO::Path m_filePath;
        size_t m_fileSize = 0;
        bool m_hasFile = false;
        bool m_hasPadding = false;
    };

    class GemTestApplication
        : public AZ::ComponentApplication
    {
    public:
        // ComponentApplication
        void SetSettingsRegistrySpecializations(SettingsRegistryInterface::Specializations& specializations) override
        {
            ComponentApplication::SetSettingsRegistrySpecializations(specializations);
            specializations.Append("test");
            specializations.Append("gemtest");
        }
    };

    class StreamerTestBase
        : public UnitTest::LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_prevFileIO = FileIOBase::GetInstance();
            FileIOBase::SetInstance(&m_fileIO);

            m_application = aznew GemTestApplication();
            AZ::ComponentApplication::Descriptor appDesc;
            appDesc.m_useExistingAllocator = true;
            auto m_systemEntity = m_application->Create(appDesc);
            m_systemEntity->AddComponent(aznew AZ::StreamerComponent());
            m_systemEntity->Init();
            m_systemEntity->Activate();

            m_streamer = Interface<IO::IStreamer>::Get();
        }

        void TearDown() override
        {
            m_streamer = nullptr;

            m_application->Destroy();
            delete m_application;
            m_application = nullptr;

            FileIOBase::SetInstance(m_prevFileIO);

            LeakDetectionFixture::TearDown();
        }

        //! Requests are typically completed by Streamer before it updates it's internal bookkeeping.
        //! If a test depends on getting status information such as if cache files have been cleared
        //! then call WaitForScheduler to give Steamers scheduler some time to update it's internal status.
        void WaitForScheduler()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::microseconds(1));
        }

    protected:
        virtual AZStd::unique_ptr<MockFileBase> CreateMockFile() = 0;
        virtual bool IsUsingArchive() const = 0;
        virtual bool CreateDedicatedCache() const = 0;

        //! Create a test file that stores 4 byte integers starting at 0 and incrementing.
        //! @filesize The size the new file needs to be in bytes. The stored values will continue till fileSize / 4.
        //! @return The name of the test file.
        AZStd::unique_ptr<MockFileBase> CreateTestFile(size_t fileSize, PadArchive padding)
        {
            AZStd::string name = AZStd::string::format("TestFile_%zu.test", m_testFileCount++);

            AZ::IO::Path testFullPath = m_tempDirectory.GetDirectory();
            testFullPath /= name;

            AZStd::unique_ptr<MockFileBase> result = CreateMockFile();
            result->CreateTestFile(testFullPath.c_str(), fileSize, padding);
            if (CreateDedicatedCache())
            {
                AZ::Interface<AZ::IO::IStreamer>::Get()->CreateDedicatedCache(name.c_str());
            }
            return result;
        }

        void VerifyTestFile(const void* buffer, size_t fileSize, size_t offset = 0)
        {
            size_t count = fileSize / sizeof(u32);
            size_t numOffset = offset / sizeof(u32);
            const u32* data = reinterpret_cast<const u32*>(buffer);
            for (size_t i = 0; i < count; ++i)
            {
                EXPECT_EQ(data[i], i + numOffset);
            }
        }

        void AssertTestFile(const void* buffer, size_t fileSize, size_t offset = 0)
        {
            size_t count = fileSize / sizeof(u32);
            size_t numOffset = offset / sizeof(u32);
            const u32* data = reinterpret_cast<const u32*>(buffer);
            for (size_t i = 0; i < count; ++i)
            {
                ASSERT_EQ(data[i], i + numOffset);
            }
        }

        void PeriodicallyCheckedRead(AZ::IO::PathView filePath, void* buffer, u64 fileSize, u64 offset, AZStd::chrono::seconds timeOut, bool& result)
        {
            AZStd::binary_semaphore sync;

            AZStd::atomic_bool readSuccessful = false;
            auto callback = [&readSuccessful, &sync](FileRequestHandle request)
            {
                auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
                readSuccessful = streamer->GetRequestStatus(request) == IStreamerTypes::RequestStatus::Completed;
                sync.release();
            };

            FileRequestPtr request = this->m_streamer->Read(filePath.Native(), buffer, fileSize, fileSize,
                IStreamerTypes::s_deadlineNow, IStreamerTypes::s_priorityMedium, offset);
            this->m_streamer->SetRequestCompleteCallback(request, AZStd::move(callback));
            this->m_streamer->QueueRequest(AZStd::move(request));

            bool hasTimedOut = !sync.try_acquire_for(timeOut);
            result = readSuccessful && !hasTimedOut;
            ASSERT_FALSE(hasTimedOut);
            ASSERT_TRUE(readSuccessful);
        }

        AZ::Test::ScopedAutoTempDirectory m_tempDirectory;
        UnitTest::TestFileIOBase m_fileIO;
        FileIOBase* m_prevFileIO{ nullptr };
        IStreamer* m_streamer{ nullptr };
        AZ::ComponentApplication* m_application{ nullptr };
        size_t m_testFileCount{ 0 };
    };

    template<typename TestTag>
    class StreamerTest : public StreamerTestBase
    {
    protected:
        bool IsUsingArchive() const override
        {
            AZ_Assert(false, "Not correctly specialized.");
            return false;
        }

        bool CreateDedicatedCache() const override
        {
            AZ_Assert(false, "Not correctly specialized.");
            return false;
        }

        AZStd::unique_ptr<MockFileBase> CreateMockFile() override
        {
            AZ_Assert(false, "Not correctly specialized.");
            return nullptr;
        }
    };

    template<>
    class StreamerTest<DedicatedCache_Uncompressed> : public StreamerTestBase
    {
    protected:
        bool IsUsingArchive() const override { return false; }
        bool CreateDedicatedCache() const override { return true; }
        AZStd::unique_ptr<MockFileBase> CreateMockFile() override
        {
            return AZStd::make_unique<MockUncompressedFile>();
        }
    };

    template<>
    class StreamerTest<GlobalCache_Uncompressed> : public StreamerTestBase
    {
    protected:
        bool IsUsingArchive() const override { return false; }
        bool CreateDedicatedCache() const override { return false; }
        AZStd::unique_ptr<MockFileBase> CreateMockFile() override
        {
            return AZStd::make_unique<MockUncompressedFile>();
        }
    };

    template<>
    class StreamerTest<DedicatedCache_Compressed> : public StreamerTestBase
    {
    protected:
        bool IsUsingArchive() const override { return true; }
        bool CreateDedicatedCache() const override { return true; }
        AZStd::unique_ptr<MockFileBase> CreateMockFile() override
        {
            return AZStd::make_unique<MockCompressedFile>();
        }
    };

    template<>
    class StreamerTest<GlobalCache_Compressed> : public StreamerTestBase
    {
    protected:
        bool IsUsingArchive() const override { return true; }
        bool CreateDedicatedCache() const override { return false; }
        AZStd::unique_ptr<MockFileBase> CreateMockFile() override
        {
            return AZStd::make_unique<MockCompressedFile>();
        }
    };

#if !AZ_TRAIT_DISABLE_FAILED_STREAMER_TESTS

    TYPED_TEST_CASE_P(StreamerTest);

    // Read a file that's smaller than the cache.
    TYPED_TEST_P(StreamerTest, Read_ReadSmallFileEntirely_FileFullyRead)
    {
        constexpr size_t fileSize = 50_kib;
        auto testFile = this->CreateTestFile(fileSize, PadArchive::No);

        char buffer[fileSize];
        bool readResult{ false };
        this->PeriodicallyCheckedRead(testFile->GetFileName(), buffer, fileSize, 0, AZStd::chrono::seconds(5), readResult);
        EXPECT_TRUE(readResult);
        if(readResult)
        {
            this->VerifyTestFile(buffer, fileSize);
        }
    }

    // Read a large file that will need to be broken into chunks.
    TYPED_TEST_P(StreamerTest, Read_ReadLargeFileEntirely_FileFullyRead)
    {
        constexpr size_t fileSize = 10_mib;
        auto testFile = this->CreateTestFile(fileSize, PadArchive::No);

        char* buffer = new char[fileSize];
        bool readResult{ false };
        this->PeriodicallyCheckedRead(testFile->GetFileName(), buffer, fileSize, 0, AZStd::chrono::seconds(5), readResult);
        EXPECT_TRUE(readResult);
        if(readResult)
        {
            this->VerifyTestFile(buffer, fileSize);
        }

        delete[] buffer;
    }

    // Reads multiple small pieces to make sure that the cache is hit, seeded and copied properly.
    TYPED_TEST_P(StreamerTest, Read_ReadMultiplePieces_AllReadRequestWereSuccessful)
    {
        constexpr size_t fileSize = 2_mib;
        // Deliberately not taking a multiple of the file size so at least one read will have a partial cache hit.
#if defined(AZ_DEBUG_BUILD)
        constexpr size_t bufferSize = 4800;
#else
        constexpr size_t bufferSize = 480;
#endif
        constexpr size_t readBlock = bufferSize * sizeof(u32);

        auto testFile = this->CreateTestFile(fileSize, PadArchive::No);

        u32 buffer[bufferSize];
        size_t block = 0;
        size_t fileRemainder = fileSize;
        for (block = 0; block < fileSize; block += readBlock)
        {
            size_t blockSize = AZStd::min(readBlock, fileRemainder);
            bool readResult{ false };
            this->PeriodicallyCheckedRead(testFile->GetFileName(), buffer, blockSize, block, AZStd::chrono::seconds(5), readResult);
            EXPECT_TRUE(readResult);
            if (readResult)
            {
                this->AssertTestFile(buffer, blockSize, block);
            }

            fileRemainder -= blockSize;
        }
    }

    // Same as the previous test, but all requests are submitted in a single batch.
    TYPED_TEST_P(StreamerTest, Read_ReadMultiplePiecesWithBatch_AllReadRequestWereSuccessful)
    {
        constexpr size_t fileSize = 2_mib;
        // Deliberately not taking a multiple of the file size so at least one read will have a partial cache hit.
#if defined(AZ_DEBUG_BUILD)
        constexpr size_t bufferSize = 4800 * sizeof(u32);
#else
        constexpr size_t bufferSize = 480 * sizeof(u32);
#endif
        constexpr size_t numRequests = (fileSize / bufferSize) + 1;

        auto testFile = this->CreateTestFile(fileSize, PadArchive::No);

        AZStd::vector<FileRequestPtr> requests;
        this->m_streamer->CreateRequestBatch(requests, numRequests);

        AZStd::binary_semaphore sync;
        AZStd::atomic_int remainingReads = numRequests;

        AZStd::atomic_bool readSuccessful = true;
        auto callback = [&readSuccessful, &sync, &remainingReads](FileRequestHandle request)
        {
            if (AZ::Interface<IStreamer>::Get()->GetRequestStatus(request) != IStreamerTypes::RequestStatus::Completed)
            {
                readSuccessful = false;
            }
            if (--remainingReads == 0)
            {
                sync.release();
            }
        };

        u8* buffer = new u8[fileSize];
        size_t block = 0;
        size_t fileRemainder = fileSize;
        size_t requestIndex = 0;
        for (block = 0; block < fileSize; block += bufferSize)
        {
            size_t blockSize = AZStd::min(bufferSize, fileRemainder);
            this->m_streamer->Read(requests[requestIndex], testFile->GetFileName().Native(), buffer + block, blockSize, blockSize,
                IStreamerTypes::s_deadlineNow, IStreamerTypes::s_priorityMedium, block);
            this->m_streamer->SetRequestCompleteCallback(requests[requestIndex], callback);
            fileRemainder -= blockSize;
            requestIndex++;
        }

        this->m_streamer->QueueRequestBatch(requests);
        bool hasTimedOut = !sync.try_acquire_for(AZStd::chrono::minutes(10)); // Especially in debug this can take a long time.
        EXPECT_FALSE(hasTimedOut);
        EXPECT_TRUE(readSuccessful);

        fileRemainder = fileSize;
        for (block = 0; block < fileSize; block += bufferSize)
        {
            size_t blockSize = AZStd::min(bufferSize, fileRemainder);
            this->AssertTestFile(buffer + block, blockSize, block);
            fileRemainder -= blockSize;
        }

        delete[] buffer;
    }

    // Queue a request on a suspended device, then resume to see if gets picked up again.
    TYPED_TEST_P(StreamerTest, SuspendProcessing_SuspendWhileFileIsQueued_FileIsNotReadUntilProcessingIsRestarted)
    {
        constexpr size_t fileSize = 50_kib;
        auto testFile = this->CreateTestFile(fileSize, PadArchive::No);

        AZStd::binary_semaphore sync;

        AZStd::atomic_bool readSuccessful = false;
        auto callback = [&readSuccessful, &sync](FileRequestHandle request)
        {
            readSuccessful = AZ::Interface<IStreamer>::Get()->GetRequestStatus(request) == IStreamerTypes::RequestStatus::Completed;
            sync.release();
        };

        char buffer[fileSize];
        FileRequestPtr request = this->m_streamer->Read(testFile->GetFileName().Native(), buffer, fileSize, fileSize);
        this->m_streamer->SetRequestCompleteCallback(request, AZStd::move(callback));

        this->m_streamer->SuspendProcessing();
        this->m_streamer->QueueRequest(AZStd::move(request));

        // Sleep for a short while to make sure the test doesn't outrun the Streamer.
        AZStd::this_thread::sleep_for(AZStd::chrono::seconds(1));
        EXPECT_EQ(IStreamerTypes::RequestStatus::Pending, this->m_streamer->GetRequestStatus(request));

        // Wait for a maximum of a few seconds for the request to complete. If it doesn't, the suspend is most likely stuck and the test should fail.
        this->m_streamer->ResumeProcessing();
        bool hasTimedOut = !sync.try_acquire_for(AZStd::chrono::seconds(5));
        EXPECT_FALSE(hasTimedOut);
        EXPECT_TRUE(readSuccessful);
    }

    TYPED_TEST_P(StreamerTest, FlushCaches_FlushAfterEveryRead_FilesAreReadCorrectly)
    {
        constexpr size_t fileSize = 4_mib;
        constexpr size_t fileCount = 128;

        AZStd::vector<AZStd::unique_ptr<MockFileBase>> testFiles;
        AZStd::vector<AZStd::unique_ptr<char[]>> testData;
        AZStd::vector<FileRequestPtr> requests;
        testFiles.reserve(fileCount);
        testData.reserve(fileCount);
        requests.reserve(fileCount * 2);

        AZStd::binary_semaphore sync;
        AZStd::atomic_bool readSuccessful = true;
        AZStd::atomic_int counter = fileCount * 2;

        auto callback = [&sync, &counter, &readSuccessful](FileRequestHandle request)
        {
            readSuccessful = readSuccessful && AZ::Interface<IStreamer>::Get()->GetRequestStatus(request) == IStreamerTypes::RequestStatus::Completed;
            counter--;
            if (counter == 0)
            {
                sync.release();
            }
        };

        for (size_t i = 0; i < fileCount; ++i)
        {
            auto testFile = this->CreateTestFile(fileSize, PadArchive::No);
            AZStd::unique_ptr<char[]> buffer(new char[fileSize]);

            auto readRequest = this->m_streamer->Read(testFile->GetFileName().Native(), buffer.get(), fileSize, fileSize);
            this->m_streamer->SetRequestCompleteCallback(readRequest, callback);
            auto flushRequest = this->m_streamer->FlushCaches();
            this->m_streamer->SetRequestCompleteCallback(flushRequest, callback);

            requests.push_back(AZStd::move(readRequest));
            requests.push_back(AZStd::move(flushRequest));

            testFiles.push_back(AZStd::move(testFile));
            testData.push_back(AZStd::move(buffer));
        }

        for (size_t i = 0; i < fileCount * 2; i += 2)
        {
            this->m_streamer->QueueRequest(requests[i]);
            this->m_streamer->QueueRequest(requests[i + 1]);
            AZStd::this_thread::yield();
        }

        bool hasTimedOut = !sync.try_acquire_for(AZStd::chrono::seconds(30));
        EXPECT_FALSE(hasTimedOut);
        EXPECT_TRUE(readSuccessful);
    }

    REGISTER_TYPED_TEST_CASE_P(StreamerTest,
        Read_ReadSmallFileEntirely_FileFullyRead,
        Read_ReadLargeFileEntirely_FileFullyRead,
        Read_ReadMultiplePieces_AllReadRequestWereSuccessful,
        Read_ReadMultiplePiecesWithBatch_AllReadRequestWereSuccessful,
        SuspendProcessing_SuspendWhileFileIsQueued_FileIsNotReadUntilProcessingIsRestarted,
        FlushCaches_FlushAfterEveryRead_FilesAreReadCorrectly);

    using StreamerTestCases = ::testing::Types<GlobalCache_Uncompressed, DedicatedCache_Uncompressed, GlobalCache_Compressed, DedicatedCache_Compressed>;

    INSTANTIATE_TYPED_TEST_CASE_P(StreamerTests, StreamerTest, StreamerTestCases);
#endif // AZ_TRAIT_DISABLE_FAILED_STREAMER_TESTS

} // namespace AZ::IO
