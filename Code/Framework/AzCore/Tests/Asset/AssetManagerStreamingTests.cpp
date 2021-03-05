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
#include <AzCore/Asset/AssetManager.h>
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
#include <Tests/Asset/BaseAssetManagerTest.h>
#include <Tests/Asset/MockLoadAssetCatalogAndHandler.h>
#include <Tests/Asset/TestAssetTypes.h>
#include <Tests/SerializeContextFixture.h>
#include <Tests/TestCatalog.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Data;

    struct StreamerWrapper
    {
        StreamerWrapper()
        {
            using ::testing::_;
            using ::testing::NiceMock;
            using ::testing::Return;

            ON_CALL(m_mockStreamer, Read(_, ::testing::An<IStreamerTypes::RequestMemoryAllocator&>(), _, _, _, _))
                .WillByDefault([this](
                    [[maybe_unused]] AZStd::string_view relativePath,
                    IStreamerTypes::RequestMemoryAllocator& allocator,
                    size_t size,
                    AZStd::chrono::microseconds deadline,
                    IStreamerTypes::Priority priority,
                    [[maybe_unused]] size_t offset)
                    {
                        // Save off the requested deadline and priority
                        m_deadline = deadline;
                        m_priority = priority;
                        // Allocate a real data buffer for the supposedly read-in asset
                        m_data = allocator.Allocate(size, size, 8);
                        // Create a real file request result and return it
                        m_request = m_context.GetNewExternalRequest();
                        return m_request;
                    });

            ON_CALL(m_mockStreamer, SetRequestCompleteCallback(_, _))
                .WillByDefault([this](FileRequestPtr& request, AZ::IO::IStreamer::OnCompleteCallback callback) -> FileRequestPtr&
                    {
                        // Save off the callback just so that we can call it when the request is "done"
                        m_callback = callback;
                        return request;
                    });

            ON_CALL(m_mockStreamer, GetRequestStatus(_))
                .WillByDefault([this]([[maybe_unused]] FileRequestHandle request)
                    {
                        // Return whatever request status has been set in this class
                        return IO::IStreamerTypes::RequestStatus::Completed;
                    });

            ON_CALL(m_mockStreamer, GetReadRequestResult(_, _, _, _))
                .WillByDefault([this](
                    [[maybe_unused]] FileRequestHandle request,
                    void*& buffer,
                    AZ::u64& numBytesRead,
                    IStreamerTypes::ClaimMemory claimMemory)
                    {
                        // Make sure the requestor plans to free the data buffer we allocated.
                        EXPECT_EQ(claimMemory, IStreamerTypes::ClaimMemory::Yes);

                        // Provide valid data buffer results.
                        numBytesRead = m_data.m_size;
                        buffer = m_data.m_address;

                        // Clear out our stored values for this, because we're handing off ownership to the caller.
                        m_data.m_address = nullptr;
                        m_data.m_size = 0;

                        return true;
                    });

            ON_CALL(m_mockStreamer, RescheduleRequest(_, _, _))
                .WillByDefault([this](IO::FileRequestPtr target, AZStd::chrono::microseconds newDeadline, IO::IStreamerTypes::Priority newPriority)
                    {
                        m_deadline = newDeadline;
                        m_priority = newPriority;

                        return target;
                    });
        }

        ~StreamerWrapper() = default;

        ::testing::NiceMock<StreamerMock> m_mockStreamer;

        AZStd::chrono::milliseconds m_deadline;
        AZ::IO::IStreamerTypes::Priority m_priority;
        IO::StreamerContext m_context;
        AZ::IO::IStreamer::OnCompleteCallback m_callback;
        IO::FileRequestPtr m_request;
        IO::IStreamerTypes::RequestMemoryAllocatorResult m_data{ nullptr, 0, IO::IStreamerTypes::MemoryType::ReadWrite };
    };

    // Use a mock asset catalog and asset handler to pretend to create and load an asset, since we don't really care about the data
    // inside the asset itself for these tests.
    // This subclass overrides the asset information to provide non-zero asset sizes so that the asset load makes it all the way to the
    // mocked-out streamer class, so that we can track information about streamer deadline and priority changes.
    // This subclass also provides facilities for getting/setting the default deadline and priority so that we can test the usage of
    // those values as well.
    class MockLoadAssetWithNonZeroSizeCatalogAndHandler
        : public MockLoadAssetCatalogAndHandler
    {
    public:
        MockLoadAssetWithNonZeroSizeCatalogAndHandler(
            AZStd::unordered_set<AZ::Data::AssetId> ids
            , AZ::Data::AssetType assetType
            , AZ::Data::AssetPtr(*createAsset)()
            , void(*destroyAsset)(AZ::Data::AssetPtr asset))
            : MockLoadAssetCatalogAndHandler(ids, assetType, createAsset, destroyAsset)
        {
        }

        // Overridden to provide a non-zero asset size so that the asset load makes it to the streamer.
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override
        {
            AZ::Data::AssetInfo result;
            if (m_ids.contains(id))
            {
                result.m_assetType = m_assetType;
                result.m_assetId = id;
                result.m_sizeBytes = sizeof(EmptyAsset);
            }
            return result;
        }

        // Overridden to provide a non-zero size and non-empty name so that the asset load makes it to the streamer.
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad([[maybe_unused]] const AZ::Data::AssetId& id,
            [[maybe_unused]] const AZ::Data::AssetType& type) override
        {
            AZ::Data::AssetStreamInfo info;
            info.m_dataLen = sizeof(EmptyAsset);
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;
            info.m_streamName = "test";
            return info;
        }

        // Provides controllable default values for deadlines and priorities.
        void GetDefaultAssetLoadPriority([[maybe_unused]] AssetType type, AZStd::chrono::milliseconds& defaultDeadline,
            AZ::IO::IStreamerTypes::Priority& defaultPriority) const override
        {
            defaultDeadline = GetDefaultDeadline();
            defaultPriority = GetDefaultPriority();
        }

        AZStd::chrono::milliseconds GetDefaultDeadline() const { return m_defaultDeadline; }
        AZ::IO::IStreamerTypes::Priority GetDefaultPriority() const { return m_defaultPriority; }

        void SetDefaultDeadline(AZStd::chrono::milliseconds deadline) { m_defaultDeadline = deadline; }
        void SetDefaultPriority(AZ::IO::IStreamerTypes::Priority priority) { m_defaultPriority = priority; }

    protected:
        AZStd::chrono::milliseconds m_defaultDeadline{ AZStd::chrono::milliseconds(0) };
        AZ::IO::IStreamerTypes::Priority m_defaultPriority{ AZ::IO::IStreamerTypes::s_priorityLowest };
    };


    // Tests that validate the interaction between AssetManager and the IO Streamer
    struct AssetManagerStreamerTests
        : BaseAssetManagerTest
    {
        static inline const AZ::Uuid MyAsset1Id{ "{5B29FE2B-6B41-48C9-826A-C723951B0560}" };

        void SetUp() override
        {
            BaseAssetManagerTest::SetUp();

            // create the database
            AssetManager::Descriptor desc;
            AssetManager::Create(desc);
        }

        void TearDown() override
        {
            // This will also delete m_assetHandlerAndCatalog
            AssetManager::Destroy();

            BaseAssetManagerTest::TearDown();
        }

        size_t GetNumJobManagerThreads() const override
        {
            return 1;
        }

        // Create a mock streamer instead of a real one.
        IO::IStreamer* CreateStreamer() override
        {
            m_mockStreamer = AZStd::make_unique<StreamerWrapper>();
            return &(m_mockStreamer->m_mockStreamer);
        }

        void DestroyStreamer([[maybe_unused]] IO::IStreamer* streamer) override
        {
            m_mockStreamer = nullptr;
        }

        AZStd::unique_ptr<StreamerWrapper> m_mockStreamer;
    };

    TEST_F(AssetManagerStreamerTests, LoadReschedule)
    {
        struct DeadlinePriorityTest
        {
            // The deadline / priority values to request for this test
            AZStd::chrono::milliseconds m_requestDeadline;
            AZ::IO::IStreamerTypes::Priority m_requestPriority;

            // The expected value of the deadline / priority after the request
            AZStd::chrono::milliseconds m_resultDeadline;
            AZ::IO::IStreamerTypes::Priority m_resultPriority;
        };

        DeadlinePriorityTest tests[] =
        {
            // Initial asset request with deadline and priority.
            // Results should match what was requested.
            {AZStd::chrono::milliseconds(1000), AZ::IO::IStreamerTypes::s_priorityLow,
             AZStd::chrono::milliseconds(1000), AZ::IO::IStreamerTypes::s_priorityLow},

            // Make the deadline longer and the priority higher.
            // Only the priority should change.
            {AZStd::chrono::milliseconds(1500), AZ::IO::IStreamerTypes::s_priorityHigh,
             AZStd::chrono::milliseconds(1000), AZ::IO::IStreamerTypes::s_priorityHigh},

            // Make the deadline shorter and the priority lower.
            // Only the deadline should change.
            {AZStd::chrono::milliseconds(500), AZ::IO::IStreamerTypes::s_priorityLow,
             AZStd::chrono::milliseconds(500), AZ::IO::IStreamerTypes::s_priorityHigh},

            // Make the deadline shorter and the priority higher.
            // Both the deadline and the priority should change.
            {AZStd::chrono::milliseconds(250), AZ::IO::IStreamerTypes::s_priorityHigh + 1,
             AZStd::chrono::milliseconds(250), AZ::IO::IStreamerTypes::s_priorityHigh + 1},
        };

        // The deadline needs to be shorter than the last test scenario, and the priority higher, so that we can
        // verify that these values are actually used in our final test scenario.
        AZStd::chrono::milliseconds assetHandlerDefaultDeadline = AZStd::chrono::milliseconds(200);
        AZ::IO::IStreamerTypes::Priority assetHandlerDefaultPriority = AZ::IO::IStreamerTypes::s_priorityHigh + 2;

        UnitTest::MockLoadAssetWithNonZeroSizeCatalogAndHandler testAssetCatalog(
            { MyAsset1Id },
            azrtti_typeid<EmptyAsset>(),
            []() { return AssetPtr(aznew EmptyAsset()); },
            [](AssetPtr ptr) { delete ptr; }
            );

        {
            AssetLoadParameters loadParams;
            AZ::Data::Asset<EmptyAsset> asset1;

            // Run through every test scenario and verify that the results match expectations.
            for (auto& test : tests)
            {
                loadParams.m_deadline = test.m_requestDeadline;
                loadParams.m_priority = test.m_requestPriority;
                asset1 = AssetManager::Instance().GetAsset<EmptyAsset>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default, loadParams);

                ASSERT_TRUE(asset1);

                EXPECT_EQ(m_mockStreamer->m_deadline, test.m_resultDeadline);
                EXPECT_EQ(m_mockStreamer->m_priority, test.m_resultPriority);

            }

            // Final scenario:  Request another load with no deadline or priority set.
            // This should use the defaults from the asset handler.
            loadParams.m_deadline = {};
            loadParams.m_priority = {};
            testAssetCatalog.SetDefaultDeadline(assetHandlerDefaultDeadline);
            testAssetCatalog.SetDefaultPriority(assetHandlerDefaultPriority);
            // (Verify that we've chosen a shorter deadline and higher priority for our defaults than our current state)
            EXPECT_LT(assetHandlerDefaultDeadline, m_mockStreamer->m_deadline);
            EXPECT_GT(assetHandlerDefaultPriority, m_mockStreamer->m_priority);

            asset1 = AssetManager::Instance().GetAsset<EmptyAsset>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default, loadParams);

            ASSERT_TRUE(asset1);

            EXPECT_EQ(m_mockStreamer->m_deadline, assetHandlerDefaultDeadline);
            EXPECT_EQ(m_mockStreamer->m_priority, assetHandlerDefaultPriority);

            // Run callback to cleanup and wait for the Asset Manager to finish processing the loaded asset.
            m_mockStreamer->m_callback(m_mockStreamer->m_request);
            asset1.BlockUntilLoadComplete();

            // Allow the asset manager to finish processing the "OnAssetReady" event so that it doesn't hold extra
            // references to the asset.  This allows the asset to get cleaned up correctly at the end of the test.
            AssetManager::Instance().DispatchEvents();

            // Clear out our pointers so that they clean themselves up and release any references to the loading asset.
            // If we didn't do this, the asset might not get cleaned up until the mock streamer is deleted, which happens
            // after the asset manager shuts down.  This would cause asserts and potentially a crash.
            m_mockStreamer->m_callback = nullptr;
            m_mockStreamer->m_request = nullptr;
        }
    }

}
