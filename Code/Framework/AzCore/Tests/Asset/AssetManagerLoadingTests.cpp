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
#include <AzCore/Console/Console.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Uuid.h>
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
#include <Tests/Asset/TestAssetTypes.h>
#include <Tests/SerializeContextFixture.h>
#include <Tests/TestCatalog.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Data;

    struct OnAssetReadyListener
        : public AssetBus::Handler,
        public AssetLoadBus::Handler
    {
        using OnAssetReadyCheck = AZStd::function<bool(const OnAssetReadyListener&)>;
        OnAssetReadyListener(const AssetId& assetId, const AssetType& assetType, bool autoConnect = true)
            : m_assetId(assetId)
            , m_assetType(assetType)
        {
            if (autoConnect && m_assetId.IsValid())
            {
                AssetBus::Handler::BusConnect(m_assetId);
                AssetLoadBus::Handler::BusConnect(m_assetId);
            }
        }
        ~OnAssetReadyListener() override
        {
            m_assetId.SetInvalid();
            m_latest = {};
            AssetBus::Handler::BusDisconnect();
            AssetLoadBus::Handler::BusDisconnect();
        }
        void OnAssetReady(Asset<AssetData> asset) override
        {
            EXPECT_EQ(asset->GetId(), m_assetId);
            EXPECT_EQ(asset->GetType(), m_assetType);
            if (m_readyCheck)
            {
                EXPECT_EQ(m_readyCheck(*this), true);
            }
            m_ready++;
            m_latest = asset;
        }
        void OnAssetReloaded(Asset<AssetData> asset) override
        {
            EXPECT_EQ(asset->GetId(), m_assetId);
            EXPECT_EQ(asset->GetType(), m_assetType);
            m_reloaded++;
            m_latest = asset;
        }
        void OnAssetError(Asset<AssetData> asset) override
        {
            EXPECT_EQ(asset->GetId(), m_assetId);
            EXPECT_EQ(asset->GetType(), m_assetType);
            m_error++;
            m_latest = asset;
        }
        void OnAssetDataLoaded(Asset<AssetData> asset) override
        {
            EXPECT_EQ(asset->GetId(), m_assetId);
            EXPECT_EQ(asset->GetType(), m_assetType);
            m_dataLoaded++;
            m_latest = asset;
        }
        AssetId     m_assetId;
        AssetType   m_assetType;
        AZStd::atomic_int m_ready{};
        AZStd::atomic_int m_error{};
        AZStd::atomic_int m_reloaded{};
        AZStd::atomic_int m_dataLoaded{};
        Asset<AssetData> m_latest;
        OnAssetReadyCheck m_readyCheck;
    };

    struct ContainerReadyListener
        : public AssetBus::MultiHandler
    {
        ContainerReadyListener(const AssetId& assetId)
            : m_assetId(assetId)
        {
            BusConnect(assetId);
        }
        ~ContainerReadyListener() override
        {
            BusDisconnect();
        }
        void OnAssetContainerReady(Asset<AssetData> asset) override
        {
            EXPECT_EQ(asset->GetId(), m_assetId);
            m_ready++;

        }

        AssetId     m_assetId;
        AZStd::atomic_int m_ready{};
    };

    /**
    * Generate a situation where we have more dependent job loads than we have threads
    * to process them.
    * This will test the aspect of the system where ObjectStreams and asset jobs loading dependent
    * assets will do the work in their own thread.
    */
    class AssetJobsFloodTest
        : public BaseAssetManagerTest
    {
    public:
        TestAssetManager* m_testAssetManager{ nullptr };
        DataDrivenHandlerAndCatalog* m_assetHandlerAndCatalog{ nullptr };

        static inline const AZ::Uuid MyAsset1Id{ "{5B29FE2B-6B41-48C9-826A-C723951B0560}" };
        static inline const AZ::Uuid MyAsset2Id{ "{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}" };
        static inline const AZ::Uuid MyAsset3Id{ "{622C3FC9-5AE2-4E52-AFA2-5F7095ADAB53}" };
        static inline const AZ::Uuid MyAsset4Id{ "{EE99215B-7AB4-4757-B8AF-F78BD4903AC4}" };
        static inline const AZ::Uuid MyAsset5Id{ "{D9CDAB04-D206-431E-BDC0-1DD615D56197}" };
        static inline const AZ::Uuid MyAsset6Id{ "{B2F139C3-5032-4B52-ADCA-D52A8F88E043}" };


        static inline const AZ::Uuid DelayLoadAssetId{ "{5553A2B0-2401-4600-AE2F-4702A3288AB2}" };
        static inline const AZ::Uuid NoLoadAssetId{ "{E14BD18D-A933-4CBD-B64E-25F14D5E69E4}" };

        // A -> A
        static inline const AZ::Uuid CircularAId{ "{8FDEC8B3-AEB3-43AF-9D99-9DFF1BB59EA8}" };

        // B -> C -> B
        static inline const AZ::Uuid CircularBId{ "{ECB6EDBC-2FDA-42FF-9564-341A0B5F5249}" };
        static inline const AZ::Uuid CircularCId{ "{B86CF17A-779E-4679-BC0B-3C47446CF89F}" };

        // D -> B -> C -> B
        static inline const AZ::Uuid CircularDId{ "{1FE8342E-9DCE-4AA9-969A-3F3A3526E6CF}" };

        // Designed to test cases where a preload chain exists and one of the underlying assets
        // Can't be loaded either due to no asset info being found or no handler (The more likely case)
        static inline const AZ::Uuid PreloadBrokenDepAId{ "{E5ABB446-DB05-4413-9FE4-EA368F293A8F}" };
        static inline const AZ::Uuid PreloadBrokenDepBId{ "{828FF33A-D716-4DF8-AD6F-6BBC66F4CC8B}" };
        static inline const AZ::Uuid PreloadAssetNoDataId{ "{9F670DC8-0D89-4FA8-A1D5-B05AF7B04DBB}" };
        static inline const AZ::Uuid PreloadAssetNoHandlerId{ "{5F32A180-E380-45A2-89F8-C5CF53B53BDD}" };

        // Preload Tree has a root, PreloadA, and QueueLoadA
        // PreloadA has PreloadB and QueueLoadB
        // QueueLoadB has PreloadC and QueueLoadC
        static inline const AZ::Uuid PreloadAssetRootId{ "{A1C3C4EA-726E-4DA1-B783-5CB5032EED4C}" };
        static inline const AZ::Uuid PreloadAssetAId{ "{84795373-9A1F-44AA-9808-AF0BF67C8BD6}" };
        static inline const AZ::Uuid QueueLoadAssetAId{ "{A16A34C9-8CDC-44FC-9962-BE0192568FA2}" };
        static inline const AZ::Uuid PreloadAssetBId{ "{3F3745C1-3B9D-47BD-9993-7B61855E8FA0}" };
        static inline const AZ::Uuid QueueLoadAssetBId{ "{82A9B00E-BD2B-4EBF-AFDE-C2C30D1822C0}" };
        static inline const AZ::Uuid PreloadAssetCId{ "{8364CF95-C443-4A00-9BB4-DCD81E516769}" };
        static inline const AZ::Uuid QueueLoadAssetCId{ "{635E9E70-EBE1-493D-92AA-2E45E350D4F5}" };


        // Initialize the Job Manager with 2 threads for the Asset Manager to use.
        size_t GetNumJobManagerThreads() const override { return 2; }

        void SetUp() override
        {
            BaseAssetManagerTest::SetUp();
            SetupTest();
        }

        void TearDown() override
        {
            TearDownTest();
            AssetManager::Destroy();
            BaseAssetManagerTest::TearDown();
        }

        void SetupAssets()
        {
            auto* catalog = m_assetHandlerAndCatalog;

            catalog->AddAsset<AssetWithAssetReference>(MyAsset1Id, "TestAsset1.txt")->AddPreload(MyAsset4Id);
            catalog->AddAsset<AssetWithAssetReference>(MyAsset2Id, "TestAsset2.txt")->AddPreload(MyAsset5Id);
            catalog->AddAsset<AssetWithAssetReference>(MyAsset3Id, "TestAsset3.txt")->AddPreload(MyAsset6Id);

            catalog->AddAsset<AssetWithSerializedData>(MyAsset4Id, "TestAsset4.txt");
            catalog->AddAsset<AssetWithSerializedData>(MyAsset5Id, "TestAsset5.txt");
            catalog->AddAsset<AssetWithSerializedData>(MyAsset6Id, "TestAsset6.txt");

            catalog->AddAsset<AssetWithAssetReference>(DelayLoadAssetId, "DelayLoadAsset.txt", 2000);
            catalog->AddAsset<AssetWithAssetReference>(NoLoadAssetId, "NoLoadAsset.txt")->AddNoLoad(MyAsset2Id);

            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(PreloadAssetRootId, "PreLoadRoot.txt")->AddPreload(PreloadAssetAId)->AddQueueLoad(QueueLoadAssetAId);

            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(PreloadAssetAId, "PreLoadA.txt")->AddPreload(PreloadAssetBId)->AddQueueLoad(QueueLoadAssetBId);
            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(PreloadAssetBId, "PreLoadB.txt");
            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(QueueLoadAssetBId, "QueueLoadB.txt");

            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(QueueLoadAssetAId, "QueueLoadA.txt")->AddPreload(PreloadAssetCId)->AddQueueLoad(QueueLoadAssetCId);
            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(PreloadAssetCId, "PreLoadC.txt");
            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(QueueLoadAssetCId, "QueueLoadC.txt");

            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(PreloadBrokenDepAId, "PreLoadBrokenA.txt")->AddPreload(PreloadBrokenDepBId);
            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(PreloadBrokenDepBId, "PreLoadBrokenB.txt")->AddPreload(PreloadAssetNoDataId)->AddPreload(PreloadAssetNoHandlerId);
            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(PreloadAssetNoDataId, "PreLoadNoData.txt", 0, true);
            catalog->AddAsset<EmptyAssetWithNoHandler>(PreloadAssetNoHandlerId, "PreLoadNoHandler.txt", 0, false, true);

            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(CircularAId, "CircularA.txt")->AddPreload(CircularAId);
            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(CircularBId, "CircularB.txt")->AddPreload(CircularCId);
            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(CircularCId, "CircularC.txt")->AddPreload(CircularBId);
            catalog->AddAsset<AssetWithQueueAndPreLoadReferences>(CircularDId, "CircularD.txt")->AddPreload(CircularBId);
        }

        void SetupTest()
        {
            AssetWithSerializedData::Reflect(*m_serializeContext);
            AssetWithAssetReference::Reflect(*m_serializeContext);
            AssetWithQueueAndPreLoadReferences::Reflect(*m_serializeContext);

            AssetManager::Descriptor desc;
            m_testAssetManager = aznew TestAssetManager(desc);
            AssetManager::SetInstance(m_testAssetManager);
            m_assetHandlerAndCatalog = aznew DataDrivenHandlerAndCatalog();
            m_assetHandlerAndCatalog->m_context = m_serializeContext;

            SetupAssets();

            AZStd::vector<AssetType> types;
            m_assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                m_testAssetManager->RegisterHandler(m_assetHandlerAndCatalog, type);
                m_testAssetManager->RegisterCatalog(m_assetHandlerAndCatalog, type);
            }

            {
                AssetWithSerializedData ap1;
                AssetWithSerializedData ap2;
                AssetWithSerializedData ap3;

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &ap1, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset5.txt", AZ::DataStream::ST_XML, &ap2, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset6.txt", AZ::DataStream::ST_XML, &ap3, m_serializeContext));

                AssetWithAssetReference assetWithPreload1;
                AssetWithAssetReference assetWithPreload2;
                AssetWithAssetReference assetWithPreload3;
                AssetWithAssetReference delayedAsset;
                AssetWithAssetReference noLoadAsset;

                assetWithPreload1.m_asset = m_testAssetManager->CreateAsset<AssetWithSerializedData>(MyAsset4Id, AssetLoadBehavior::PreLoad);
                assetWithPreload2.m_asset = m_testAssetManager->CreateAsset<AssetWithSerializedData>(MyAsset5Id, AssetLoadBehavior::PreLoad);
                assetWithPreload3.m_asset = m_testAssetManager->CreateAsset<AssetWithSerializedData>(MyAsset6Id, AssetLoadBehavior::PreLoad);
                noLoadAsset.m_asset = m_testAssetManager->CreateAsset<AssetWithSerializedData>(MyAsset2Id, AssetLoadBehavior::NoLoad);
                EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 4);

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &assetWithPreload1, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &assetWithPreload2, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &assetWithPreload3, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "DelayLoadAsset.txt", AZ::DataStream::ST_XML, &delayedAsset, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "NoLoadAsset.txt", AZ::DataStream::ST_XML, &noLoadAsset, m_serializeContext));

                AssetWithQueueAndPreLoadReferences preLoadRoot;
                AssetWithQueueAndPreLoadReferences preLoadA;
                AssetWithQueueAndPreLoadReferences queueLoadA;
                AssetWithQueueAndPreLoadReferences preLoadBrokenA;
                AssetWithQueueAndPreLoadReferences preLoadBrokenB;
                // We don't need to set up the internal asset references for PreLoadB, QueueLoadB, PreLoadC, QueueLoadC, PreLoadNoData
                // so we'll just share the same empty asset to serialize out for those.
                AssetWithQueueAndPreLoadReferences noRefs;

                preLoadRoot.m_preLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(PreloadAssetAId, AssetLoadBehavior::PreLoad);
                preLoadRoot.m_queueLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(QueueLoadAssetAId, AssetLoadBehavior::QueueLoad);
                preLoadA.m_preLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(PreloadAssetBId, AssetLoadBehavior::PreLoad);
                preLoadA.m_queueLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(QueueLoadAssetBId, AssetLoadBehavior::QueueLoad);
                queueLoadA.m_preLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(PreloadAssetCId, AssetLoadBehavior::PreLoad);
                queueLoadA.m_queueLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(QueueLoadAssetCId, AssetLoadBehavior::QueueLoad);
                preLoadBrokenA.m_preLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(PreloadBrokenDepBId, AssetLoadBehavior::PreLoad);
                preLoadBrokenB.m_preLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(PreloadAssetNoDataId, AssetLoadBehavior::PreLoad);

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadRoot.txt", AZ::DataStream::ST_XML, &preLoadRoot, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadA.txt", AZ::DataStream::ST_XML, &preLoadA, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadB.txt", AZ::DataStream::ST_XML, &noRefs, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadC.txt", AZ::DataStream::ST_XML, &noRefs, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "QueueLoadA.txt", AZ::DataStream::ST_XML, &queueLoadA, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "QueueLoadB.txt", AZ::DataStream::ST_XML, &noRefs, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "QueueLoadC.txt", AZ::DataStream::ST_XML, &noRefs, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadBrokenA.txt", AZ::DataStream::ST_XML, &preLoadBrokenA, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadBrokenB.txt", AZ::DataStream::ST_XML, &preLoadBrokenB, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "PreLoadNoData.txt", AZ::DataStream::ST_XML, &noRefs, m_serializeContext));

                AssetWithQueueAndPreLoadReferences circularA;
                AssetWithQueueAndPreLoadReferences circularB;
                AssetWithQueueAndPreLoadReferences circularC;
                AssetWithQueueAndPreLoadReferences circularD;

                circularA.m_preLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(CircularAId, AssetLoadBehavior::PreLoad);
                circularB.m_preLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(CircularCId, AssetLoadBehavior::PreLoad);
                circularC.m_preLoad = m_testAssetManager->CreateAsset<AssetWithAssetReference>(CircularBId, AssetLoadBehavior::PreLoad);
                circularD.m_preLoad = circularC.m_preLoad;

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "CircularA.txt", AZ::DataStream::ST_XML, &circularA, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "CircularB.txt", AZ::DataStream::ST_XML, &circularB, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "CircularC.txt", AZ::DataStream::ST_XML, &circularC, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "CircularD.txt", AZ::DataStream::ST_XML, &circularD, m_serializeContext));
                m_assetHandlerAndCatalog->m_numCreations = 0;
            }
        }

        void TearDownTest()
        {
            DeleteAssetFromDisk(GetTestFolderPath() + "TestAsset4.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "TestAsset5.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "TestAsset6.txt");

            DeleteAssetFromDisk(GetTestFolderPath() + "TestAsset1.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "TestAsset2.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "TestAsset3.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "DelayLoadAsset.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "NoLoadAsset.txt");

            DeleteAssetFromDisk(GetTestFolderPath() + "PreLoadRoot.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "PreLoadA.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "PreLoadB.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "PreLoadC.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "QueueLoadA.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "QueueLoadB.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "QueueLoadC.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "PreLoadBrokenA.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "PreLoadBrokenB.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "PreLoadNoData.txt");

            DeleteAssetFromDisk(GetTestFolderPath() + "CircularA.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "CircularB.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "CircularC.txt");
            DeleteAssetFromDisk(GetTestFolderPath() + "CircularD.txt");
        }

        void CheckFinishedCreationsAndDestructions()
        {
            // Make sure asset jobs have finished before validating the number of destroyed assets, because it's possible that the asset job
            // still contains a reference on the job thread that won't trigger the asset destruction until the asset job is destroyed.
            BlockUntilAssetJobsAreComplete();
            m_testAssetManager->DispatchEvents();
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, m_assetHandlerAndCatalog->m_numDestructions);
        }

    };

    static constexpr AZStd::chrono::seconds MaxDispatchTimeoutSeconds = BaseAssetManagerTest::DefaultTimeoutSeconds * 12;
    
    template <typename Pred>
    bool DispatchEventsUntilCondition(AZ::Data::AssetManager& assetManager, Pred&& conditionPredicate,
        AZStd::chrono::seconds logIntervalSeconds = BaseAssetManagerTest::DefaultTimeoutSeconds,
        AZStd::chrono::seconds maxTimeoutSeconds = MaxDispatchTimeoutSeconds)
    {
        // If the Max Timeout is hit the test will be marked as a failure

        AZStd::chrono::time_point dispatchEventTimeStart = AZStd::chrono::system_clock::now();
        AZStd::chrono::seconds dispatchEventNextLogTime = logIntervalSeconds;

        while (!conditionPredicate())
        {
            AZStd::chrono::time_point currentTime = AZStd::chrono::system_clock::now();
            if (AZStd::chrono::seconds elapsedTime{ currentTime - dispatchEventTimeStart };
                elapsedTime >= dispatchEventNextLogTime)
            {
                const testing::TestInfo* test_info = ::testing::UnitTest::GetInstance()->current_test_info();
                AZ_Printf("AssetManagerLoadingTest", "The DispatchEventsUntiTimeout function has been waiting for %llu seconds"
                    " in test %s.%s", elapsedTime.count(), test_info->test_case_name(), test_info->name());
                // Update the next log time to be the next multiple of DefaultTimeout Seconds
                // after current elapsed time
                dispatchEventNextLogTime = elapsedTime + logIntervalSeconds - ((elapsedTime + logIntervalSeconds) % logIntervalSeconds);
                if (elapsedTime >= maxTimeoutSeconds)
                {
                    return false;
                }
            }
            assetManager.DispatchEvents();
            AZStd::this_thread::yield();
        }

        return true;
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_ASSET_MANAGER_FLOOD_TEST
    TEST_F(AssetJobsFloodTest, DISABLED_FloodTest)
#else
    TEST_F(AssetJobsFloodTest, FloodTest)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        const auto timeoutSeconds = AZStd::chrono::seconds(10);

        OnAssetReadyListener assetStatus1(MyAsset1Id, azrtti_typeid<AssetWithAssetReference>());
        OnAssetReadyListener assetStatus2(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>());
        OnAssetReadyListener assetStatus3(MyAsset3Id, azrtti_typeid<AssetWithAssetReference>());
        OnAssetReadyListener assetStatus4(MyAsset4Id, azrtti_typeid<AssetWithSerializedData>());
        OnAssetReadyListener assetStatus5(MyAsset5Id, azrtti_typeid<AssetWithSerializedData>());
        OnAssetReadyListener assetStatus6(MyAsset6Id, azrtti_typeid<AssetWithSerializedData>());

        // Suspend the streamer until we've got all the assets queued simultaneously to ensure that we've flooded the system.
        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
        streamer->SuspendProcessing();

        // Load all three root assets, each of which cascades another asset load as PreLoad.
        Asset<AssetWithAssetReference> asset1 = m_testAssetManager->GetAsset(MyAsset1Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
        Asset<AssetWithAssetReference> asset2 = m_testAssetManager->GetAsset(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
        Asset<AssetWithAssetReference> asset3 = m_testAssetManager->GetAsset(MyAsset3Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);

        // These two assets should be queued - this should not block the above assets which reference them from processing them
        // Due to the Queued status we check for below
        Data::Asset<AssetWithSerializedData> asset4Block = m_testAssetManager->GetAsset(MyAsset4Id, azrtti_typeid<AssetWithSerializedData>(), AZ::Data::AssetLoadBehavior::Default);
        Data::Asset<AssetWithSerializedData> asset5Block = m_testAssetManager->GetAsset(MyAsset5Id, azrtti_typeid<AssetWithSerializedData>(), AZ::Data::AssetLoadBehavior::Default);

        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 6);

        // Add a reload request - this should sit on the reload queue, however it should not be processed because the original job for asset3 has begun the
        // Load and assets in the m_assets map in loading state block reload.
        m_testAssetManager->ReloadAsset(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);

        // Now that all the initial requests are queued, start up the streamer.
        streamer->ResumeProcessing();

        // Make sure all 6 assets have loaded.
        asset1.BlockUntilLoadComplete();
        asset2.BlockUntilLoadComplete();
        asset3.BlockUntilLoadComplete();
        asset4Block.BlockUntilLoadComplete();
        asset5Block.BlockUntilLoadComplete();

        EXPECT_TRUE(asset1.IsReady());
        EXPECT_TRUE(asset2.IsReady());
        EXPECT_TRUE(asset3.IsReady());
        EXPECT_TRUE(asset1->m_asset.IsReady());
        EXPECT_TRUE(asset2->m_asset.IsReady());
        EXPECT_TRUE(asset3->m_asset.IsReady());
        EXPECT_TRUE(asset4Block.IsReady());
        EXPECT_TRUE(asset5Block.IsReady());
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 6);

        // Validate that asset 6 has already completed as well.
        Data::Asset<AssetWithSerializedData> asset6Block = m_testAssetManager->GetAsset(MyAsset6Id, azrtti_typeid<AssetWithSerializedData>(), AZ::Data::AssetLoadBehavior::Default);
        EXPECT_TRUE(asset6Block.IsReady());

        // Assets above can be ready (PreNotify) before the signal has reached our listener - allow for a small window to hear
        auto maxTimeout = AZStd::chrono::system_clock::now() + timeoutSeconds;
        bool timedOut = false;
        while (!assetStatus1.m_ready || !assetStatus2.m_ready || !assetStatus3.m_ready)
        {
            AssetBus::ExecuteQueuedEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                timedOut = true;
                break;
            }
        }

        // Make sure we didn't time out.
        ASSERT_FALSE(timedOut);

        EXPECT_EQ(assetStatus1.m_ready, 1);
        EXPECT_EQ(assetStatus2.m_ready, 1);
        EXPECT_EQ(assetStatus3.m_ready, 1);
        // MyAsset4 and 5 were loaded as a blocking dependency while a job was waiting to load them
        // we want to verify they did not go through a second load
        EXPECT_EQ(assetStatus4.m_ready, 1);
        EXPECT_EQ(assetStatus5.m_ready, 1);
        // MyAsset6 was loaded by MyAsset3.  Make sure it's ready too.
        EXPECT_EQ(assetStatus6.m_ready, 1);

        EXPECT_EQ(assetStatus1.m_reloaded, 0);
        EXPECT_EQ(assetStatus2.m_reloaded, 0);
        EXPECT_EQ(assetStatus3.m_reloaded, 0);
        EXPECT_EQ(assetStatus4.m_reloaded, 0);
        EXPECT_EQ(assetStatus5.m_reloaded, 0);
        EXPECT_EQ(assetStatus6.m_reloaded, 0);

        // This should process
        m_testAssetManager->ReloadAsset(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);
        // This should process
        m_testAssetManager->ReloadAsset(MyAsset2Id, AZ::Data::AssetLoadBehavior::Default);
        streamer->SuspendProcessing();
        // This should process but be queued
        m_testAssetManager->ReloadAsset(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);
        // This should get tossed because there's a reload request waiting
        m_testAssetManager->ReloadAsset(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);
        streamer->ResumeProcessing();

        // Allow our reloads to process and signal our listeners
        maxTimeout = AZStd::chrono::system_clock::now() + timeoutSeconds;
        while (!assetStatus1.m_reloaded || !assetStatus2.m_reloaded || !assetStatus3.m_reloaded)
        {
            m_testAssetManager->DispatchEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                timedOut = true;
                break;
            }
            AZStd::this_thread::yield();
        }

        // Make sure we didn't time out.
        ASSERT_FALSE(timedOut);

        EXPECT_EQ(assetStatus1.m_ready, 1);
        EXPECT_EQ(assetStatus2.m_ready, 1);
        EXPECT_EQ(assetStatus3.m_ready, 1);

        EXPECT_EQ(assetStatus1.m_reloaded, 1);
        EXPECT_EQ(assetStatus2.m_reloaded, 1);
        EXPECT_EQ(assetStatus3.m_reloaded, 1);
        EXPECT_EQ(assetStatus4.m_reloaded, 0);
        EXPECT_EQ(assetStatus5.m_reloaded, 0);
        EXPECT_EQ(assetStatus6.m_reloaded, 0);


        OnAssetReadyListener delayLoadAssetStatus(DelayLoadAssetId, azrtti_typeid<AssetWithAssetReference>());

        Data::Asset<AssetWithAssetReference> delayedAsset = m_testAssetManager->GetAsset(
            DelayLoadAssetId,
            azrtti_typeid<AssetWithAssetReference>(),
            AZ::Data::AssetLoadBehavior::Default);

        // this verifies that a reloading asset in "loading" state queues another load when it is complete
        maxTimeout = AZStd::chrono::system_clock::now() + timeoutSeconds;
        while (!delayLoadAssetStatus.m_ready)
        {
            AssetBus::ExecuteQueuedEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                timedOut = true;
                break;
            }
        }
        // Make sure we didn't time out.
        ASSERT_FALSE(timedOut);

        EXPECT_EQ(delayLoadAssetStatus.m_ready, 1);

        // This should go through to loading
        m_testAssetManager->ReloadAsset(DelayLoadAssetId, AZ::Data::AssetLoadBehavior::Default);
        maxTimeout = AZStd::chrono::system_clock::now() + timeoutSeconds;
        while (m_testAssetManager->GetReloadStatus(DelayLoadAssetId) != AZ::Data::AssetData::AssetStatus::Loading)
        {
            AssetBus::ExecuteQueuedEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                timedOut = true;
                break;
            }
        }
        // Make sure we didn't time out.
        ASSERT_FALSE(timedOut);


        // This should mark the first for an additional reload
        m_testAssetManager->ReloadAsset(DelayLoadAssetId, AZ::Data::AssetLoadBehavior::Default);
        // This should do nothing
        m_testAssetManager->ReloadAsset(DelayLoadAssetId, AZ::Data::AssetLoadBehavior::Default);

        maxTimeout = AZStd::chrono::system_clock::now() + timeoutSeconds;
        while (delayLoadAssetStatus.m_reloaded < 2)
        {
            m_testAssetManager->DispatchEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                timedOut = true;
                break;
            }
            AZStd::this_thread::yield();
        }
        // Make sure we didn't time out.
        ASSERT_FALSE(timedOut);

        EXPECT_EQ(delayLoadAssetStatus.m_ready, 1);

        // the initial reload and the marked reload should both have gone through
        EXPECT_EQ(delayLoadAssetStatus.m_reloaded, 2);

        // There should be no other reloads now.  This is the status of our requests to reload the
        // asset which isn't usually the same as the status of the base delayedAsset we're still holding
        AZ::Data::AssetData::AssetStatus curStatus = m_testAssetManager->GetReloadStatus(DelayLoadAssetId);
        // For our reload tests "NotLoaded" is equivalent to "None currently reloading in any status"
        AZ::Data::AssetData::AssetStatus expected_status = AZ::Data::AssetData::AssetStatus::NotLoaded;
        EXPECT_EQ(curStatus, expected_status);

        // Our base delayedAsset should still be Ready as we still hold the reference
        AZ::Data::AssetData::AssetStatus baseStatus = delayedAsset->GetStatus();
        AZ::Data::AssetData::AssetStatus expected_base_status = AZ::Data::AssetData::AssetStatus::Ready;
        EXPECT_EQ(baseStatus, expected_base_status);
    }

    struct DebugListener : AZ::Interface<IDebugAssetEvent>::Registrar
    {
        void AssetStatusUpdate(AZ::Data::AssetId id, AZ::Data::AssetData::AssetStatus status) override
        {
            AZ::Debug::Trace::Output(
                "", AZStd::string::format("Status %s - %d\n", id.ToString<AZStd::string>().c_str(), static_cast<int>(status)).c_str());
        }
        void ReleaseAsset(AZ::Data::AssetId id) override
        {
            AZ::Debug::Trace::Output(
                "", AZStd::string::format("Release %s\n", id.ToString<AZStd::string>().c_str()).c_str());
        }
    };

    TEST_F(AssetJobsFloodTest, RapidAcquireAndRelease)
    {
        DebugListener listener;  
        auto assetUuids = {
            MyAsset1Id,
            MyAsset2Id,
            MyAsset3Id,
        };

        AZStd::vector<AZStd::thread> threads;
        AZStd::mutex mutex;
        AZStd::atomic<int> threadCount(static_cast<int>(assetUuids.size()));
        AZStd::condition_variable cv;
        AZStd::atomic_bool keepDispatching(true);

        auto dispatch = [&keepDispatching]() {
            while (keepDispatching)
            {
                AssetManager::Instance().DispatchEvents();
            }
        };

        AZStd::thread dispatchThread(dispatch);

        for (const auto& assetUuid : assetUuids)
        {
            threads.emplace_back([this, &threadCount, &cv, assetUuid]() {
                bool checkLoaded = true;

                for (int i = 0; i < 5000; i++)
                {
                    Asset<AssetWithAssetReference> asset1 =
                        m_testAssetManager->GetAsset(assetUuid, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::PreLoad);
                    
                    if (checkLoaded)
                    {
                        asset1.BlockUntilLoadComplete();

                        EXPECT_TRUE(asset1.IsReady()) << "Iteration " << i << " failed.  Asset status: " <<  static_cast<int>(asset1.GetStatus());
                    }

                    checkLoaded = !checkLoaded;
                }

                --threadCount;
                cv.notify_one();
            });
        }

        bool timedOut = false;

        // Used to detect a deadlock.  If we wait for more than 5 seconds, it's likely a deadlock has occurred
        while (threadCount > 0 && !timedOut)
        {
            AZStd::unique_lock<AZStd::mutex> lock(mutex);
            timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds * 20000));
        }

        ASSERT_EQ(threadCount, 0) << "Thread count is non-zero, a thread has likely deadlocked.  Test will not shut down cleanly.";

        for (auto& thread : threads)
        {
            thread.join();
        }

        keepDispatching = false;
        dispatchThread.join();

        AssetManager::Destroy();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_AssetLoadBehaviorIsPreserved)
#else
    TEST_F(AssetJobsFloodTest, AssetLoadBehaviorIsPreserved)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        auto asset = AZ::Data::AssetManager::Instance().GetAsset<AssetWithAssetReference>(MyAsset1Id, AZ::Data::AssetLoadBehavior::PreLoad);

        asset.BlockUntilLoadComplete();

        EXPECT_EQ(asset.GetAutoLoadBehavior(), AZ::Data::AssetLoadBehavior::PreLoad);
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_BlockOnTheSameAsset_DoesNotDeadlock)
#else
    TEST_F(AssetJobsFloodTest, BlockOnTheSameAsset_DoesNotDeadlock)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        // This test is meant to ensure we don't get into a deadlock in the following situation:
        // Asset A is loaded and ready notification is queued for dispatch
        // Asset B is requested and we block waiting for it
        // While waiting, we dispatch events
        // Dispatched event leads to another blocking load of asset B

        // Setup
        constexpr char AssetNoRefA[] = "{EC5E3E4F-518C-4B03-A8BF-C9966CF763EB}";
        constexpr char AssetNoRefB[] = "{C07E55B5-E60C-4575-AE07-32DD3DC68B1A}";

        {
            m_assetHandlerAndCatalog->AddAsset<AssetWithSerializedData>(AssetNoRefA, "a.txt");
            m_assetHandlerAndCatalog->AddAsset<AssetWithSerializedData>(AssetNoRefB, "b.txt", 10);

            AssetWithSerializedData ap;

            EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "a.txt", AZ::DataStream::ST_XML, &ap, m_serializeContext));
            EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "b.txt", AZ::DataStream::ST_XML, &ap, m_serializeContext));
        }

        auto& assetManager = AssetManager::Instance();

        AssetBusCallbacks callbacks{};
        AZ_PUSH_DISABLE_WARNING(5233, "-Wunknown-warning-option") // Older versions of MSVC toolchain require to pass constexpr in the
                                                                  // capture. Newer versions issue unused warning
        callbacks.SetOnAssetReadyCallback([&, AssetNoRefB](const Asset<AssetData>&, AssetBusCallbacks&)
        AZ_POP_DISABLE_WARNING
            {
                // This callback should run inside the "main thread" dispatch events loop
                auto loadAsset = assetManager.GetAsset<AssetWithSerializedData>(AZ::Uuid(AssetNoRefB), AssetLoadBehavior::Default);
                loadAsset.BlockUntilLoadComplete();

                EXPECT_TRUE(loadAsset.IsReady());
            });

        callbacks.BusConnect(AssetId(AZ::Uuid(AssetNoRefA)));

        auto assetNoRefA = assetManager.GetAsset<AssetWithSerializedData>(AZ::Uuid(AssetNoRefA), AssetLoadBehavior::Default);

        // Don't call BlockUntilReady because that can call dispatch events, we need this asset to be in the PreNotify state
        while (assetNoRefA.GetStatus() != AssetData::AssetStatus::ReadyPreNotify)
        {
            AZStd::this_thread::yield();
        }

        EXPECT_EQ(assetNoRefA.GetStatus(), AssetData::AssetStatus::ReadyPreNotify);

        // Suspend the streamer until after we start blocking so we have time to get into the dispatch events call
        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
        streamer->SuspendProcessing();

        auto assetRefA = assetManager.GetAsset<AssetWithSerializedData>(AZ::Uuid(AssetNoRefB), AssetLoadBehavior::Default);

        AZStd::binary_semaphore completeSignal;

        AZStd::thread thread([streamer, &completeSignal]()
            {
                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(30));
                streamer->ResumeProcessing();

                if (!completeSignal.try_acquire_for(AZStd::chrono::seconds(5)))
                {
                    FAIL() << "Deadlock detected";
                }
            });

        assetRefA.BlockUntilLoadComplete();
        completeSignal.release();

        EXPECT_TRUE(assetNoRefA.IsReady());
        EXPECT_EQ(assetRefA.GetStatus(), AssetData::AssetStatus::Ready);

        thread.join();
        callbacks.BusDisconnect();
    }

    /**
    * Verify that loads without using the Asset Container still work correctly
    */
    class AssetContainerDisableTest
        : public BaseAssetManagerTest
    {
    public:
        static inline const AZ::Uuid MyAsset1Id{ "{5B29FE2B-6B41-48C9-826A-C723951B0560}" };
        static inline const AZ::Uuid MyAsset2Id{ "{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}" };
        static inline const AZ::Uuid MyAsset3Id{ "{622C3FC9-5AE2-4E52-AFA2-5F7095ADAB53}" };
        static inline const AZ::Uuid MyAsset4Id{ "{EE99215B-7AB4-4757-B8AF-F78BD4903AC4}" };
        static inline const AZ::Uuid MyAsset5Id{ "{D9CDAB04-D206-431E-BDC0-1DD615D56197}" };
        static inline const AZ::Uuid MyAsset6Id{ "{B2F139C3-5032-4B52-ADCA-D52A8F88E043}" };

        TestAssetManager* m_testAssetManager{ nullptr };
        DataDrivenHandlerAndCatalog* m_assetHandlerAndCatalog{ nullptr };
        LoadAssetDataSynchronizer m_loadDataSynchronizer;

        // Initialize the Job Manager with 2 threads for the Asset Manager to use.
        size_t GetNumJobManagerThreads() const override { return 2; }

        void SetUp() override
        {
            BaseAssetManagerTest::SetUp();
            SetupTest();

        }

        void TearDown() override
        {
            AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);
            delete m_assetHandlerAndCatalog;
            AssetManager::Destroy();
            BaseAssetManagerTest::TearDown();
        }

        void SetupAssets()
        {
            auto* catalog = m_assetHandlerAndCatalog;

            catalog->AddAsset<AssetWithAssetReference>(MyAsset1Id, "TestAsset1.txt", 1000, false, false, &m_loadDataSynchronizer)->AddPreload(MyAsset4Id);
            catalog->AddAsset<AssetWithAssetReference>(MyAsset2Id, "TestAsset2.txt", 1000, false, false, &m_loadDataSynchronizer)->AddPreload(MyAsset5Id);
            catalog->AddAsset<AssetWithAssetReference>(MyAsset3Id, "TestAsset3.txt", 1000, false, false, &m_loadDataSynchronizer)->AddPreload(MyAsset6Id);

            catalog->AddAsset<AssetWithSerializedData>(MyAsset4Id, "TestAsset4.txt", 1000);
            catalog->AddAsset<AssetWithSerializedData>(MyAsset5Id, "TestAsset5.txt", 1000);
            catalog->AddAsset<AssetWithSerializedData>(MyAsset6Id, "TestAsset6.txt", 1000);
        }

        void SetupTest()
        {
            AssetWithSerializedData::Reflect(*m_serializeContext);
            AssetWithAssetReference::Reflect(*m_serializeContext);

            AssetManager::Descriptor desc;
            m_testAssetManager = aznew TestAssetManager(desc);
            AssetManager::SetInstance(m_testAssetManager);
            m_assetHandlerAndCatalog = aznew DataDrivenHandlerAndCatalog();
            m_assetHandlerAndCatalog->m_context = m_serializeContext;

            SetupAssets();

            AZStd::vector<AssetType> types;
            m_assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                m_testAssetManager->RegisterHandler(m_assetHandlerAndCatalog, type);
                m_testAssetManager->RegisterCatalog(m_assetHandlerAndCatalog, type);
            }

            {
                AssetWithSerializedData ap1;
                AssetWithSerializedData ap2;
                AssetWithSerializedData ap3;

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &ap1, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset5.txt", AZ::DataStream::ST_XML, &ap2, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset6.txt", AZ::DataStream::ST_XML, &ap3, m_serializeContext));

                AssetWithAssetReference assetWithPreload1;
                AssetWithAssetReference assetWithPreload2;
                AssetWithAssetReference assetWithPreload3;

                assetWithPreload1.m_asset = m_testAssetManager->CreateAsset<AssetWithSerializedData>(MyAsset4Id, AssetLoadBehavior::PreLoad);
                assetWithPreload2.m_asset = m_testAssetManager->CreateAsset<AssetWithSerializedData>(MyAsset5Id, AssetLoadBehavior::PreLoad);
                assetWithPreload3.m_asset = m_testAssetManager->CreateAsset<AssetWithSerializedData>(MyAsset6Id, AssetLoadBehavior::PreLoad);
                EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 3);

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &assetWithPreload1, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &assetWithPreload2, m_serializeContext));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &assetWithPreload3, m_serializeContext));

                m_assetHandlerAndCatalog->m_numCreations = 0;
            }
        }
    };

    // Test is currently disabled.  Without loading using containers this uses blocking waits in asset load jobs which
    // have a race condition deadlock.  Details in SPEC-5061
#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_ASSETCONTAINERDISABLETEST
    TEST_F(AssetContainerDisableTest, DISABLED_AssetContainerDisableTest_LoadMoreAssetsThanJobThreads_LoadSucceeds)
#else
    TEST_F(AssetContainerDisableTest, DISABLED_AssetContainerDisableTest_LoadMoreAssetsThanJobThreads_LoadSucceeds)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_testAssetManager->SetParallelDependentLoadingEnabled(false);

        OnAssetReadyListener assetStatus1(MyAsset1Id, azrtti_typeid<AssetWithAssetReference>());
        OnAssetReadyListener assetStatus2(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>());
        OnAssetReadyListener assetStatus3(MyAsset3Id, azrtti_typeid<AssetWithAssetReference>());
        OnAssetReadyListener assetStatus4(MyAsset4Id, azrtti_typeid<AssetWithSerializedData>());
        OnAssetReadyListener assetStatus5(MyAsset5Id, azrtti_typeid<AssetWithSerializedData>());
        OnAssetReadyListener assetStatus6(MyAsset6Id, azrtti_typeid<AssetWithSerializedData>());

        // Load all three root assets, each of which cascades another asset load with a PreLoad dependency.
        Asset<AssetWithAssetReference> asset1 = m_testAssetManager->GetAsset(MyAsset1Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
        Asset<AssetWithAssetReference> asset2 = m_testAssetManager->GetAsset(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
        Asset<AssetWithAssetReference> asset3 = m_testAssetManager->GetAsset(MyAsset3Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);

        // There should be at least 3 asset creations, but potentially could have up to 6 depending on how quickly the
        // loads process.
        EXPECT_GE(m_assetHandlerAndCatalog->m_numCreations, 3);
        EXPECT_LE(m_assetHandlerAndCatalog->m_numCreations, 6);

        while (m_loadDataSynchronizer.m_numBlocking < 2)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));
        }

        m_loadDataSynchronizer.m_readyToLoad = true;
        m_loadDataSynchronizer.m_condition.notify_all();

        asset3.BlockUntilLoadComplete();
        asset2.BlockUntilLoadComplete();
        asset1.BlockUntilLoadComplete();

        EXPECT_TRUE(asset1.IsReady());
        EXPECT_TRUE(asset2.IsReady());
        EXPECT_TRUE(asset3.IsReady());
        EXPECT_TRUE(asset1->m_asset.IsReady());
        EXPECT_TRUE(asset2->m_asset.IsReady());
        EXPECT_TRUE(asset3->m_asset.IsReady());

        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 6);

        // Assets above can be ready (PreNotify) before the signal has reached our listener - allow for a small window to hear
        auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;
        while (!assetStatus1.m_ready || !assetStatus2.m_ready || !assetStatus3.m_ready)
        {
            AssetBus::ExecuteQueuedEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                AZ_Assert(false, "Timeout reached.");
                break;
            }
        }

        EXPECT_EQ(assetStatus1.m_ready, 1);
        EXPECT_EQ(assetStatus2.m_ready, 1);
        EXPECT_EQ(assetStatus3.m_ready, 1);
        // MyAsset4, MyAsset5, and MyAsset6 should have been loaded as blocking PreLoad dependent assets.
        EXPECT_EQ(assetStatus4.m_ready, 1);
        EXPECT_EQ(assetStatus5.m_ready, 1);
        EXPECT_EQ(assetStatus6.m_ready, 1);

        m_testAssetManager->SetParallelDependentLoadingEnabled(true);
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_FAILED_ASSET_LOAD_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_LoadTest_SameAsset_DifferentFilters)
#else
    TEST_F(AssetJobsFloodTest, LoadTest_SameAsset_DifferentFilters)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_FAILED_ASSET_LOAD_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();

        // Queue a load of PreloadAssetA, which will trigger the loads of PreloadAssetAId and its dependents (PreLoadAssetB, QueueLoadAssetB)
        Data::Asset<AssetWithQueueAndPreLoadReferences> asset2 = m_testAssetManager->GetAsset(PreloadAssetAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(), AZ::Data::AssetLoadBehavior::Default);
        // Next queue a load of PreloadAssetA again, but with a "no load" filter to specifically NOT load the dependents.
        Data::Asset<AssetWithQueueAndPreLoadReferences> asset1 = m_testAssetManager->GetAsset(PreloadAssetAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(), AZ::Data::AssetLoadBehavior::Default, AssetLoadParameters(&AssetFilterNoAssetLoading));

        // Wait for asset2 to be ready, as this means that the dependents are ready as well.
        while (!asset2.IsReady())
        {
            m_testAssetManager->DispatchEvents();
            AZStd::this_thread::yield();
        }

        // Even though asset1 filtered out the loads, it will still find the in-process load of PreLoadAssetB and get a reference to it.
        // Since asset2 completed, this means that both asset1 and asset2 should have valid, Ready references to PreLoadAssetB.
        EXPECT_TRUE(asset1->m_preLoad.IsReady());
        EXPECT_TRUE(asset2->m_preLoad.IsReady());

        bool stillWaiting;

        do
        {
            stillWaiting = false;
            auto&& containers = m_testAssetManager->GetAssetContainers();

            for (const auto& [container, containerSp] : containers)
            {
                if (!container->IsReady())
                {
                    m_testAssetManager->DispatchEvents();
                    AZStd::this_thread::yield();
                    stillWaiting = true;
                    break;
                }
            }
            // If any assets were ReadyPreNotify
            m_testAssetManager->DispatchEvents();
            if (!stillWaiting)
            {
                EXPECT_EQ(containers.size(), 0);
            }
        } while (stillWaiting);

        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_LoadTest_NoLoadDependentFilter_PreventsDependentsFromLoading)
#else
    TEST_F(AssetJobsFloodTest, LoadTest_NoLoadDependentFilter_PreventsDependentsFromLoading)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();

        // Queue a load of PreloadAssetA with a "no load" filter to specifically NOT load the dependents.
        Data::Asset<AssetWithQueueAndPreLoadReferences> asset1 = m_testAssetManager->GetAsset(
            PreloadAssetAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(),
            AZ::Data::AssetLoadBehavior::Default, AssetLoadParameters(&AssetFilterNoAssetLoading));

        // Wait for asset1 to be ready.
        while (!asset1.IsReady())
        {
            m_testAssetManager->DispatchEvents();
            AZStd::this_thread::yield();
        }

        // The dependent assets shouldn't be loaded for asset1 due to the filter.
        EXPECT_EQ(asset1->m_preLoad.GetStatus(), AssetData::AssetStatus::NotLoaded);
        EXPECT_EQ(asset1->m_queueLoad.GetStatus(), AssetData::AssetStatus::NotLoaded);

        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_NoDependencies_CanLoadAsContainer)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_NoDependencies_CanLoadAsContainer)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(PreloadAssetBId);
            OnAssetReadyListener preloadBListener(PreloadAssetBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            auto asset = AssetManager::Instance().FindOrCreateAsset(PreloadAssetBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(),
                AZ::Data::AssetLoadBehavior::Default);

            auto containerReady = m_testAssetManager->GetAssetContainer(asset);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready || !preloadBListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 0);
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 0);
            EXPECT_EQ(preloadBListener.m_ready, 1);
            EXPECT_EQ(preloadBListener.m_dataLoaded, 0);
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }



    TEST_F(AssetJobsFloodTest, DISABLED_ContainerCoreTest_BasicDependencyManagement_Success)
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;

        const AZ::u32 NumTestAssets = 3;
        const AZ::u32 AssetsPerContainer = 2;
        {
            // Load all three root assets, each of which cascades another asset load.
            auto asset1 = m_testAssetManager->FindOrCreateAsset(MyAsset1Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            auto asset2 = m_testAssetManager->FindOrCreateAsset(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            auto asset3 = m_testAssetManager->FindOrCreateAsset(MyAsset3Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);

            ContainerReadyListener readyListener1(MyAsset1Id);
            ContainerReadyListener readyListener2(MyAsset2Id);
            ContainerReadyListener readyListener3(MyAsset3Id);

            auto asset1Container = m_testAssetManager->GetAssetContainer(asset1);
            auto asset2Container = m_testAssetManager->GetAssetContainer(asset2);
            auto asset3Container = m_testAssetManager->GetAssetContainer(asset3);
            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener1.m_ready || !readyListener2.m_ready || !readyListener3.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(m_testAssetManager->GetRemainingJobs(), 0);

            EXPECT_EQ(asset1Container->IsReady(), true);
            EXPECT_EQ(asset2Container->IsReady(), true);
            EXPECT_EQ(asset3Container->IsReady(), true);

            EXPECT_EQ(m_testAssetManager->GetRemainingJobs(), 0);

            auto rootAsset = asset1Container->GetRootAsset();
            EXPECT_EQ(rootAsset->GetId(), MyAsset1Id);
            EXPECT_EQ(rootAsset->GetType(), azrtti_typeid<AssetWithAssetReference>());

            rootAsset = asset2Container->GetRootAsset();
            EXPECT_EQ(rootAsset->GetId(), MyAsset2Id);
            EXPECT_EQ(rootAsset->GetType(), azrtti_typeid<AssetWithAssetReference>());

            rootAsset = asset3Container->GetRootAsset();
            EXPECT_EQ(rootAsset->GetId(), MyAsset3Id);
            EXPECT_EQ(rootAsset->GetType(), azrtti_typeid<AssetWithAssetReference>());

            rootAsset = {};
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numDestructions, 0);

            EXPECT_EQ(asset1Container->IsReady(), true);
            EXPECT_EQ(asset2Container->IsReady(), true);
            EXPECT_EQ(asset3Container->IsReady(), true);

            EXPECT_EQ(asset1Container->GetDependencies().size(), 1);
            EXPECT_EQ(asset2Container->GetDependencies().size(), 1);
            EXPECT_EQ(asset3Container->GetDependencies().size(), 1);

            auto asset1CopyContainer = m_testAssetManager->GetAssetContainer(asset1);

            EXPECT_EQ(asset1CopyContainer->IsReady(), true);

            // We've now created the dependencies for each asset in the container as well
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, NumTestAssets * AssetsPerContainer);
            EXPECT_EQ(m_testAssetManager->GetRemainingJobs(), 0);
            EXPECT_EQ(asset1CopyContainer->GetDependencies().size(), 1);

            asset1Container = {};

            EXPECT_EQ(asset1CopyContainer->IsReady(), true);
            EXPECT_EQ(asset1CopyContainer->GetDependencies().size(), 1);

            asset1CopyContainer = {};
            asset1 = {};
            // We've released the references for one asset and its dependency
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numDestructions, AssetsPerContainer);
            asset1 = m_testAssetManager->FindOrCreateAsset(MyAsset1Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            asset1Container = m_testAssetManager->GetAssetContainer(asset1);

            maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!asset1Container->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            EXPECT_EQ(asset1Container->IsReady(), true);
        }

        CheckFinishedCreationsAndDestructions();

        // We created each asset and its dependency and recreated one pair
        EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, (NumTestAssets + 1) * AssetsPerContainer);

        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerFilterTest_ContainersWithAndWithoutFiltering_Success)
#else
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerFilterTest_ContainersWithAndWithoutFiltering_Success)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            AZ::Data::AssetFilterCB filterNone = [](const AZ::Data::AssetFilterInfo&) { return true; };
            auto asset1 = m_testAssetManager->FindOrCreateAsset(MyAsset1Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            auto asset1Container = m_testAssetManager->GetAssetContainer(asset1, AssetLoadParameters{ filterNone });

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!asset1Container->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset1Container->IsReady(), true);
            EXPECT_EQ(asset1Container->GetDependencies().size(), 1);

            asset1 = {};
            asset1Container = {};

            AZ::Data::AssetFilterCB noDependencyCB = [](const AZ::Data::AssetFilterInfo& filterInfo) { return filterInfo.m_assetType != azrtti_typeid<AssetWithSerializedData>(); };
            asset1 = m_testAssetManager->FindOrCreateAsset(MyAsset1Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            asset1Container = m_testAssetManager->GetAssetContainer(asset1, AssetLoadParameters{ noDependencyCB });
            maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;
            while (!asset1Container->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset1Container->IsReady(), true);
            EXPECT_EQ(asset1Container->GetDependencies().size(), 0);
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerNotificationTest_ListenForAssetReady_OnlyHearCorrectAsset)
#else
    TEST_F(AssetJobsFloodTest, ContainerNotificationTest_ListenForAssetReady_OnlyHearCorrectAsset)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener1(MyAsset1Id);
            ContainerReadyListener readyListener2(MyAsset2Id);

            auto asset1 = m_testAssetManager->FindOrCreateAsset(MyAsset1Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            auto asset1Container = m_testAssetManager->GetAssetContainer(asset1);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener1.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset1Container->IsReady(), true);
            EXPECT_EQ(asset1Container->GetDependencies().size(), 1);

            // MyAsset2 should not have signalled
            EXPECT_EQ(readyListener2.m_ready, 0);

            auto asset2 = m_testAssetManager->FindOrCreateAsset(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            auto asset2Container = m_testAssetManager->GetAssetContainer(asset2);

            maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!asset2Container->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            m_testAssetManager->DispatchEvents();
            EXPECT_EQ(asset2Container->IsReady(), true);
            EXPECT_EQ(asset2Container->GetDependencies().size(), 1);
            auto readyVal = readyListener2.m_ready.load();

            auto asset2ContainerCopy = m_testAssetManager->GetAssetContainer(asset2);

            // Should be ready immediately
            EXPECT_EQ(asset2ContainerCopy->IsReady(), true);
            EXPECT_EQ(asset2ContainerCopy->GetDependencies().size(), 1);
            // Copy shouldn't have signaled because it was ready to begin with
            EXPECT_EQ(readyListener2.m_ready, readyVal);
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_FAILED_ASSET_LOAD_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_AssetWithNoLoadReference_LoadDependencies_NoLoadNotLoaded)
#else
    TEST_F(AssetJobsFloodTest, AssetWithNoLoadReference_LoadDependencies_NoLoadNotLoaded)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_FAILED_ASSET_LOAD_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(NoLoadAssetId);

            // noLoad should only load itself, its dependency shouldn't be loaded but it should know about it
            auto noLoadRef = m_testAssetManager->FindOrCreateAsset(NoLoadAssetId, azrtti_typeid<AssetWithAssetReference>(),
                AZ::Data::AssetLoadBehavior::Default);
            auto noLoadRefContainer = m_testAssetManager->GetAssetContainer(noLoadRef);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(readyListener.m_ready, 1);
            EXPECT_EQ(noLoadRefContainer->IsReady(), true);
            EXPECT_EQ(noLoadRefContainer->GetDependencies().size(), 0);
            // Asset2 should be registered as a noload dependency
            EXPECT_EQ(noLoadRefContainer->GetUnloadedDependencies().size(), 1);
            EXPECT_NE(noLoadRefContainer->GetUnloadedDependencies().find(MyAsset2Id), noLoadRefContainer->GetUnloadedDependencies().end());
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_FAILED_ASSET_LOAD_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_AssetWithNoLoadReference_LoadContainerDependencies_LoadAllLoadsNoLoad)
#else
    TEST_F(AssetJobsFloodTest, AssetWithNoLoadReference_LoadContainerDependencies_LoadAllLoadsNoLoad)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_FAILED_ASSET_LOAD_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(NoLoadAssetId);

            // noLoad should only load itself, its dependency shouldn't be loaded but it should know about it
            auto noLoadRef = m_testAssetManager->FindOrCreateAsset(NoLoadAssetId, azrtti_typeid<AssetWithAssetReference>(),
                AZ::Data::AssetLoadBehavior::Default);
            auto noLoadRefContainer = m_testAssetManager->GetAssetContainer(noLoadRef, AssetLoadParameters(nullptr, AZ::Data::AssetDependencyLoadRules::LoadAll));

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(readyListener.m_ready, 1);
            EXPECT_EQ(noLoadRefContainer->IsReady(), true);
            EXPECT_EQ(noLoadRefContainer->GetDependencies().size(), 2);
            EXPECT_EQ(noLoadRefContainer->GetUnloadedDependencies().size(), 0);
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_FAILED_ASSET_LOAD_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_AssetWithNoLoadReference_LoadDependencies_BehaviorObeyed)
#else
    TEST_F(AssetJobsFloodTest, AssetWithNoLoadReference_LoadDependencies_BehaviorObeyed)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_FAILED_ASSET_LOAD_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener containerLoadingCompleteListener(NoLoadAssetId);
            OnAssetReadyListener readyListener(NoLoadAssetId, azrtti_typeid<AssetWithAssetReference>());
            OnAssetReadyListener dependencyListener(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>());

            SCOPED_TRACE("LoadDependencies_BehaviorObeyed");

            auto AssetOnlyReady = [&readyListener]() -> bool
            {
                return readyListener.m_ready;
            };
            auto AssetAndDependencyReady = [&readyListener, &dependencyListener]() -> bool
            {
                return readyListener.m_ready && dependencyListener.m_ready;
            };
            auto AssetContainerReady = [&containerLoadingCompleteListener]() -> bool
            {
                return containerLoadingCompleteListener.m_ready;
            };

            auto noLoadRef = m_testAssetManager->GetAsset(NoLoadAssetId, azrtti_typeid<AssetWithAssetReference>(),
                AZ::Data::AssetLoadBehavior::Default);

            // Dispatch AssetBus events until the NoLoadAssetId has signaled an OnAssetReady
            // event or the timeout has been reached
            EXPECT_TRUE(DispatchEventsUntilCondition(*m_testAssetManager, AssetOnlyReady))
                << "The DispatchEventsUntiTimeout function has not completed in "
                << MaxDispatchTimeoutSeconds.count() << " seconds. The test will be marked as a failure\n";

            // Dispatch AssetBus events until the asset container used to load
            // NoLoadAssetId has signaled an OnAssetContainerReady event
            // or the timeout has been reached
            // Wait until the current asset container has finished loading the NoLoadAssetId
            // before trigger another load
            // If the wait does not occur here, most likely what would occur is
            // the AssetManager::m_ownedAssetContainers object is still loading the NoLoadAssetId
            // using the default AssetLoadParameters
            // If a call to GetAsset occurs at this point while the Asset is still loading
            // it will ignore the new loadParams below and instead just re-use the existing
            // AssetContainerReader instance, resulting in the dependent MyAsset2Id not
            // being loaded
            // The function that can return an existing AssetContainer instance is the
            // AssetManager::GetAssetContainer. Since it can be in the middle of a load,
            // updating the AssetLoadParams would have an effect on the current in progress
            // load
            EXPECT_TRUE(DispatchEventsUntilCondition(*m_testAssetManager, AssetContainerReady))
                << "The DispatchEventsUntiTimeout function has not completed in "
                << MaxDispatchTimeoutSeconds.count() << " seconds. The test will be marked as a failure\n";

            // Reset the ContainerLoadingComplete ready status back to 0
            containerLoadingCompleteListener.m_ready = 0;

            AZ::Data::AssetLoadParameters loadParams(nullptr, AZ::Data::AssetDependencyLoadRules::LoadAll);
            loadParams.m_reloadMissingDependencies = true;
            auto loadDependencyRef = m_testAssetManager->GetAsset(NoLoadAssetId, azrtti_typeid<AssetWithAssetReference>(),
                AZ::Data::AssetLoadBehavior::Default, loadParams);

            // Dispatch AssetBus events until the NoLoadAssetId and the MyAsset2Id has signaled
            // an OnAssetReady event or the timeout has been reached
            EXPECT_TRUE(DispatchEventsUntilCondition(*m_testAssetManager, AssetAndDependencyReady))
                << "The DispatchEventsUntiTimeout function has not completed in "
                << MaxDispatchTimeoutSeconds.count() << " seconds. The test will be marked as a failure\n";

            EXPECT_EQ(readyListener.m_ready, 1);
            EXPECT_EQ(dependencyListener.m_ready, 1);

            EXPECT_TRUE(DispatchEventsUntilCondition(*m_testAssetManager, AssetContainerReady))
                << "The DispatchEventsUntiTimeout function has not completed in "
                << MaxDispatchTimeoutSeconds.count() << " seconds. The test will be marked as a failure\n";
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_RootLoadedAlready_SuccessAndSignal)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_RootLoadedAlready_SuccessAndSignal)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 20);
        {
            // Listen for MyAsset2Id
            ContainerReadyListener readyListener(MyAsset2Id);

            auto asset2Get = m_testAssetManager->GetAssetInternal(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default, AssetLoadParameters{ &AZ::Data::AssetFilterNoAssetLoading });
            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;
            while (!asset2Get->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            auto asset2 = m_testAssetManager->FindOrCreateAsset(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            auto asset2Container = m_testAssetManager->GetAssetContainer(asset2);

            maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset2Container->IsReady(), true);
            EXPECT_EQ(asset2Container->GetDependencies().size(), 1);
            EXPECT_EQ(readyListener.m_ready, 1);

            asset2Get = { };
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_DependencyLoadedAlready_SuccessAndSignal)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_DependencyLoadedAlready_SuccessAndSignal)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 20);
        {
            // Listen for MyAsset2Id
            ContainerReadyListener readyListener(MyAsset2Id);

            auto asset2PrimeGet = m_testAssetManager->GetAsset(MyAsset5Id, azrtti_typeid<AssetWithSerializedData>(), AZ::Data::AssetLoadBehavior::Default);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;
            while (!asset2PrimeGet->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            auto asset2 = m_testAssetManager->FindOrCreateAsset(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            auto asset2Container = m_testAssetManager->GetAssetContainer(asset2);

            maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(asset2Container->IsReady(), true);
            EXPECT_EQ(asset2Container->GetDependencies().size(), 1);
            EXPECT_TRUE(asset2Container->GetRootAsset().GetAs<AssetWithAssetReference>()->m_asset.IsReady());
            EXPECT_EQ(readyListener.m_ready, 1);

            asset2PrimeGet = { };
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_DependencyAndRootLoadedAlready_SuccessAndNoNewSignal)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_DependencyAndRootLoadedAlready_SuccessAndNoNewSignal)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            // Listen for MyAsset2Id
            ContainerReadyListener readyListener(MyAsset2Id);
            auto asset2Get = m_testAssetManager->GetAssetInternal(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default, AssetLoadParameters{ &AssetFilterNoAssetLoading });
            auto asset2PrimeGet = m_testAssetManager->GetAssetInternal(MyAsset5Id, azrtti_typeid<AssetWithSerializedData>(), AZ::Data::AssetLoadBehavior::Default, AssetLoadParameters{ &AssetFilterNoAssetLoading });
            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!asset2Get->IsReady() || !asset2PrimeGet->IsReady())
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            auto asset2 = m_testAssetManager->FindOrCreateAsset(MyAsset2Id, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::Default);
            auto asset2Container = m_testAssetManager->GetAssetContainer(asset2);

            // Should not need to wait, everything should be ready
            EXPECT_EQ(asset2Container->IsReady(), true);
            EXPECT_EQ(asset2Container->GetDependencies().size(), 1);
            // No signal should have been sent, it was already ready
            EXPECT_EQ(readyListener.m_ready, 0);

            // NotifyAssetReady events can still hold references
            m_testAssetManager->DispatchEvents();

            asset2Get = { };
            asset2PrimeGet = { };
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_AssetWithQueueAndPreLoadReferencesTwoLevels_OnAssetReadyFollowsPreloads)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_AssetWithQueueAndPreLoadReferencesTwoLevels_OnAssetReadyFollowsPreloads)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            // Listen for PreloadAssetAId which is one half of the tree under PreloadAssetRootId
            ContainerReadyListener readyListener(PreloadAssetAId);
            OnAssetReadyListener preLoadAListener(PreloadAssetAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener preLoadBListener(PreloadAssetBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener queueLoadBListener(QueueLoadAssetBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());

            preLoadAListener.m_readyCheck = [&]([[maybe_unused]] const OnAssetReadyListener& thisListener)
            {
                return (preLoadBListener.m_ready > 0);
            };

            auto asset = m_testAssetManager->FindOrCreateAsset(PreloadAssetAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(),
                AZ::Data::AssetLoadBehavior::Default);
            auto containerReady = m_testAssetManager->GetAssetContainer(asset);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 2);

            EXPECT_EQ(preLoadAListener.m_ready, 1);
            EXPECT_EQ(preLoadAListener.m_dataLoaded, 1);
            EXPECT_EQ(preLoadBListener.m_ready, 1);
            EXPECT_EQ(preLoadBListener.m_dataLoaded, 0);
            EXPECT_EQ(queueLoadBListener.m_ready, 1);
            EXPECT_EQ(queueLoadBListener.m_dataLoaded, 0);
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_AssetWithQueueAndPreLoadReferencesThreeLevels_OnAssetReadyFollowsPreloads)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_AssetWithQueueAndPreLoadReferencesThreeLevels_OnAssetReadyFollowsPreloads)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            // Listen for PreloadAssetRootId - Should listen for the entire tree.  Preload dependencies should signal NotifyAssetReady in
            // order where dependencies signal first and will signal before the entire container is ready.  QueueLoads are independent
            // And will be ready before the entire container is considered ready, but don't prevent individual assets from signalling ready
            // Once all of their preloads (if Any) and themselves are ready
            ContainerReadyListener readyListener(PreloadAssetRootId);
            OnAssetReadyListener preLoadRootListener(PreloadAssetRootId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener preLoadAListener(PreloadAssetAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener preLoadBListener(PreloadAssetBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener preLoadCListener(PreloadAssetCId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener queueLoadAListener(QueueLoadAssetAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener queueLoadBListener(QueueLoadAssetBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener queueLoadCListener(QueueLoadAssetCId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            preLoadRootListener.m_readyCheck = [&]([[maybe_unused]] const OnAssetReadyListener& thisListener)
            {
                return (preLoadAListener.m_ready && preLoadBListener.m_ready);
            };

            preLoadAListener.m_readyCheck = [&]([[maybe_unused]] const OnAssetReadyListener& thisListener)
            {
                return (preLoadBListener.m_ready > 0);
            };

            queueLoadAListener.m_readyCheck = [&]([[maybe_unused]] const OnAssetReadyListener& thisListener)
            {
                return (preLoadCListener.m_ready > 0);
            };

            auto asset = m_testAssetManager->FindOrCreateAsset(PreloadAssetRootId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(), AZ::Data::AssetLoadBehavior::Default);
            auto containerReady = m_testAssetManager->GetAssetContainer(asset);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 6);

            EXPECT_EQ(preLoadRootListener.m_ready, 1);
            EXPECT_EQ(preLoadRootListener.m_dataLoaded, 1);
            EXPECT_EQ(preLoadAListener.m_ready, 1);
            EXPECT_EQ(preLoadAListener.m_dataLoaded, 1);
            EXPECT_EQ(preLoadBListener.m_ready, 1);
            EXPECT_EQ(preLoadBListener.m_dataLoaded, 0);
            EXPECT_EQ(preLoadCListener.m_ready, 1);
            EXPECT_EQ(preLoadCListener.m_dataLoaded, 0);
            EXPECT_EQ(queueLoadAListener.m_ready, 1);
            EXPECT_EQ(queueLoadAListener.m_dataLoaded, 1);
            EXPECT_EQ(queueLoadBListener.m_ready, 1);
            EXPECT_EQ(queueLoadBListener.m_dataLoaded, 0);
            EXPECT_EQ(queueLoadCListener.m_ready, 1);
            EXPECT_EQ(queueLoadCListener.m_dataLoaded, 0);

        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    // If our preload list contains assets we can't load we should catch the errors and load what we can
#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_RootHasBrokenPreloads_LoadsRoot)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_RootHasBrokenPreloads_LoadsRoot)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(PreloadBrokenDepBId);
            OnAssetReadyListener preLoadBListener(PreloadBrokenDepBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener preLoadNoDataListener(PreloadAssetNoDataId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener preLoadNoHandlerListener(PreloadAssetNoHandlerId, azrtti_typeid<EmptyAssetWithNoHandler>());

            auto asset = m_testAssetManager->FindOrCreateAsset(PreloadBrokenDepBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(), AZ::Data::AssetLoadBehavior::Default);
            auto containerReady = m_testAssetManager->GetAssetContainer(asset);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 0);
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 2);
            EXPECT_EQ(preLoadBListener.m_ready, 1);
            // Had no valid dependencies so didn't need to do any preloading

            EXPECT_EQ(preLoadBListener.m_dataLoaded, 0);

            // None of this should have signalled
            EXPECT_EQ(preLoadNoDataListener.m_ready, 0);
            EXPECT_EQ(preLoadNoDataListener.m_dataLoaded, 0);
            EXPECT_EQ(preLoadNoHandlerListener.m_ready, 0);
            EXPECT_EQ(preLoadNoHandlerListener.m_dataLoaded, 0);
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    // If our preload list contains assets we can't load we should catch the errors and load what we can
#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_ChildHasBrokenPreloads_LoadsRootAndChild)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_ChildHasBrokenPreloads_LoadsRootAndChild)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(PreloadBrokenDepAId);
            OnAssetReadyListener preLoadAListener(PreloadBrokenDepAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener preLoadBListener(PreloadBrokenDepBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener preLoadNoDataListener(PreloadAssetNoDataId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener preLoadNoHandlerListener(PreloadAssetNoHandlerId, azrtti_typeid<EmptyAssetWithNoHandler>());

            auto asset = m_testAssetManager->FindOrCreateAsset(PreloadBrokenDepAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(),
                AZ::Data::AssetLoadBehavior::Default);
            auto containerReady = m_testAssetManager->GetAssetContainer(asset);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 1);
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 2);
            EXPECT_EQ(preLoadAListener.m_ready, 1);
            EXPECT_EQ(preLoadAListener.m_dataLoaded, 1);

            EXPECT_EQ(preLoadBListener.m_ready, 1);
            // Had no valid dependencies so didn't need to do any preloading
            EXPECT_EQ(preLoadBListener.m_dataLoaded, 0);

            // None of this should have signalled
            EXPECT_EQ(preLoadNoDataListener.m_ready, 0);
            EXPECT_EQ(preLoadNoDataListener.m_dataLoaded, 0);
            EXPECT_EQ(preLoadNoHandlerListener.m_ready, 0);
            EXPECT_EQ(preLoadNoHandlerListener.m_dataLoaded, 0);
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    // If our preload list contains assets we can't load we should catch the errors and load what we can
#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_SimpleCircularPreload_LoadsRoot)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_SimpleCircularPreload_LoadsRoot)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(CircularAId);
            OnAssetReadyListener circularAListener(CircularAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());

            // This will attempt to load asset A that has a preload dependency on A.
            AZ_TEST_START_TRACE_SUPPRESSION;
            auto asset = m_testAssetManager->FindOrCreateAsset(CircularAId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(),
                AZ::Data::AssetLoadBehavior::Default);
            auto containerReady = m_testAssetManager->GetAssetContainer(asset);
            // We should catch the basic ciruclar dependency error as well as that it's a preload
            AZ_TEST_STOP_TRACE_SUPPRESSION(2);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            // Even though it's a circular reference, the container will be considered ready once A is loaded,
            // and asset A will in fact be loaded.
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 0);
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 1);
            EXPECT_EQ(circularAListener.m_ready, 1);
            EXPECT_EQ(circularAListener.m_dataLoaded, 0);

            // Break the circular reference so that the test can clean up correctly without leaking memory.
            {
                auto assetData = asset.GetAs<AssetWithQueueAndPreLoadReferences>();
                assetData->m_preLoad.Reset();
            }
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }


    // If our preload list contains assets we can't load we should catch the errors and load what we can
#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_DoubleCircularPreload_LoadsOne)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_DoubleCircularPreload_LoadsOne)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(CircularBId);
            OnAssetReadyListener circularBListener(CircularBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener circularCListener(CircularCId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());

            // This will attempt to load asset B that has a preload dependency on C, and C has a preload dependency on B.
            AZ_TEST_START_TRACE_SUPPRESSION;
            auto asset = m_testAssetManager->FindOrCreateAsset(CircularBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(),
                AZ::Data::AssetLoadBehavior::Default);
            auto containerReady = m_testAssetManager->GetAssetContainer(asset);
            // We should catch the basic circular dependency error as well as that it's a preload
            AZ_TEST_STOP_TRACE_SUPPRESSION(2);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }

            // Even though it's a circular reference, the container will be considered ready once B and C are loaded.
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 1);
            // C's dependency back on B should have been ignored
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 1);
            EXPECT_EQ(circularBListener.m_ready, 1);
            EXPECT_EQ(circularBListener.m_dataLoaded, 1);
            EXPECT_EQ(circularCListener.m_ready, 1);
            // Circular C should be treated as a regular dependency, not a "signaling" one, so it shouldn't go through the dataLoaded path.
            EXPECT_EQ(circularCListener.m_dataLoaded, 0);

            // Break the circular reference so that the test can clean up correctly without leaking memory.
            {
                auto assetData = asset.GetAs<AssetWithQueueAndPreLoadReferences>();
                assetData->m_preLoad.Reset();
            }
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }

    // There should be three errors for self referential and chained circular preload dependencies
    // This container will detect these and still load, however if they were truly required to be
    // PreLoaded there could still be issues at run time, as one will be ready before the other
#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsFloodTest, DISABLED_ContainerLoadTest_CircularPreLoadBelowRoot_LoadCompletes)
#else
    TEST_F(AssetJobsFloodTest, ContainerLoadTest_CircularPreLoadBelowRoot_LoadCompletes)
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusConnect();
        // Setup has already created/destroyed assets
        m_assetHandlerAndCatalog->m_numCreations = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;
        {
            ContainerReadyListener readyListener(CircularDId);
            OnAssetReadyListener circularDListener(CircularDId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener circularBListener(CircularBId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());
            OnAssetReadyListener circularCListener(CircularCId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>());

            // This will attempt to load asset D, which has a preload dependency on B.
            // B has a preload dependency on C, and C has a preload dependency on B.
            AZ_TEST_START_TRACE_SUPPRESSION;
            auto asset = m_testAssetManager->FindOrCreateAsset(CircularDId, azrtti_typeid<AssetWithQueueAndPreLoadReferences>(), AZ::Data::AssetLoadBehavior::Default);
            auto containerReady = m_testAssetManager->GetAssetContainer(asset);
            // One error in SetupPreloads - Two of the assets create a dependency loop
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            auto maxTimeout = AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds;

            while (!readyListener.m_ready)
            {
                m_testAssetManager->DispatchEvents();
                if (AZStd::chrono::system_clock::now() > maxTimeout)
                {
                    break;
                }
                AZStd::this_thread::yield();
            }
            EXPECT_EQ(containerReady->IsReady(), true);
            EXPECT_EQ(containerReady->GetDependencies().size(), 2);
            // C's dependency back on B should have been ignored
            EXPECT_EQ(containerReady->GetInvalidDependencies(), 0);
            EXPECT_EQ(circularDListener.m_ready, 1);
            EXPECT_EQ(circularDListener.m_dataLoaded, 1);

            EXPECT_EQ(circularBListener.m_ready, 1);
            EXPECT_EQ(circularCListener.m_ready, 1);

            // Break the circular reference so that the test can clean up correctly without leaking memory.
            // The references are D -> B <-> C, so we clear B's reference to C to break the loop.
            {
                auto assetDataD = asset.GetAs<AssetWithQueueAndPreLoadReferences>();
                auto assetDataB = assetDataD->m_preLoad.GetAs<AssetWithQueueAndPreLoadReferences>();
                assetDataB->m_preLoad.Reset();
            }
        }

        CheckFinishedCreationsAndDestructions();
        m_assetHandlerAndCatalog->AssetCatalogRequestBus::Handler::BusDisconnect();
    }
    /**
    * Run multiple threads that get and release assets simultaneously to test AssetManager's thread safety
    */
    class AssetJobsMultithreadedTest
        : public BaseAssetManagerTest
    {
    public:
        static inline const AZ::Uuid MyAsset1Id{ "{5B29FE2B-6B41-48C9-826A-C723951B0560}" };
        static inline const AZ::Uuid MyAsset2Id{ "{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}" };
        static inline const AZ::Uuid MyAsset3Id{ "{622C3FC9-5AE2-4E52-AFA2-5F7095ADAB53}" };
        static inline const AZ::Uuid MyAsset4Id{ "{EE99215B-7AB4-4757-B8AF-F78BD4903AC4}" };
        static inline const AZ::Uuid MyAsset5Id{ "{D9CDAB04-D206-431E-BDC0-1DD615D56197}" };
        static inline const AZ::Uuid MyAsset6Id{ "{B2F139C3-5032-4B52-ADCA-D52A8F88E043}" };

        // Initialize the Job Manager with 2 threads for the Asset Manager to use.
        size_t GetNumJobManagerThreads() const override { return 2; }

        static constexpr inline char MyAssetAId[] = "{C5B08D5D-8589-4706-A53F-96248CFDCE73}";
        static constexpr inline char MyAssetBId[] = "{E1DECFB8-6FAC-4FE4-BD54-3A4A4E6616A5}";
        static constexpr inline char MyAssetCId[] = "{F7091500-A220-4407-BEF4-B658D8D24289}";
        static constexpr inline char MyAssetDId[] = "{1BB6CA5B-CE56-497B-B721-9460365E1125}";

        void SetupAssets(DataDrivenHandlerAndCatalog* catalog)
        {
            catalog->AddAsset<AssetWithAssetReference>(MyAsset1Id, "TestAsset1.txt")->AddPreload(MyAsset4Id);
            catalog->AddAsset<AssetWithAssetReference>(MyAsset2Id, "TestAsset2.txt")->AddPreload(MyAsset5Id);
            catalog->AddAsset<AssetWithAssetReference>(MyAsset3Id, "TestAsset3.txt")->AddPreload(MyAsset6Id);
            catalog->AddAsset<AssetWithSerializedData>(MyAsset4Id, "TestAsset4.txt");
            catalog->AddAsset<AssetWithSerializedData>(MyAsset5Id, "TestAsset5.txt");
            catalog->AddAsset<AssetWithSerializedData>(MyAsset6Id, "TestAsset6.txt");

            catalog->AddAsset<AssetWithAssetReference>(MyAssetAId, "TestAsset1.txt")->AddPreload(MyAssetBId);
            catalog->AddAsset<AssetWithAssetReference>(MyAssetBId, "TestAsset2.txt")->AddPreload(MyAssetCId);
            catalog->AddAsset<AssetWithAssetReference>(MyAssetCId, "TestAsset3.txt")->AddPreload(MyAssetDId);
            catalog->AddAsset<AssetWithSerializedData>(MyAssetDId, "TestAsset4.txt");
        }

        void ParallelCreateAndDestroy()
        {
            SerializeContext context;
            AssetWithSerializedData::Reflect(context);
            AssetWithAssetReference::Reflect(context);

            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            auto& db = AssetManager::Instance();

            auto* assetHandlerAndCatalog = aznew DataDrivenHandlerAndCatalog;
            assetHandlerAndCatalog->m_context = &context;
            SetupAssets(assetHandlerAndCatalog);
            AZStd::vector<AssetType> types;
            assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                db.RegisterHandler(assetHandlerAndCatalog, type);
                db.RegisterCatalog(assetHandlerAndCatalog, type);
            }

            {
                AssetWithSerializedData ap1;
                AssetWithSerializedData ap2;
                AssetWithSerializedData ap3;

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &ap1, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset5.txt", AZ::DataStream::ST_XML, &ap2, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset6.txt", AZ::DataStream::ST_XML, &ap3, &context));

                AssetWithAssetReference assetWithPreload1;
                AssetWithAssetReference assetWithPreload2;
                AssetWithAssetReference assetWithPreload3;
                assetWithPreload1.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(MyAsset4Id, AssetLoadBehavior::PreLoad);
                assetWithPreload2.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(MyAsset5Id, AssetLoadBehavior::PreLoad);
                assetWithPreload3.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(MyAsset6Id, AssetLoadBehavior::PreLoad);

                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &assetWithPreload1, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &assetWithPreload2, &context));
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &assetWithPreload3, &context));

                EXPECT_TRUE(assetHandlerAndCatalog->m_numCreations == 3);
                assetHandlerAndCatalog->m_numCreations = 0;
            }

            auto assetUuids = {
                MyAsset1Id,
                MyAsset2Id,
                MyAsset3Id,
            };

            AZStd::vector<AZStd::thread> threads;
            AZStd::mutex mutex;
            AZStd::atomic<int> threadCount((int)assetUuids.size());
            AZStd::condition_variable cv;
            AZStd::atomic_bool keepDispatching(true);

            auto dispatch = [&keepDispatching]()
            {
                while (keepDispatching)
                {
                    AssetManager::Instance().DispatchEvents();
                }
            };

            AZStd::thread dispatchThread(dispatch);

            for (const auto& assetUuid : assetUuids)
            {
                threads.emplace_back([&db, &threadCount, &cv, assetUuid]()
                    {
                        for (int i = 0; i < 1000; i++)
                        {
                            Asset<AssetWithAssetReference> asset1 = db.GetAsset(assetUuid, azrtti_typeid<AssetWithAssetReference>(), AZ::Data::AssetLoadBehavior::PreLoad);

                            asset1.BlockUntilLoadComplete();

                            EXPECT_TRUE(asset1.IsReady());

                            Asset<AssetWithSerializedData> dependentAsset = asset1->m_asset;
                            EXPECT_TRUE(dependentAsset.IsReady());

                            // There should be at least 1 ref here in this scope
                            EXPECT_GE(asset1->GetUseCount(), 1);
                            asset1 = Asset<AssetData>();
                        }

                        threadCount--;
                        cv.notify_one();
                    });
            }

            bool timedOut = false;

            // Used to detect a deadlock.  If we wait for more than 5 seconds, it's likely a deadlock has occurred
            while (threadCount > 0 && !timedOut)
            {
                AZStd::unique_lock<AZStd::mutex> lock(mutex);
                timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds *2));
            }

            EXPECT_EQ(threadCount, 0);

            for (auto& thread : threads)
            {
                thread.join();
            }

            keepDispatching = false;
            dispatchThread.join();

            AssetManager::Destroy();
        }

        void ParallelCyclicAssetReferences()
        {
            SerializeContext context;
            AssetWithAssetReference::Reflect(context);

            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            auto& db = AssetManager::Instance();

            auto* assetHandlerAndCatalog = aznew DataDrivenHandlerAndCatalog;
            assetHandlerAndCatalog->m_context = &context;
            SetupAssets(assetHandlerAndCatalog);
            AZStd::vector<AssetType> types;
            assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                db.RegisterHandler(assetHandlerAndCatalog, type);
                db.RegisterCatalog(assetHandlerAndCatalog, type);
            }

            {
                // A will be saved to disk with MyAsset1Id
                AssetWithAssetReference a;
                a.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(MyAsset2Id);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &a, &context));
                AssetWithAssetReference b;
                b.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(MyAsset3Id);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &b, &context));
                AssetWithAssetReference c;
                c.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(MyAsset4Id);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &c, &context));
                AssetWithAssetReference d;
                d.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(MyAsset5Id);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &d, &context));
                AssetWithAssetReference e;
                e.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(MyAsset6Id);
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset5.txt", AZ::DataStream::ST_XML, &e, &context));
                AssetWithAssetReference f;
                f.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(MyAsset1Id); // refer back to asset1
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset6.txt", AZ::DataStream::ST_XML, &f, &context));

                EXPECT_TRUE(assetHandlerAndCatalog->m_numCreations == 6);
                assetHandlerAndCatalog->m_numCreations = 0;
            }

            const size_t numThreads = 4;
            AZStd::atomic_int threadCount(numThreads);
            AZStd::condition_variable cv;
            AZStd::vector<AZStd::thread> threads;
            AZStd::atomic_bool keepDispatching(true);

            auto dispatch = [&keepDispatching]()
            {
                while (keepDispatching)
                {
                    AssetManager::Instance().DispatchEvents();
                }
            };

            AZStd::thread dispatchThread(dispatch);

            for (size_t threadIdx = 0; threadIdx < numThreads; ++threadIdx)
            {
                threads.emplace_back([&threadCount, &db, &cv]()
                    {
                        Data::Asset<AssetWithAssetReference> assetA = db.GetAsset<AssetWithAssetReference>(
                            MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);
                        while (assetA.IsLoading())
                        {
                            AZStd::this_thread::yield();
                        }

                        EXPECT_TRUE(assetA.IsReady());

                        Data::Asset<AssetWithAssetReference> assetB = assetA->m_asset;
                        EXPECT_TRUE(assetB.IsReady());

                        Data::Asset<AssetWithAssetReference> assetC = assetB->m_asset;
                        EXPECT_TRUE(assetC.IsReady());

                        Data::Asset<AssetWithAssetReference> assetD = assetC->m_asset;
                        EXPECT_TRUE(assetD.IsReady());

                        Data::Asset<AssetWithAssetReference> assetE = assetD->m_asset;
                        EXPECT_TRUE(assetE.IsReady());

                        assetA = Data::Asset<AssetWithAssetReference>();

                        --threadCount;
                        cv.notify_one();
                    });
            }

            // Used to detect a deadlock.  If we wait for more than 5 seconds, it's likely a deadlock has occurred
            bool timedOut = false;
            AZStd::mutex mutex;
            while (threadCount > 0 && !timedOut)
            {
                AZStd::unique_lock<AZStd::mutex> lock(mutex);
                timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds));
            }

            EXPECT_TRUE(threadCount == 0);

            for (auto& thread : threads)
            {
                thread.join();
            }

            keepDispatching = false;
            dispatchThread.join();

            AssetManager::Destroy();
        }

        void ParallelDeepAssetReferences()
        {
            SerializeContext context;
            AssetWithSerializedData::Reflect(context);
            AssetWithAssetReference::Reflect(context);

            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            auto& db = AssetManager::Instance();

            auto* assetHandlerAndCatalog = aznew DataDrivenHandlerAndCatalog;
            assetHandlerAndCatalog->m_context = &context;
            SetupAssets(assetHandlerAndCatalog);
            AZStd::vector<AssetType> types;
            assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                db.RegisterHandler(assetHandlerAndCatalog, type);
                db.RegisterCatalog(assetHandlerAndCatalog, type);
            }

            {
                // AssetD is MYASSETD
                AssetWithSerializedData d;
                d.m_data = 42;
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset4.txt", AZ::DataStream::ST_XML, &d, &context));

                // AssetC is MYASSETC
                AssetWithAssetReference c;
                c.m_asset = AssetManager::Instance().CreateAsset<AssetWithSerializedData>(AssetId(MyAssetDId)); // point at D
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset3.txt", AZ::DataStream::ST_XML, &c, &context));

                // AssetB is MYASSETB
                AssetWithAssetReference b;
                b.m_asset = AssetManager::Instance().CreateAsset<AssetWithAssetReference>(AssetId(MyAssetCId)); // point at C
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset2.txt", AZ::DataStream::ST_XML, &b, &context));

                // AssetA will be written to disk as MYASSETA
                AssetWithAssetReference a;
                a.m_asset = AssetManager::Instance().CreateAsset<AssetWithAssetReference>(AssetId(MyAssetBId)); // point at B
                EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "TestAsset1.txt", AZ::DataStream::ST_XML, &a, &context));
            }

            const size_t numThreads = 4;
            AZStd::atomic_int threadCount(numThreads);
            AZStd::condition_variable cv;
            AZStd::vector<AZStd::thread> threads;
            AZStd::atomic_bool keepDispatching(true);

            auto dispatch = [&keepDispatching]()
            {
                while (keepDispatching)
                {
                    AssetManager::Instance().DispatchEvents();
                }
            };

            AZStd::thread dispatchThread(dispatch);

            for (size_t threadIdx = 0; threadIdx < numThreads; ++threadIdx)
            {
                threads.emplace_back([&threadCount, &db, &cv]()
                    {
                        Data::Asset<AssetWithAssetReference> assetA = db.GetAsset<AssetWithAssetReference>(AssetId(MyAssetAId), AZ::Data::AssetLoadBehavior::Default);

                        assetA.BlockUntilLoadComplete();

                        EXPECT_TRUE(assetA.IsReady());

                        Data::Asset<AssetWithAssetReference> assetB = assetA->m_asset;
                        EXPECT_TRUE(assetB.IsReady());

                        Data::Asset<AssetWithAssetReference> assetC = assetB->m_asset;
                        EXPECT_TRUE(assetC.IsReady());

                        Data::Asset<AssetWithSerializedData> assetD = assetC->m_asset;
                        EXPECT_TRUE(assetD.IsReady());
                        EXPECT_EQ(42, assetD->m_data);

                        assetA = Data::Asset<AssetWithAssetReference>();

                        --threadCount;
                        cv.notify_one();
                    });
            }

            // Used to detect a deadlock.  If we wait for more than 5 seconds, it's likely a deadlock has occurred
            bool timedOut = false;
            AZStd::mutex mutex;
            while (threadCount > 0 && !timedOut)
            {
                AZStd::unique_lock<AZStd::mutex> lock(mutex);
                timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::system_clock::now() + DefaultTimeoutSeconds));
            }

            EXPECT_TRUE(threadCount == 0);

            for (auto& thread : threads)
            {
                thread.join();
            }

            keepDispatching = false;
            dispatchThread.join();

            AssetManager::Destroy();
        }

        void ParallelGetAndReleaseAsset()
        {
            SerializeContext context;
            AssetWithSerializedData::Reflect(context);

            const size_t numThreads = 4;

            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            auto& db = AssetManager::Instance();

            auto* assetHandlerAndCatalog = aznew DataDrivenHandlerAndCatalog;
            assetHandlerAndCatalog->m_context = &context;
            SetupAssets(assetHandlerAndCatalog);
            AZStd::vector<AssetType> types;
            assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                db.RegisterHandler(assetHandlerAndCatalog, type);
                db.RegisterCatalog(assetHandlerAndCatalog, type);
            }

            AZStd::vector<AZ::Uuid> assetUuids = { MyAsset1Id, MyAsset2Id };
            AZStd::vector<AZStd::thread> threads;
            AZStd::atomic<int> threadCount(numThreads);
            AZStd::atomic_bool keepDispatching(true);

            auto dispatch = [&keepDispatching]()
            {
                while (keepDispatching)
                {
                    AssetManager::Instance().DispatchEvents();
                }
            };

            AZStd::atomic_bool wait(true);
            AZStd::atomic_bool keepRunning(true);
            AZStd::atomic<int> threadsRunning(0);
            AZStd::atomic<int> dummy(0);

            AZStd::thread dispatchThread(dispatch);

            auto getAssetFunc = [&db, &threadCount, assetUuids, &wait, &threadsRunning, &dummy, &keepRunning](int index)
            {
                threadsRunning++;
                while (wait)
                {
                    AZStd::this_thread::yield();
                }

                while (keepRunning)
                {
                    for (int innerIdx = index * 7; innerIdx > 0; innerIdx--)
                    {
                        // this inner loop is just to burn some time which will be different
                        // per thread. Adding a dummy statement to ensure that the compiler does not optimize this loop.
                        dummy = innerIdx;
                    }

                    int assetIndex = (int)(index % assetUuids.size());
                    Data::Asset<AssetWithSerializedData> asset1 = db.FindOrCreateAsset(assetUuids[assetIndex], azrtti_typeid<AssetWithSerializedData>(), AZ::Data::AssetLoadBehavior::Default);

                    // There should be at least 1 ref here in this scope
                    EXPECT_GE(asset1.Get()->GetUseCount(), 1);
                };

                threadCount--;
            };

            for (int idx = 0; idx < numThreads; idx++)
            {
                threads.emplace_back(AZStd::bind(getAssetFunc, idx));
            }

            while (threadsRunning < numThreads)
            {
                AZStd::this_thread::yield();
            }

            // We have ensured that all the threads have started at this point and we can let them start hammering at the AssetManager
            wait = false;

            AZStd::chrono::system_clock::time_point start = AZStd::chrono::system_clock::now();
            while (AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(AZStd::chrono::system_clock::now() - start) < AZStd::chrono::seconds(2))
            {
                AZStd::this_thread::yield();
            }

            keepRunning = false;

            for (auto& thread : threads)
            {
                thread.join();
            }

            EXPECT_EQ(threadCount, 0);

            // Make sure asset jobs have finished before validating the number of destroyed assets, because it's possible that the asset job
            // still contains a reference on the job thread that won't trigger the asset destruction until the asset job is destroyed.
            BlockUntilAssetJobsAreComplete();

            EXPECT_EQ(assetHandlerAndCatalog->m_numCreations, assetHandlerAndCatalog->m_numDestructions);
            EXPECT_FALSE(db.FindAsset(assetUuids[0], AZ::Data::AssetLoadBehavior::Default));
            EXPECT_FALSE(db.FindAsset(assetUuids[1], AZ::Data::AssetLoadBehavior::Default));

            keepDispatching = false;
            dispatchThread.join();

            AssetManager::Destroy();
        }
    };
#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_ASSET_JOB_PARALLEL_TESTS
    TEST_F(AssetJobsMultithreadedTest, DISABLED_ParallelCreateAndDestroy)
#else
    TEST_F(AssetJobsMultithreadedTest, ParallelCreateAndDestroy)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        ParallelCreateAndDestroy();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS || AZ_TRAIT_DISABLE_ASSET_JOB_PARALLEL_TESTS
    TEST_F(AssetJobsMultithreadedTest, DISABLED_ParallelGetAndReleaseAsset)
#else
    TEST_F(AssetJobsMultithreadedTest, ParallelGetAndReleaseAsset)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        ParallelGetAndReleaseAsset();
    }

    // This is disabled because cyclic references + pre load is not supported currently, but should be
    TEST_F(AssetJobsMultithreadedTest, DISABLED_ParallelCyclicAssetReferences)
    {
        ParallelCyclicAssetReferences();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetJobsMultithreadedTest, DISABLED_ParallelDeepAssetReferences)
#else
    // temporarily disabled until sporadic failures can be root caused
    TEST_F(AssetJobsMultithreadedTest, DISABLED_ParallelDeepAssetReferences)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        ParallelDeepAssetReferences();
    }

    class AssetManagerTests
        : public BaseAssetManagerTest
    {
    protected:
        static inline const AZ::Uuid MyAsset1Id{ "{5B29FE2B-6B41-48C9-826A-C723951B0560}" };
        static inline const AZ::Uuid MyAsset2Id{ "{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}" };
        static inline const AZ::Uuid MyAsset3Id{ "{622C3FC9-5AE2-4E52-AFA2-5F7095ADAB53}" };

        DataDrivenHandlerAndCatalog* m_assetHandlerAndCatalog;
        AZStd::unique_ptr<AZ::Console> m_console;

        // Initialize the Job Manager with 2 threads for the Asset Manager to use.
        size_t GetNumJobManagerThreads() const override { return 2; }

        void SetUp() override
        {
            BaseAssetManagerTest::SetUp();

            m_console = AZStd::make_unique<AZ::Console>();
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());

            // create the database
            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            // create and register an asset handler
            m_assetHandlerAndCatalog = aznew DataDrivenHandlerAndCatalog;
            m_assetHandlerAndCatalog->m_context = m_serializeContext;
            m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(10, 10);

            AssetWithCustomData::Reflect(*m_serializeContext);

            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset1Id, "MyAsset1.txt");
            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset2Id, "MyAsset2.txt");
            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset3Id, "MyAsset3.txt");

            AZStd::vector<AssetType> types;
            m_assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, type);
                AssetManager::Instance().RegisterCatalog(m_assetHandlerAndCatalog, type);
            }

            WriteAssetToDisk("MyAsset1.txt", MyAsset1Id.ToString<AZStd::string>().c_str());
            WriteAssetToDisk("MyAsset2.txt", MyAsset2Id.ToString<AZStd::string>().c_str());
            WriteAssetToDisk("MyAsset3.txt", MyAsset3Id.ToString<AZStd::string>().c_str());
        }

        void TearDown() override
        {
            AssetManager::Destroy();
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console = nullptr;
            BaseAssetManagerTest::TearDown();
        }
    };

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerTests, DISABLED_BlockUntilLoadComplete_Queued_BlocksUntilLoaded)
#else
    TEST_F(AssetManagerTests, BlockUntilLoadComplete_Queued_BlocksUntilLoaded)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 500);
        {
            auto asset = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);

            EXPECT_FALSE(asset.IsError());
            EXPECT_FALSE(asset.IsReady());

            asset.BlockUntilLoadComplete();
            EXPECT_TRUE(asset.IsReady());
        }
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerTests, DISABLED_BlockUntilLoadComplete_AlreadyLoaded_ContinuesImmediately)
#else
    TEST_F(AssetManagerTests, BlockUntilLoadComplete_AlreadyLoaded_ContinuesImmediately)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 0);
        {
            auto asset = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);

            asset.BlockUntilLoadComplete();
            EXPECT_FALSE(asset.IsError());
            EXPECT_TRUE(asset.IsReady());

            asset.BlockUntilLoadComplete();
            EXPECT_TRUE(asset.IsReady());
        }
    }

    TEST_F(AssetManagerTests, BlockUntilLoadComplete_LoadFailure_ThreadContinuesAfterFailure)
    {
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 50);
        m_assetHandlerAndCatalog->m_failLoad = true;

        {
            auto asset = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);

            EXPECT_FALSE(asset.IsError());
            EXPECT_FALSE(asset.IsReady());

            asset.BlockUntilLoadComplete();
            EXPECT_FALSE(asset.IsReady());
            EXPECT_TRUE(asset.IsError());
        }
    }

    TEST_F(AssetManagerTests, BlockUntilLoadComplete_NotQueued_Fails)
    {
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 500);

        {
            auto asset = AssetManager::Instance().CreateAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);

            EXPECT_FALSE(asset.IsError());
            EXPECT_FALSE(asset.IsReady());

            AZ_TEST_START_TRACE_SUPPRESSION;
            asset.BlockUntilLoadComplete();
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);

            EXPECT_FALSE(asset.IsReady());
        }
    }

    TEST_F(AssetManagerTests, FindOrCreateAsset)
    {
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 0);
        m_assetHandlerAndCatalog->m_numCreations = 0;

        {
            {
                // First call should create
                auto asset = AssetManager::Instance().FindOrCreateAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);

                ASSERT_TRUE(asset);
                EXPECT_FALSE(asset.IsError());
                EXPECT_FALSE(asset.IsReady());
                EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 1);

                // Second call should result in a find
                asset = AssetManager::Instance().FindOrCreateAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);

                ASSERT_TRUE(asset);
                EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 1); // Should be the same as before
            }

            {
                // Test that we create another asset after all references are deleted
                auto asset = AssetManager::Instance().FindOrCreateAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);

                ASSERT_TRUE(asset);
                EXPECT_FALSE(asset.IsError());
                EXPECT_FALSE(asset.IsReady());
                EXPECT_EQ(m_assetHandlerAndCatalog->m_numCreations, 2);
            }
        }
    }

    struct CancelListener
        : AssetBus::Handler
    {
        CancelListener(const AZ::Data::AssetId& assetId)
        {
            BusConnect(assetId);
        }

        void OnAssetCanceled([[maybe_unused]] AssetId assetId) override
        {
            m_canceled = true;
        }

        ~CancelListener() override
        {
            BusDisconnect();
        }

        AZStd::atomic_bool m_canceled{ false };
    };

    using AssetManagerCancelTests = AssetManagerTests;

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerCancelTests, DISABLED_CancelLoad_NoReferences_LoadCancels)
#else
    // Asset cancellation is temporarily disabled, re-enable this test when cancellation is more stable. LYN-3263
    TEST_F(AssetManagerCancelTests, DISABLED_CancelLoad_NoReferences_LoadCancels)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 100);
        m_assetHandlerAndCatalog->m_numLoads = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;

        {
            CancelListener listener(MyAsset3Id);

            auto asset1 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default); // These are not flagged as blocking loads because doing so moves the work off the job thread, which we need to keep busy
            auto asset2 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset2Id, AZ::Data::AssetLoadBehavior::Default);

            ASSERT_TRUE(asset1);
            ASSERT_TRUE(asset2);

            {
                auto asset3 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);

                ASSERT_TRUE(asset3);
            }

            asset1.BlockUntilLoadComplete();
            asset2.BlockUntilLoadComplete();

            EXPECT_TRUE(asset1.IsReady());
            EXPECT_TRUE(asset2.IsReady());

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
            AssetManager::Instance().DispatchEvents();

            EXPECT_EQ(m_assetHandlerAndCatalog->m_numLoads, 2);
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numDestructions, 1);
            EXPECT_TRUE(listener.m_canceled);
        }
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerCancelTests, DISABLED_CanceledLoad_CanBeLoadedAgainLater)
#else
    // Asset cancellation is temporarily disabled, re-enable this test when cancellation is more stable. LYN-3263
    TEST_F(AssetManagerCancelTests, DISABLED_CanceledLoad_CanBeLoadedAgainLater)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 50);
        m_assetHandlerAndCatalog->m_numLoads = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;

        {
            auto asset1 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);
            auto asset2 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset2Id, AZ::Data::AssetLoadBehavior::Default);

            ASSERT_TRUE(asset1);
            ASSERT_TRUE(asset2);

            {
                auto asset3 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);

                ASSERT_TRUE(asset3);
            }

            asset1.BlockUntilLoadComplete();
            asset2.BlockUntilLoadComplete();

            EXPECT_TRUE(asset1.IsReady());
            EXPECT_TRUE(asset2.IsReady());

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(100));
            AssetManager::Instance().DispatchEvents();

            EXPECT_EQ(m_assetHandlerAndCatalog->m_numLoads, 2);
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numDestructions, 1);

            {
                auto asset3 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);

                ASSERT_TRUE(asset3);

                EXPECT_FALSE(asset3.IsReady());

                asset3.BlockUntilLoadComplete();

                EXPECT_TRUE(asset3.IsReady());
            }
        }
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerCancelTests, DISABLED_CancelLoad_InProgressLoad_Continues)
#else
    // Asset cancellation is temporarily disabled, re-enable this test when cancellation is more stable. LYN-3263
    TEST_F(AssetManagerCancelTests, DISABLED_CancelLoad_InProgressLoad_Continues)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, 100);
        m_assetHandlerAndCatalog->m_numLoads = 0;
        m_assetHandlerAndCatalog->m_numDestructions = 0;

        {
            {
                auto asset1 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);

                ASSERT_TRUE(asset1);

                AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(50));
            }

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(80));
            AssetManager::Instance().DispatchEvents();

            EXPECT_EQ(m_assetHandlerAndCatalog->m_numLoads, 1);
            EXPECT_EQ(m_assetHandlerAndCatalog->m_numDestructions, 1);
        }
    }


    struct AssetManagerLoadWarningTests
        : AssetManagerTests
        , public AZ::Debug::TraceMessageBus::Handler
    {
    public:
        void SetUp() override
        {
            AssetManagerTests::SetUp();
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            AssetManagerTests::TearDown();
        }

        // Helper method to set the AssetManager console variable that controls the warning threshold.
        // Whenever the threshold is exceeded, a warning will be printed.
        void SetLoadWarningMilliseconds(uint32_t milliseconds)
        {
            AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();
            ASSERT_TRUE(console);

            bool warningEnable = false;
            console->PerformCommand("cl_assetLoadWarningEnable true");
            EXPECT_EQ(console->GetCvarValue("cl_assetLoadWarningEnable", warningEnable), GetValueResult::Success);
            EXPECT_TRUE(warningEnable);

            uint32_t thresholdMs = 0;
            console->PerformCommand(AZStd::string::format("cl_assetLoadWarningMsThreshold %u", milliseconds).c_str());
            EXPECT_EQ(console->GetCvarValue("cl_assetLoadWarningMsThreshold", thresholdMs), GetValueResult::Success);
            EXPECT_EQ(thresholdMs, milliseconds);
        }

        // Track the number of warnings emitted during the test.
        bool OnWarning([[maybe_unused]] const char* window, [[maybe_unused]] const char* message) override
        {
            m_numWarnings++;
            return false;
        }

        int m_numWarnings = 0;
    };

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerLoadWarningTests, DISABLED_AssetManager_LoadAssetThresholdWarning_TriggersWhenThresholdExceeded)
#else
    TEST_F(AssetManagerLoadWarningTests, AssetManager_LoadAssetThresholdWarning_TriggersWhenThresholdExceeded)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        constexpr uint32_t loadWarningThresholdMs = 50;
        constexpr uint32_t loadMs = 100;
        SetLoadWarningMilliseconds(loadWarningThresholdMs);
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, loadMs);

        {
            auto asset = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);
            asset.BlockUntilLoadComplete();
            EXPECT_TRUE(asset.IsReady());
        }
        // If warnings are enabled, we should get a notification that we've gone beyond the load time threshold.
#ifdef AZ_ENABLE_TRACING
        EXPECT_EQ(1, m_numWarnings);
#else
        EXPECT_EQ(0, m_numWarnings);
#endif

        AssetManager::Destroy();
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerLoadWarningTests, DISABLED_AssetManager_LoadAssetThresholdWarning_DoesNotTriggersWhenAtOrBelowThreshold)
#else
    TEST_F(AssetManagerLoadWarningTests, AssetManager_LoadAssetThresholdWarning_DoesNotTriggersWhenAtOrBelowThreshold)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        constexpr uint32_t loadWarningThresholdMs = 100;
        constexpr uint32_t loadMs = 10;
        SetLoadWarningMilliseconds(loadWarningThresholdMs);
        m_assetHandlerAndCatalog->SetArtificialDelayMilliseconds(0, loadMs);

        {
            auto asset = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);
            asset.BlockUntilLoadComplete();
            EXPECT_TRUE(asset.IsReady());
        }
        // We should NOT get a notification that we've gone beyond the load time threshold.
        EXPECT_EQ(0, m_numWarnings);

        AssetManager::Destroy();
    }

    /**
    * This class sets up the data and parameters needed for testing various scenarios in which asset references get cleared while in
    * the middle of loading.  The tests help ensure that assets can't get stuck in perpetual loading states.
    **/
    class AssetManagerClearAssetReferenceTests
        : public BaseAssetManagerTest
    {
    protected:
        static inline const AZ::Uuid RootAssetId{ "{AB13F568-C676-41FE-A7E9-341F71A78104}" };
        static inline const AZ::Uuid DependentPreloadAssetId{ "{9E0AE541-0080-4ADA-B4F4-A0F25D0A6D1A}" };
        static inline const AZ::Uuid NestedDependentPreloadBlockingAssetId{ "{FED70BCD-2846-4CBA-84D1-ED24DA7FCD4B}" };

        DataDrivenHandlerAndCatalog* m_assetHandlerAndCatalog;
        LoadAssetDataSynchronizer m_loadDataSynchronizer;

        // Initialize the Job Manager with 1 thread for the Asset Manager to use.
        // This is necessary for these tests to help ensure that we can control the exact loading state more deterministically.
        size_t GetNumJobManagerThreads() const override { return 1; }

        AssetManagerClearAssetReferenceTests() = default;
        ~AssetManagerClearAssetReferenceTests() override = default;

        void SetUp() override
        {
            BaseAssetManagerTest::SetUp();

            // create the database
            AssetManager::Descriptor desc;
            AssetManager::Create(desc);

            // create and register an asset handler
            m_assetHandlerAndCatalog = aznew DataDrivenHandlerAndCatalog;
            m_assetHandlerAndCatalog->m_context = m_serializeContext;

            AssetWithCustomData::Reflect(*m_serializeContext);
            AssetWithSerializedData::Reflect(*m_serializeContext);
            AssetWithAssetReference::Reflect(*m_serializeContext);

            // For our tests, we will set up a chain of assets that look like this:
            // RootAssetId -(Preload)-> DependentPreloadAssetId -(Preload)-> NestedDependentPreloadBlockingAssetId
            // The asset at the end of the chain uses a loadDataSynchronizer to ensure that we can synchronize logic perfectly
            // with the exact moment that it is in the middle of the "Loading" phase.
            // These tests validate behaviors when the root asset is released while a dependent asset is in the middle of loading,
            // so getting the timing correct is mandatory for these tests.

            m_assetHandlerAndCatalog->AddAsset<AssetWithAssetReference>(RootAssetId,
                "RootAsset.txt")->AddPreload(DependentPreloadAssetId);
            m_assetHandlerAndCatalog->AddAsset<AssetWithAssetReference>(DependentPreloadAssetId,
                "DependentPreloadAsset.txt")->AddPreload(NestedDependentPreloadBlockingAssetId);
            m_assetHandlerAndCatalog->AddAsset<AssetWithSerializedData>(NestedDependentPreloadBlockingAssetId,
                "DependentPreloadBlockingAsset.txt", 0, false, false, &m_loadDataSynchronizer);

            AZStd::vector<AssetType> types;
            m_assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, type);
                AssetManager::Instance().RegisterCatalog(m_assetHandlerAndCatalog, type);
            }

            // Create and save the dependent asset first, so that we can get a reference to it.
            AssetWithSerializedData dependentBlockingAsset;
            EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "DependentPreloadBlockingAsset.txt",
                AZ::DataStream::ST_XML, &dependentBlockingAsset, m_serializeContext));

            AssetWithAssetReference dependentAsset;
            dependentAsset.m_asset = AssetManager::Instance().CreateAsset<AssetWithAssetReference>(
                NestedDependentPreloadBlockingAssetId, AssetLoadBehavior::PreLoad);
            EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "DependentPreloadAsset.txt",
                AZ::DataStream::ST_XML, &dependentAsset, m_serializeContext));

            // Create and save the top-level asset.
            AssetWithAssetReference rootAsset;
            rootAsset.m_asset = AssetManager::Instance().CreateAsset<AssetWithAssetReference>(
                DependentPreloadAssetId, AssetLoadBehavior::PreLoad);
            EXPECT_TRUE(AZ::Utils::SaveObjectToFile(GetTestFolderPath() + "RootAsset.txt",
                AZ::DataStream::ST_XML, &rootAsset, m_serializeContext));
        }

        void TearDown() override
        {
            // Manually release the handler.  By doing this, we're also implicitly validating that no assets have
            // remained in a loading state at the point of test teardown.
            AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);
            AssetManager::Instance().UnregisterCatalog(m_assetHandlerAndCatalog);
            delete m_assetHandlerAndCatalog;

            AssetManager::Destroy();
            BaseAssetManagerTest::TearDown();
        }
    };

    // Verify that within a SuspendAssetRelease / ResumeAssetRelease block, if an asset is in the Loading state and all references to it
    // are removed, and then a new reference is requested, that the asset will finish loading successfully.  The specific error case
    // being checked is as follows:
    // - A 0-refcounted asset reference could cause the surrounding asset container to be destroyed
    // - The asset itself is never freed because of the SuspendAssetRelease call
    // - A new asset reference is requested
    // - The asset could now be in a perpetual loading state because the asset exists in a Loading state, but there is no top-level
    //   asset container to ever transition the asset to a Ready state.
#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerClearAssetReferenceTests,
            DISABLED_ContainerLoadTest_AssetLosesAndGainsReferencesDuringLoadAndSuspendedRelease_AssetSuccessfullyFinishesLoading)
#else
    // Asset cancellation is temporarily disabled, re-enable this test when cancellation is more stable. LYN-3263
    TEST_F(AssetManagerClearAssetReferenceTests,
            DISABLED_ContainerLoadTest_AssetLosesAndGainsReferencesDuringLoadAndSuspendedRelease_AssetSuccessfullyFinishesLoading)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
{
        // Start the load and wait for the dependent asset to hit the loading state.
        auto rootAsset = AssetManager::Instance().GetAsset<AssetWithAssetReference>(RootAssetId, AssetLoadBehavior::Default);
        while (m_loadDataSynchronizer.m_numBlocking < 1)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));
        }

        // Verify that the loading state on the nested dependent asset has been reached.
        // (We use FindAsset instead of GetAsset to ensure that we don't create any additional asset containers or trigger any loads)
        auto nestedDependentAsset = AssetManager::Instance().FindAsset<AssetWithSerializedData>(
            NestedDependentPreloadBlockingAssetId, AssetLoadBehavior::Default);
        EXPECT_EQ(nestedDependentAsset.GetStatus(), AssetData::AssetStatus::Loading);

        // Verify that the root asset isn't loading yet, and so we have the only strong reference to the asset.
        EXPECT_TRUE((rootAsset.GetStatus() == AssetData::AssetStatus::Queued) || (rootAsset.GetStatus() == AssetData::AssetStatus::Loading) || rootAsset.GetStatus() == AssetData::AssetStatus::StreamReady);
        EXPECT_EQ(rootAsset->GetUseCount(), 1);

        // Suspend asset releases so that 0-refcounted assets aren't destroyed.
        AssetManager::Instance().SuspendAssetRelease();

        // Release the reference to the asset.  Normally, this would destroy the asset, but because we called SuspendAssetRelease the
        // reference will stay around internally within the Asset Manager.  This is normally used for cases where we expect the asset
        // will nearly immediately get a new reference.
        rootAsset.Reset();

        // Get a new reference to the asset, and verify that it is still in a queued or loading state.
        rootAsset = AssetManager::Instance().GetAsset<AssetWithAssetReference>(RootAssetId, AssetLoadBehavior::Default);
        EXPECT_TRUE((rootAsset.GetStatus() == AssetData::AssetStatus::Queued) || (rootAsset.GetStatus() == AssetData::AssetStatus::Loading) || rootAsset.GetStatus() == AssetData::AssetStatus::StreamReady);

        // Allow 0-refcounted assets to be destroyed again.
        AssetManager::Instance().ResumeAssetRelease();

        // Now that we've removed and regained a reference to the asset while in the loading state, allow the asset to continue loading.
        m_loadDataSynchronizer.m_readyToLoad = true;
        m_loadDataSynchronizer.m_condition.notify_all();
        AssetManager::Instance().DispatchEvents();

        // If the test works, the load will complete and the asset will successfully load.
        // If the test fails, the asset will be in a perpetual loading state and this will deadlock, with the loading threads sitting idle.
        rootAsset.BlockUntilLoadComplete();
        EXPECT_TRUE(rootAsset.IsReady());

        // Run one final DispatchEvents to clear out the event queue.
        AssetManager::Instance().DispatchEvents();
    }

    // Verify that if the root asset in an asset container no longer has any references, and the asset container is still
    // in the middle of loading, then the asset container will finish loading before destroying itself.  The specific bug
    // case to watch for is that any dependent asset loads that are triggered won't transition to a ready state until the
    // entire container is ready, so deleting the container mid-load would cause those dependent assets to get stuck in a
    // perpetual loading state.
#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerClearAssetReferenceTests, DISABLED_ContainerLoadTest_RootAssetDestroyedWhileContainerLoading_ContainerFinishesLoad)
#else
    // Asset cancellation is temporarily disabled, re-enable this test when cancellation is more stable. LYN-3263
    TEST_F(AssetManagerClearAssetReferenceTests, DISABLED_ContainerLoadTest_RootAssetDestroyedWhileContainerLoading_ContainerFinishesLoad)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        OnAssetReadyListener assetStatus1(DependentPreloadAssetId, azrtti_typeid<AssetWithAssetReference>());
        OnAssetReadyListener assetStatus2(NestedDependentPreloadBlockingAssetId, azrtti_typeid<AssetWithSerializedData>());

        // Start the load and wait for the dependent asset to hit the loading state.
        auto rootAsset = AssetManager::Instance().GetAsset<AssetWithAssetReference>(RootAssetId, AssetLoadBehavior::Default);
        while (m_loadDataSynchronizer.m_numBlocking < 1)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));
        }

        // Get references to the dependent and nested dependent assets
        auto dependentAsset = AssetManager::Instance().FindAsset<AssetWithAssetReference>(
            DependentPreloadAssetId, AssetLoadBehavior::Default);
        auto nestedDependentAsset = AssetManager::Instance().FindAsset<AssetWithSerializedData>(
            NestedDependentPreloadBlockingAssetId, AssetLoadBehavior::Default);

        // Verify that the loading state on the nested dependent asset has been reached.
        EXPECT_EQ(nestedDependentAsset.GetStatus(), AssetData::AssetStatus::Loading);

        // Verify that the root asset isn't loading yet, and so we have only one strong reference to the asset, owned by this method.
        EXPECT_TRUE((rootAsset.GetStatus() == AssetData::AssetStatus::Queued) || (rootAsset.GetStatus() == AssetData::AssetStatus::Loading) || rootAsset.GetStatus() == AssetData::AssetStatus::StreamReady);
        EXPECT_EQ(rootAsset->GetUseCount(), 1);

        // Release the reference to the asset.  This should destroy the asset.  However, because the container for the asset is still in
        // a loading state, it needs to remain around until it finishes or else the dependent assets that are mid-load will get stuck in a
        // perpetual loading state.
        rootAsset.Reset();

        // Now that we've destroyed the root asset, allow the nested dependent asset to continue loading.
        m_loadDataSynchronizer.m_readyToLoad = true;
        m_loadDataSynchronizer.m_condition.notify_all();
        AssetManager::Instance().DispatchEvents();

        // If the test works, the loads will complete and the dependent assets will successfully load.
        // We specifically wait for OnAssetReady to be triggered instead of using BlockUntilLoadComplete() to ensure that
        // all loading jobs have 100% completed and there aren't any other outstanding asset references held on other threads.
        const auto timeoutSeconds = AZStd::chrono::seconds(20);
        auto maxTimeout = AZStd::chrono::system_clock::now() + timeoutSeconds;
        bool timedOut = false;
        while (!(assetStatus1.m_ready && assetStatus2.m_ready))
        {
            AssetBus::ExecuteQueuedEvents();
            if (AZStd::chrono::system_clock::now() > maxTimeout)
            {
                timedOut = true;
                break;
            }
        }
        EXPECT_FALSE(timedOut);
        EXPECT_TRUE(dependentAsset.IsReady());
        EXPECT_TRUE(nestedDependentAsset.IsReady());
    }

    using AssetManagerErrorTests = AssetManagerTests;

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerErrorTests, DISABLED_QueueLoad_WithMissingAsset_ReturnsFalse)
#else
    TEST_F(AssetManagerErrorTests, QueueLoad_WithMissingAsset_ReturnsFalse)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        AZ::Data::AssetId invalidId("{D000421C-96EF-4FF6-9B7A-3F639E907675}");
        OnAssetReadyListener assetStatus(invalidId, azrtti_typeid<AssetWithCustomData>());

        // Create an asset with an ID that isn't currently registered.
        AZ::Data::Asset<AssetWithCustomData> invalidAsset(invalidId, azrtti_typeid<AssetWithCustomData>());

        // Try to queue the load.  This will generate a warning due to the asset not being in the catalog.
        bool queueResult = invalidAsset.QueueLoad();

        // Because the asset is missing, the QueueLoad should return false.
        EXPECT_FALSE(queueResult);

        // OnAssetError should get called for the asset once DispatchEvents is called.
        AssetManager::Instance().DispatchEvents();
        EXPECT_EQ(assetStatus.m_error, 1);
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerErrorTests, DISABLED_GetAsset_WithMissingAsset_ReturnsValidAssetWithErrorState)
#else
    TEST_F(AssetManagerErrorTests, GetAsset_WithMissingAsset_ReturnsValidAssetWithErrorState)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        AZ::Data::AssetId invalidId("{D000421C-96EF-4FF6-9B7A-3F639E907675}");
        OnAssetReadyListener assetStatus(invalidId, azrtti_typeid<AssetWithCustomData>());

        // Call GetAsset with an ID that isn't currently registered.  This will generate a warning
        auto invalidAsset = AssetManager::Instance().GetAsset<AssetWithCustomData>(invalidId, AssetLoadBehavior::Default);

        // Because the asset is missing, we should end up with a valid asset in an error state.
        EXPECT_TRUE(invalidAsset);
        EXPECT_TRUE(invalidAsset.IsError());

        // OnAssetError should get called for the asset once DispatchEvents is called.
        AssetManager::Instance().DispatchEvents();
        EXPECT_EQ(assetStatus.m_error, 1);
    }

#if AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    TEST_F(AssetManagerErrorTests, DISABLED_AssetBus_BusConnectAfterLoadOfMissingAsset_OnAssetErrorStillGenerated)
#else
    TEST_F(AssetManagerErrorTests, AssetBus_BusConnectAfterLoadOfMissingAsset_OnAssetErrorStillGenerated)
#endif // AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_TESTS
    {
        AZ::Data::AssetId invalidId("{D000421C-96EF-4FF6-9B7A-3F639E907675}");

        // Call GetAsset with an ID that isn't currently registered.  This will generate a warning due to the asset not being in the catalog
        auto invalidAsset = AssetManager::Instance().GetAsset<AssetWithCustomData>(invalidId, AssetLoadBehavior::Default);

        // Call DispatchEvents to clear out the event queue.
        AssetManager::Instance().DispatchEvents();

        // Start listening for the asset *after* the events have been sent.  This should still immediately generate an OnAssetError.
        {
            OnAssetReadyListener assetStatus(invalidId, azrtti_typeid<AssetWithCustomData>());
            EXPECT_EQ(assetStatus.m_error, 1);
        }
    }
}
