/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/Asset/BaseAssetManagerTest.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Utils/Utils.h>
#include <Streamer/IStreamerMock.h>

namespace UnitTest
{
    /**
    * Find the current status of the reload
    */
    AZ::Data::AssetData::AssetStatus TestAssetManager::GetReloadStatus(const AssetId& assetId)
    {
        AZStd::lock_guard<AZStd::recursive_mutex> assetLock(m_assetMutex);

        auto reloadInfo = m_reloads.find(assetId);
        if (reloadInfo != m_reloads.end())
        {
            return reloadInfo->second.GetStatus();
        }
        return AZ::Data::AssetData::AssetStatus::NotLoaded;
    }

    const AZ::Data::AssetManager::OwnedAssetContainerMap& TestAssetManager::GetAssetContainers() const
    {
        return m_ownedAssetContainers;
    }

    const AssetManager::AssetMap& TestAssetManager::GetAssets() const
    {
        return m_assets;
    }

    void BaseAssetManagerTest::SetUp()
    {
        SerializeContextFixture::SetUp();

        SuppressTraceOutput(false);

        AZ::JobManagerDesc jobDesc;

        AZ::JobManagerThreadDesc threadDesc;
        for (size_t threadCount = 0; threadCount < GetNumJobManagerThreads(); threadCount++)
        {
            jobDesc.m_workerThreads.push_back(threadDesc);
        }
        m_jobManager = aznew AZ::JobManager(jobDesc);
        m_jobContext = aznew AZ::JobContext(*m_jobManager);
        AZ::JobContext::SetGlobalContext(m_jobContext);

        m_prevFileIO = IO::FileIOBase::GetInstance();
        IO::FileIOBase::SetInstance(&m_fileIO);

        m_streamer = CreateStreamer();
        if (m_streamer)
        {
            Interface<IO::IStreamer>::Register(m_streamer);
        }
    }

    void BaseAssetManagerTest::TearDown()
    {
        if (m_streamer)
        {
            Interface<IO::IStreamer>::Unregister(m_streamer);
        }
        DestroyStreamer(m_streamer);

        // Clean up any temporary asset files created during the test.
        for (auto& assetName : m_assetsWritten)
        {
            DeleteAssetFromDisk(assetName);
        }

        // Make sure to clear the memory from the name storage before shutting down the allocator.
        m_assetsWritten.clear();
        m_assetsWritten.shrink_to_fit();

        IO::FileIOBase::SetInstance(m_prevFileIO);

        AZ::JobContext::SetGlobalContext(nullptr);
        delete m_jobContext;
        delete m_jobManager;

        // Reset back to default suppression settings to avoid affecting other tests
        SuppressTraceOutput(true);

        SerializeContextFixture::TearDown();
    }

    void BaseAssetManagerTest::SuppressTraceOutput(bool suppress)
    {
        UnitTest::TestRunner::Instance().m_suppressAsserts = suppress;
        UnitTest::TestRunner::Instance().m_suppressErrors = suppress;
        UnitTest::TestRunner::Instance().m_suppressWarnings = suppress;
        UnitTest::TestRunner::Instance().m_suppressPrintf = suppress;
        UnitTest::TestRunner::Instance().m_suppressOutput = suppress;
    }

    void BaseAssetManagerTest::WriteAssetToDisk(const AZStd::string& assetName, [[maybe_unused]] const AZStd::string& assetIdGuid)
    {
        AZ::IO::Path assetFileName = GetTestFolderPath() / assetName;

        AssetWithCustomData asset;

        EXPECT_TRUE(AZ::Utils::SaveObjectToFile(assetFileName.Native(), AZ::DataStream::ST_XML, &asset, m_serializeContext));

        // Keep track of every asset written so that we can remove it on teardown
        m_assetsWritten.emplace_back(AZStd::move(assetFileName).Native());
    }

    void BaseAssetManagerTest::DeleteAssetFromDisk(const AZStd::string& assetName)
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (fileIO->Exists(assetName.c_str()))
        {
            fileIO->Remove(assetName.c_str());
        }
    }

    void BaseAssetManagerTest::BlockUntilAssetJobsAreComplete()
    {
        auto maxTimeout = AZStd::chrono::steady_clock::now() + DefaultTimeoutSeconds;

        while (AssetManager::Instance().HasActiveJobsOrStreamerRequests())
        {
            if (AZStd::chrono::steady_clock::now() > maxTimeout)
            {
                break;
            }
            AZStd::this_thread::yield();
        }

        EXPECT_FALSE(AssetManager::Instance().HasActiveJobsOrStreamerRequests());
    }

    MemoryStreamerWrapper::MemoryStreamerWrapper()
    {
        using ::testing::_;
        using ::testing::NiceMock;
        using ::testing::Return;

        ON_CALL(m_mockStreamer, SuspendProcessing()).WillByDefault([this]()
        {
            m_suspended = true;
        });

        ON_CALL(m_mockStreamer, ResumeProcessing()).WillByDefault([this]()
        {
            AZStd::unique_lock lock(m_mutex);

            m_suspended = false;

            while (!m_processingQueue.empty())
            {
                FileRequestHandle requestHandle = m_processingQueue.front();
                m_processingQueue.pop();

                const auto& onCompleteCallback = GetReadRequest(requestHandle)->m_callback;

                if (onCompleteCallback)
                {
                    onCompleteCallback(requestHandle);
                }
            }
        });

        ON_CALL(m_mockStreamer, Read(_, ::testing::An<IStreamerTypes::RequestMemoryAllocator&>(), _, _, _, _))
            .WillByDefault(
            [this](
            [[maybe_unused]] AZStd::string_view relativePath, IStreamerTypes::RequestMemoryAllocator& allocator, size_t size,
            AZStd::chrono::microseconds deadline, IStreamerTypes::Priority priority, [[maybe_unused]] size_t offset)
            {
                AZStd::unique_lock lock(m_mutex);

                ReadRequest request;

                // Save off the requested deadline and priority
                request.m_deadline = deadline;
                request.m_priority = priority;
                request.m_data = allocator.Allocate(size, size, 8);

                const auto* virtualFile = FindFile(relativePath);

                AZ_Assert(
                    virtualFile->size() == size, "Streamer read request size did not match size of saved file: %d vs %d (%.*s)",
                    virtualFile->size(), size,
                    relativePath.size(), relativePath.data());
                AZ_Assert(size > 0, "Size is zero %.*s", relativePath.size(), relativePath.data());

                memcpy(request.m_data.m_address, virtualFile->data(), size);

                // Create a real file request result and return it
                request.m_request = m_context.GetNewExternalRequest();

                m_readRequests.push_back(request);

                return request.m_request;
            });

        ON_CALL(m_mockStreamer, SetRequestCompleteCallback(_, _))
            .WillByDefault([this](FileRequestPtr& request, AZ::IO::IStreamer::OnCompleteCallback callback) -> FileRequestPtr&
            {
                // Save off the callback just so that we can call it when the request is "done"
                AZStd::unique_lock lock(m_mutex);
                ReadRequest* readRequest = GetReadRequest(request);
                readRequest->m_callback = callback;

                return request;
            });

        ON_CALL(m_mockStreamer, QueueRequest(_))
            .WillByDefault([this](const auto& fileRequest)
            {
                if (!m_suspended)
                {
                    decltype(ReadRequest::m_callback) onCompleteCallback;

                    AZStd::unique_lock lock(m_mutex);
                    ReadRequest* readRequest = GetReadRequest(fileRequest);
                    onCompleteCallback = readRequest->m_callback;

                    if (onCompleteCallback)
                    {
                        onCompleteCallback(fileRequest);

                        m_readRequests.erase(readRequest);
                    }
                }
                else
                {
                    AZStd::unique_lock lock(m_mutex);

                    m_processingQueue.push(fileRequest);
                }
            });

        ON_CALL(m_mockStreamer, GetRequestStatus(_))
            .WillByDefault([]([[maybe_unused]] FileRequestHandle request)
            {
                // Return whatever request status has been set in this class
                return IO::IStreamerTypes::RequestStatus::Completed;
            });

        ON_CALL(m_mockStreamer, GetReadRequestResult(_, _, _, _))
            .WillByDefault([this](
            [[maybe_unused]] FileRequestHandle request, void*& buffer, AZ::u64& numBytesRead,
            IStreamerTypes::ClaimMemory claimMemory)
            {
                // Make sure the requestor plans to free the data buffer we allocated.
                EXPECT_EQ(claimMemory, IStreamerTypes::ClaimMemory::Yes);

                AZStd::unique_lock lock(m_mutex);

                ReadRequest* readRequest = GetReadRequest(request);

                // Provide valid data buffer results.
                numBytesRead = readRequest->m_data.m_size;
                buffer = readRequest->m_data.m_address;

                return true;
            });

        ON_CALL(m_mockStreamer, RescheduleRequest(_, _, _))
            .WillByDefault([this](IO::FileRequestPtr target, AZStd::chrono::microseconds newDeadline, IO::IStreamerTypes::Priority newPriority)
            {
                AZStd::unique_lock lock(m_mutex);
                ReadRequest* readRequest = GetReadRequest(target);

                readRequest->m_deadline = newDeadline;
                readRequest->m_priority = newPriority;

                return target;
            });
    }

    ReadRequest* MemoryStreamerWrapper::GetReadRequest(FileRequestHandle request)
    {
        auto itr = AZStd::find_if(
            m_readRequests.begin(), m_readRequests.end(),
            [request](const ReadRequest& searchItem) -> bool
            {
                return (searchItem.m_request == request);
            });

        return itr;
    }

    AZStd::vector<char>* MemoryStreamerWrapper::FindFile(AZStd::string_view path)
    {
        auto itr = m_virtualFiles.find(path);

        if (itr == m_virtualFiles.end())
        {
            // Path didn't work as-is, does it have the test folder prefixed? If so try removing it
            AZ::IO::Path testFolderPath = GetTestFolderPath();
            if (AZ::IO::PathView(path).IsRelativeTo(testFolderPath))
            {
                AZ::IO::Path pathWithoutFolder = AZ::IO::PathView(path).LexicallyProximate(testFolderPath).String();

                itr = m_virtualFiles.find(pathWithoutFolder);
            }
            else // Path isn't prefixed, so try adding it
            {
                itr = m_virtualFiles.find(GetTestFolderPath() / path);
            }
        }

        if (itr != m_virtualFiles.end())
        {
            return &itr->second;
        }

        // Currently no test expects a file not to exist so we assert to make it easy to quickly find where something went wrong
        // If we ever need to test for a non-existent file this assert should just be conditionally disabled for that specific test
        AZ_Assert(false, "Failed to find virtual file %.*s", AZ_STRING_ARG(path))

        return nullptr;
    }

    void DisklessAssetManagerBase::SetUp()
    {
        using ::testing::_;
        using ::testing::NiceMock;
        using ::testing::Return;

        BaseAssetManagerTest::SetUp();

        ON_CALL(m_fileIO, Size(::testing::Matcher<const char*>(::testing::_), _))
            .WillByDefault(
            [this](const char* path, u64& size)
            {
                AZStd::scoped_lock lock(m_streamerWrapper->m_mutex);

                const auto* file = m_streamerWrapper->FindFile(path);

                if (file)
                {
                    size = file->size();
                    return ResultCode::Success;
                }

                AZ_Error("DisklessAssetManagerBase", false, "Failed to find virtual file %.*s", path);

                return ResultCode::Error;
            });

        m_prevFileIO = IO::FileIOBase::GetInstance();
        IO::FileIOBase::SetInstance(nullptr);
        IO::FileIOBase::SetInstance(&m_fileIO);
    }

    void DisklessAssetManagerBase::TearDown()
    {
        IO::FileIOBase::SetInstance(nullptr);
        IO::FileIOBase::SetInstance(m_prevFileIO);

        BaseAssetManagerTest::TearDown();
    }

    IO::IStreamer* DisklessAssetManagerBase::CreateStreamer()
    {
        m_streamerWrapper = AZStd::make_unique<MemoryStreamerWrapper>();

        return &(m_streamerWrapper->m_mockStreamer);
    }

    void DisklessAssetManagerBase::DestroyStreamer(IO::IStreamer*)
    {
        m_streamerWrapper = nullptr;
    }

    void DisklessAssetManagerBase::WriteAssetToDisk(const AZStd::string& assetName, const AZStd::string&)
    {
        AZ::IO::Path assetFileName = GetTestFolderPath() / assetName;

        AssetWithCustomData asset;

        EXPECT_TRUE(m_streamerWrapper->WriteMemoryFile(assetFileName.Native(), &asset, m_serializeContext));
    }

    void DisklessAssetManagerBase::DeleteAssetFromDisk(const AZStd::string&)
    {

    }
}
