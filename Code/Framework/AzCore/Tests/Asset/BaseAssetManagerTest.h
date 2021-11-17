/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Utils/Utils.h>
#include <Tests/FileIOBaseTestTypes.h>
#include <Tests/Asset/TestAssetTypes.h>
#include <Tests/SerializeContextFixture.h>
#include <Tests/TestCatalog.h>
#include <AzCore/UnitTest/Mocks/MockFileIOBase.h>
#include <Streamer/IStreamerMock.h>

namespace UnitTest
{

    // Helper subclass of AssetManager that makes it possible to query some of the normally hidden implementation.
    class TestAssetManager : public AssetManager
    {
    public:
        TestAssetManager(const Descriptor& desc) : AssetManager(desc) {}

        // Find the current status of the reload
        AZ::Data::AssetData::AssetStatus GetReloadStatus(const AssetId& assetId);

        // Get the number of jobs left to process
        size_t GetRemainingJobs() const;

        const AZ::Data::AssetManager::OwnedAssetContainerMap& GetAssetContainers() const;

        const AssetMap& GetAssets() const;

        // Expose these methods so that they can be queried by the unit tests.
        using AssetManager::GetAssetInternal;
        using AssetManager::GetAssetContainer;
    };

    // Base test class for the AssetManager unit tests that provides the general setup/teardown needed for all the tests.
    class BaseAssetManagerTest
        : public SerializeContextFixture
    {
    public:
        ~BaseAssetManagerTest() override = default;

        // Subclasses are required to declare how many job threads they would like for their tests.
        virtual size_t GetNumJobManagerThreads() const = 0;

        // Subclasses can optionally override the streamer creation and destruction
        virtual IO::IStreamer* CreateStreamer() { return aznew IO::Streamer(AZStd::thread_desc{}, StreamerComponent::CreateStreamerStack()); }
        virtual void DestroyStreamer(IO::IStreamer* streamer)
        {
            delete streamer;
            streamer = nullptr;
        }

        void SetUp() override;
        void TearDown() override;

        static void SuppressTraceOutput(bool suppress);

        // Helper methods to create and destroy actual assets on the disk for true end-to-end asset loading.
        virtual void WriteAssetToDisk(const AZStd::string& assetName, const AZStd::string& assetIdGuid);
        virtual void DeleteAssetFromDisk(const AZStd::string& assetName);

        void BlockUntilAssetJobsAreComplete();

        constexpr static AZStd::chrono::duration<AZStd::sys_time_t> DefaultTimeoutSeconds{ AZStd::chrono::seconds(AZ_TRAIT_UNIT_TEST_ASSET_MANAGER_TEST_DEFAULT_TIMEOUT_SECS) };

    protected:
        AZ::JobManager* m_jobManager{ nullptr };
        AZ::JobContext* m_jobContext{ nullptr };
        IO::FileIOBase* m_prevFileIO{ nullptr };
        IO::IStreamer* m_streamer{ nullptr };
        TestFileIOBase m_fileIO;
        AZStd::vector<AZStd::string> m_assetsWritten;
    };

    
    struct ReadRequest
    {
        AZStd::chrono::milliseconds m_deadline{};
        AZ::IO::IStreamerTypes::Priority m_priority{};
        IO::IStreamerTypes::RequestMemoryAllocatorResult m_data{ nullptr, 0, IO::IStreamerTypes::MemoryType::ReadWrite };
        AZ::IO::IStreamer::OnCompleteCallback m_callback;
        IO::FileRequestPtr m_request;
    };

    struct MemoryStreamerWrapper
    {
        MemoryStreamerWrapper();
        ~MemoryStreamerWrapper() = default;

        ReadRequest* GetReadRequest(IO::FileRequestHandle request);

        template<typename TObject>
        bool WriteMemoryFile(const AZStd::string& filePath, TObject* object, AZ::SerializeContext* context)
        {
            auto& buffer = m_virtualFiles[filePath];
            ByteContainerStream stream(&buffer);

            return AZ::Utils::SaveObjectToStream(stream, DataStream::StreamType::ST_XML, object, context);
        }

        AZStd::vector<char>* FindFile(AZStd::string_view path);

        ::testing::NiceMock<StreamerMock> m_mockStreamer;
        IO::StreamerContext m_context;
        AZStd::atomic_bool m_suspended{ false };

        AZStd::recursive_mutex m_mutex;
        AZStd::queue<FileRequestHandle> m_processingQueue; // Keeps tracks of requests that have been queued while processing is suspended
        AZStd::vector<ReadRequest> m_readRequests;
        AZStd::unordered_map<AZStd::string, AZStd::vector<char>> m_virtualFiles;
    };

    struct DisklessAssetManagerBase : BaseAssetManagerTest
    {
        void SetUp() override;
        void TearDown() override;
        IO::IStreamer* CreateStreamer() override;
        void DestroyStreamer(IO::IStreamer*) override;

        void WriteAssetToDisk(const AZStd::string& assetName, const AZStd::string& assetIdGuid) override;
        void DeleteAssetFromDisk(const AZStd::string& assetName) override;

        AZStd::unique_ptr<MemoryStreamerWrapper> m_streamerWrapper;
        ::testing::NiceMock<MockFileIOBase> m_fileIO;
        IO::FileIOBase* m_prevFileIO{};
    };

}
