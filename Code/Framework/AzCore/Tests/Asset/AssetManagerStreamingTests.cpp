/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Asset/AssetManager.h>
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
                .WillByDefault([]([[maybe_unused]] FileRequestHandle request)
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

        AZ::IO::IStreamerTypes::Deadline m_deadline;
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
        AZ_CLASS_ALLOCATOR(MockLoadAssetWithNonZeroSizeCatalogAndHandler, AZ::SystemAllocator)
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
        void GetDefaultAssetLoadPriority([[maybe_unused]] AssetType type, AZ::IO::IStreamerTypes::Deadline& defaultDeadline,
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

    // The AssetManagerStreamerImmediateCompletionTests class adjusts the asset loading to force it to complete immediately,
    // while still within the callstack for GetAsset().  This can be used to test various conditions in which the load thread
    // completes more rapidly than expected, and can expose subtle race conditions.
    // There are a few key things that this class does to make this work:
    // - The file I/O streamer is mocked
    // - The asset stream data is mocked to a 0-byte length for the asset so that the stream load will bypass the I/O streamer and
    //   just immediately return completion.
    // - The number of JobManager threads is set to 0, forcing jobs to execute synchronously inline when they are started.
    // With these changes, GetAssetInternal() will queue the stream, which will immediately call the callback that creates LoadAssetJob,
    // which immediately executes in-place to process the asset due to the synchronous JobManager.
    // Note that if we just created the asset in a Ready state, most of the asset loading code is completely bypassed, and so we
    // wouldn't be able to test for race conditions in the AssetContainer.
    // 
    // This class also unregisters the catalog and asset handler before shutting down the asset manager.  This is done to catch
    // any outstanding asset references that exist due to loads not completing and cleaning up successfully.
    struct AssetManagerStreamerImmediateCompletionTests : public BaseAssetManagerTest,
                                                          public AZ::Data::AssetCatalogRequestBus::Handler,
                                                          public AZ::Data::AssetHandler,
                                                          public AZ::Data::AssetCatalog
    {
        static inline const AZ::Uuid TestAssetId{"{E970B177-5F45-44EB-A2C4-9F29D9A0B2A2}"};
        static inline const AZ::Uuid MissingAssetId{"{11111111-1111-1111-1111-111111111111}"};
        static inline constexpr AZStd::string_view TestAssetPath = "test";

        void SetUp() override
        {
            BaseAssetManagerTest::SetUp();
            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            // Register the handler and catalog after creation, because we intend to destroy them before AssetManager destruction.
            // The specific asset we load is irrelevant, so register EmptyAsset.
            AZ::Data::AssetManager::Instance().RegisterHandler(this, AZ::AzTypeInfo<EmptyAsset>::Uuid());
            AZ::Data::AssetManager::Instance().RegisterCatalog(this, AZ::AzTypeInfo<EmptyAsset>::Uuid());

            // Intercept messages for finding assets by name so that we can mock out the asset we're loading.
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            // Unregister before destroying AssetManager.
            // This will catch any assets that got stuck in a loading state without getting cleaned up.
            AZ::Data::AssetManager::Instance().UnregisterCatalog(this);
            AZ::Data::AssetManager::Instance().UnregisterHandler(this);

            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();

            AssetManager::Destroy();
            BaseAssetManagerTest::TearDown();
        }

        size_t GetNumJobManagerThreads() const override
        {
            // Return 0 threads so that the Job Manager executes jobs synchronously inline.  This lets us finish a load while still
            // in the callstack that initiates the load.
            return 0;
        }

        // Create a mock streamer instead of a real one, since we don't really want to load an asset.
        IO::IStreamer* CreateStreamer() override
        {
            m_mockStreamer = AZStd::make_unique<StreamerWrapper>();
            return &(m_mockStreamer->m_mockStreamer);
        }

        void DestroyStreamer([[maybe_unused]] IO::IStreamer* streamer) override
        {
            m_mockStreamer = nullptr;
        }

        // AssetHandler implementation

        // Minimalist mock to create a new EmptyAsset with the desired asset ID.
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, [[maybe_unused]] const AZ::Data::AssetType& type) override
        {
            return new EmptyAsset(id);
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            delete ptr;
        }

        // The mocked-out Asset Catalog handles EmptyAsset types.
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(AZ::AzTypeInfo<EmptyAsset>::Uuid());
        }

        // This is a mocked-out load, so just immediately return completion without doing anything.
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            [[maybe_unused]] const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            [[maybe_unused]] AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            [[maybe_unused]] const AZ::Data::AssetFilterCB& assetLoadFilterCB) override
        {
            return AZ::Data::AssetHandler::LoadResult::LoadComplete;
        }

        // AssetCatalogRequestBus implementation
        
        // Minimalist mocks to provide our desired asset path or asset id
        AZStd::string GetAssetPathById(const AZ::Data::AssetId& id) override
        {
            if (id == TestAssetId)
            {
                return TestAssetPath;
            }

            return "";
        }

        AZ::Data::AssetId GetAssetIdByPath(
            const char* path, [[maybe_unused]] const AZ::Data::AssetType& typeToRegister,
            [[maybe_unused]] bool autoRegisterIfNotFound) override
        {
            if (path == TestAssetPath)
            {
                return TestAssetId;
            }

            return AZ::Data::AssetId();
        }

        // Return the mocked-out information for our test asset
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override
        {
            AZ::Data::AssetInfo assetInfo;

            if (id == TestAssetId)
            {
                assetInfo.m_assetId = TestAssetId;
                assetInfo.m_assetType = AZ::AzTypeInfo<EmptyAsset>::Uuid();
                assetInfo.m_relativePath = TestAssetPath;
            }

            return assetInfo;
        }

        // AssetCatalog implementation
        
        // Set the mocked-out asset load to have a 0-byte length so that the load skips I/O and immediately returns success
        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(
            const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            EXPECT_TRUE(type == AZ::AzTypeInfo<EmptyAsset>::Uuid());
            AZ::Data::AssetStreamInfo info;

            info.m_dataOffset = 0;
            info.m_dataLen = 0;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;

            if (id == TestAssetId)
            {
                info.m_streamName = TestAssetPath;
            }

            return info;
        }

        AZStd::unique_ptr<StreamerWrapper> m_mockStreamer;
    };

    // This test will verify that even if the asset loading stream/job returns immediately, all of the loading
    // code works successfully.  The test here is fairly simple - it just loads the asset and verifies that it
    // loaded successfully.  The bulk of the test is really in the setup class above, where the load is forced
    // to complete immediately.  Also, the true failure condition is caught in the setup class too, which is
    // the presence of any assets at the point that the asset handler is unregistered.  If they're present, then
    // the immediate load wasn't truly successful, as it left around extra references to the asset that haven't
    // been cleaned up.
    TEST_F(AssetManagerStreamerImmediateCompletionTests, LoadAssetWithImmediateJobCompletion_WorksSuccessfully)
    {
        AZ::Data::AssetLoadParameters loadParams;

        auto testAsset =
            AssetManager::Instance().GetAsset<EmptyAsset>(TestAssetId, AZ::Data::AssetLoadBehavior::Default, loadParams);

        AZ::Data::AssetManager::Instance().DispatchEvents();
        EXPECT_TRUE(testAsset.IsReady());
    }

    // This test verifies that even if the asset loading returns immediately with an error, all of the loading code works
    // successfully.  The test itself loads a missing asset twice - the first time is a non-immediate error, where the error
    // isn't reported until the DispatchEvents() call.  The second time is an immediate error, because now the asset is already
    // registered in an Error state.  If the test fails, it will likely get caught in the shutdown of the test class, if any
    // assets still exist at the point that the asset handler is unregistered.  If they're present, then handling of the immediate
    // error didn't work, as it left around extra references to the asset that haven't been cleaned up.
    TEST_F(AssetManagerStreamerImmediateCompletionTests, ImmediateAssetError_WorksSuccessfully)
    {
        AZ::Data::AssetLoadParameters loadParams;

        // Attempt to load a missing asset the first time.  It will get an error, but not until the DispatchEvents() call happens.
        auto testAsset1 = AssetManager::Instance().GetAsset<EmptyAsset>(MissingAssetId, AZ::Data::AssetLoadBehavior::Default, loadParams);
        AZ::Data::AssetManager::Instance().DispatchEvents();
        EXPECT_TRUE(testAsset1.IsError());

        // While the reference to the missing asset still exists, try to get it again.  This will cause a more immediate error in
        // the AssetContainer code, which should still get handled correctly.  In the failure condition, it will instead leave the
        // AssetContainer in a state where it never sends the final OnAssetContainerReady/Canceled message.
        auto testAsset2 = AssetManager::Instance().GetAsset<EmptyAsset>(MissingAssetId, AZ::Data::AssetLoadBehavior::Default, loadParams);
        AZ::Data::AssetManager::Instance().DispatchEvents();
        EXPECT_TRUE(testAsset2.IsError());
    }

} // namespace UnitTest
