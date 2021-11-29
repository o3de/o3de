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
    using namespace AZ;
    using namespace AZ::Data;

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

    size_t TestAssetManager::GetRemainingJobs() const
    {
        return m_activeJobs.size();
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
        AZStd::string assetFileName = GetTestFolderPath() + assetName;

        AssetWithCustomData asset;

        EXPECT_TRUE(AZ::Utils::SaveObjectToFile(assetFileName, AZ::DataStream::ST_XML, &asset, m_serializeContext));

        // Keep track of every asset written so that we can remove it on teardown
        m_assetsWritten.emplace_back(AZStd::move(assetFileName));
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
        auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

        while (AssetManager::Instance().HasActiveJobsOrStreamerRequests())
        {
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                break;
            }
            AZStd::this_thread::yield();
        }

        EXPECT_FALSE(AssetManager::Instance().HasActiveJobsOrStreamerRequests());
    }
}
