/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/Instance/InstanceDatabase.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Debug/Timer.h>

using namespace AZ;
using namespace AZ::Data;

namespace UnitTest
{
    static const AssetId s_assetId0{ Uuid("{5B29FE2B-6B41-48C9-826A-C723951B0560}") };
    static const AssetId s_assetId1{ Uuid("{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}") };
    static const AssetId s_assetId2{ Uuid("{EE99215B-7AB4-4757-B8AF-F78BD4903AC4}") };
    static const AssetId s_assetId3{ Uuid("{D9CDAB04-D206-431E-BDC0-1DD615D56197}") };

    static const InstanceId s_instanceId0{ InstanceId::CreateFromAssetId(s_assetId0) };
    static const InstanceId s_instanceId1{ InstanceId::CreateFromAssetId(s_assetId1) };
    static const InstanceId s_instanceId2{ InstanceId::CreateFromAssetId(s_assetId2) };
    static const InstanceId s_instanceId3{ InstanceId::CreateFromAssetId(s_assetId3) };

    // test asset type
    class TestAssetType : public AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(TestAssetType, AZ::SystemAllocator);
        AZ_RTTI(TestAssetType, "{73D60606-BDE5-44F9-9420-5649FE7BA5B8}", AssetData);

        TestAssetType()
        {
            m_status = AssetStatus::Ready;
        }
    };

    class TestInstanceA : public InstanceData
    {
    public:
        AZ_INSTANCE_DATA(TestInstanceA, "{65CBF1C8-F65F-4A84-8A11-B510BC435DB0}");
        AZ_CLASS_ALLOCATOR(TestInstanceA, AZ::SystemAllocator);

        TestInstanceA(TestAssetType* asset)
            : m_asset{ asset, AZ::Data::AssetLoadBehavior::Default }
        {
        }

        Asset<TestAssetType> m_asset;
    };

    class TestInstanceB : public InstanceData
    {
    public:
        AZ_INSTANCE_DATA(TestInstanceB, "{4ED0A8BF-7800-44B2-AC73-2CB759C61C37}");
        AZ_CLASS_ALLOCATOR(TestInstanceB, AZ::SystemAllocator);

        TestInstanceB(TestAssetType* asset)
            : m_asset{ asset, AZ::Data::AssetLoadBehavior::Default }
        {
        }

        ~TestInstanceB()
        {
            if (m_onDeleteCallback)
            {
                m_onDeleteCallback();
            }
        }

        Asset<TestAssetType> m_asset;
        AZStd::function<void()> m_onDeleteCallback;
    };

    // test asset handler
    template<typename AssetDataT>
    class MyAssetHandler : public AssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MyAssetHandler, AZ::SystemAllocator);

        AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override
        {
            (void)id;
            EXPECT_TRUE(type == AzTypeInfo<AssetDataT>::Uuid());
            if (type == AzTypeInfo<AssetDataT>::Uuid())
            {
                return aznew AssetDataT();
            }
            return nullptr;
        }

        LoadResult LoadAssetData(const Asset<AssetData>&, AZStd::shared_ptr<AssetDataStream>, const AZ::Data::AssetFilterCB&) override
        {
            return LoadResult::Error;
        }

        void DestroyAsset(AssetPtr ptr) override
        {
            EXPECT_TRUE(ptr->GetType() == AzTypeInfo<AssetDataT>::Uuid());
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override
        {
            assetTypes.push_back(AzTypeInfo<AssetDataT>::Uuid());
        }
    };

    class InstanceDatabaseTest : public LeakDetectionFixture
    {
    protected:
        MyAssetHandler<TestAssetType>* m_assetHandler;

    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            // create the asset database
            {
                AssetManager::Descriptor desc;
                AssetManager::Create(desc);
            }

            // create the instance database
            {
                InstanceHandler<TestInstanceA> instanceHandler;
                instanceHandler.m_createFunction = [](AssetData* assetData)
                {
                    EXPECT_TRUE(azrtti_istypeof<TestAssetType>(assetData));
                    return aznew TestInstanceA(static_cast<TestAssetType*>(assetData));
                };
                InstanceDatabase<TestInstanceA>::Create(azrtti_typeid<TestAssetType>(), instanceHandler);
            }

            // create and register an asset handler
            m_assetHandler = aznew MyAssetHandler<TestAssetType>;
            AssetManager::Instance().RegisterHandler(m_assetHandler, AzTypeInfo<TestAssetType>::Uuid());
        }

        void TearDown() override
        {
            // destroy the database
            AssetManager::Destroy();

            InstanceDatabase<TestInstanceA>::Destroy();

            LeakDetectionFixture::TearDown();
        }
    };

    TEST_F(InstanceDatabaseTest, InstanceCreate)
    {
        auto& assetManager = AssetManager::Instance();
        auto& instanceDatabase = InstanceDatabase<TestInstanceA>::Instance();

        Asset<TestAssetType> someAsset = assetManager.CreateAsset<TestAssetType>(s_assetId0, AZ::Data::AssetLoadBehavior::Default);

        Instance<TestInstanceA> instance = instanceDatabase.Find(s_instanceId0);
        EXPECT_EQ(instance, nullptr);

        instance = instanceDatabase.FindOrCreate(s_instanceId0, someAsset);
        EXPECT_NE(instance, nullptr);

        Instance<TestInstanceA> instance2 = instanceDatabase.FindOrCreate(s_instanceId0, someAsset);
        EXPECT_EQ(instance, instance2);

        Instance<TestInstanceA> instance3 = instanceDatabase.Find(s_instanceId0);
        EXPECT_EQ(instance, instance3);
    }

    TEST_F(InstanceDatabaseTest, InstanceOrphan)
    {
        auto& assetManager = AssetManager::Instance();
        auto& instanceDatabase = InstanceDatabase<TestInstanceA>::Instance();

        Asset<TestAssetType> someAsset = assetManager.CreateAsset<TestAssetType>(s_assetId0, AZ::Data::AssetLoadBehavior::Default);

        Instance<TestInstanceA> orphanedInstance = instanceDatabase.FindOrCreate(s_instanceId0, someAsset);
        EXPECT_NE(orphanedInstance, nullptr);

        instanceDatabase.TEMPOrphan(s_instanceId0);
        // After orphan, the instance should not be found in the database, but it should still be valid
        EXPECT_EQ(instanceDatabase.Find(s_instanceId0), nullptr);
        EXPECT_NE(orphanedInstance, nullptr);

        instanceDatabase.TEMPOrphan(s_instanceId0);
        // Orphaning twice should be a no-op
        EXPECT_EQ(instanceDatabase.Find(s_instanceId0), nullptr);
        EXPECT_NE(orphanedInstance, nullptr);

        Instance<TestInstanceA> instance2 = instanceDatabase.FindOrCreate(s_instanceId0, someAsset);
        // Creating another instance with the same id should return a different instance than the one that was orphaned
        EXPECT_NE(orphanedInstance, instance2);
    }

    enum class ParallelInstanceTestCases
    {
        Create,
        CreateAndDeferRemoval,
        CreateAndOrphan,
        CreateDeferRemovalAndOrphan
    };

    enum class ParralleInstanceCurrentAction
    {
        Create,
        DeferredRemoval,
        Orphan
    };

    ParralleInstanceCurrentAction ParallelInstanceGetCurrentAction(const ParallelInstanceTestCases& testCase)
    {
        switch (testCase)
        {
        case ParallelInstanceTestCases::CreateAndDeferRemoval:
            switch (rand() % 2)
            {
            case 0: return ParralleInstanceCurrentAction::Create;
            case 1: return ParralleInstanceCurrentAction::DeferredRemoval;
            }
        case ParallelInstanceTestCases::CreateAndOrphan:
            switch (rand() % 2)
            {
            case 0: return ParralleInstanceCurrentAction::Create;
            case 1: return ParralleInstanceCurrentAction::Orphan;
            }
        case ParallelInstanceTestCases::CreateDeferRemovalAndOrphan:
            switch (rand() % 3)
            {
            case 0: return ParralleInstanceCurrentAction::Create;
            case 1: return ParralleInstanceCurrentAction::DeferredRemoval;
            case 2: return ParralleInstanceCurrentAction::Orphan;
            }
        case ParallelInstanceTestCases::Create:
        default:
            return ParralleInstanceCurrentAction::Create;
        }
    }

    void ParallelInstanceCreateHelper(const size_t& threadCountMax, const size_t& assetIdCount, const uint32_t& iterations, const ParallelInstanceTestCases& testCase)
    {
        //printf("Testing threads=%zu assetIds=%zu ... ", threadCountMax, assetIdCount);

        AZ::Debug::Timer timer;
        timer.Stamp();

        auto& assetManager = AssetManager::Instance();
        auto& instanceManager = InstanceDatabase<TestInstanceA>::Instance();

        AZStd::vector<Uuid> guids;
        AZStd::vector<Data::Instance<Data::InstanceData>> instances;
        AZStd::vector<Asset<TestAssetType>> assets;

        for (size_t i = 0; i < assetIdCount; ++i)
        {
            Uuid guid = Uuid::CreateRandom();

            guids.emplace_back(guid);
            instances.emplace_back(nullptr);

            // Pre-create asset so we don't attempt to load it from the catalog.
            assets.emplace_back(assetManager.CreateAsset<TestAssetType>(guid, AZ::Data::AssetLoadBehavior::Default));
        }

        AZStd::vector<AZStd::thread> threads;
        AZStd::mutex mutex;
        AZStd::mutex referenceTableMutex;
        AZStd::atomic<int> threadCount((int)threadCountMax);
        AZStd::condition_variable cv;
        AZStd::atomic_bool keepDispatching(true);

        auto dispatch = [&keepDispatching]()
        {
            while (keepDispatching)
            {
                AssetManager::Instance().DispatchEvents();
            }
        };

        srand(0);

        AZStd::thread dispatchThread(dispatch);

        for (size_t i = 0; i < threadCountMax; ++i)
        {
            threads.emplace_back(
                [&instanceManager, &threadCount, &cv, &guids, &instances, &assets, &iterations, &testCase, &referenceTableMutex]()
                {

                    bool deferRemoval = testCase == ParallelInstanceTestCases::CreateAndDeferRemoval ||
                            testCase == ParallelInstanceTestCases::CreateDeferRemovalAndOrphan;

                    for (uint32_t i = 0; i < iterations; ++i) // queue up a bunch of work
                    {
                        const size_t index = rand() % guids.size();
                        const Uuid uuid = guids[index];
                        const AssetId assetId{ uuid };
                        const InstanceId instanceId{ InstanceId::CreateFromAssetId(assetId) };

                        ParralleInstanceCurrentAction currentAction = ParallelInstanceGetCurrentAction(testCase);

                        if (currentAction == ParralleInstanceCurrentAction::Orphan)
                        {
                            // Orphan the instance, but don't decrease its refcount
                            instanceManager.TEMPOrphan(instanceId);
                        }
                        else if (currentAction == ParralleInstanceCurrentAction::DeferredRemoval)
                        {
                            // Drop the refcount to zero so the instance will be released
                            referenceTableMutex.lock();
                            instances[index] = nullptr;
                            referenceTableMutex.unlock();
                        }
                        else
                        {
                            // Otherwise, add a new instance
                            Instance<TestInstanceA> instance = instanceManager.FindOrCreate(instanceId, assets[index]);
                            EXPECT_NE(instance, nullptr);
                            EXPECT_EQ(instance->GetId(), instanceId);
                            EXPECT_EQ(instance->m_asset, assets[index]);

                            if (deferRemoval)
                            {
                                // Keep a reference to the instance alive so it can be removed later
                                referenceTableMutex.lock();
                                instances[index] = instance;
                                referenceTableMutex.unlock();
                            }
                        }
                    }

                    threadCount--;
                    cv.notify_one();
                });
        }

        bool timedOut = false;

        // Used to detect a deadlock.  If we wait for more than 10 seconds, it's likely a deadlock has occurred
        while (threadCount > 0 && !timedOut)
        {
            AZStd::unique_lock<AZStd::mutex> lock(mutex);
            timedOut =
                (AZStd::cv_status::timeout ==
                 cv.wait_until(lock, AZStd::chrono::steady_clock::now() + AZStd::chrono::seconds(1)));
        }

        EXPECT_TRUE(threadCount == 0) << "One or more threads appear to be deadlocked at " << timer.GetDeltaTimeInSeconds() << " seconds";

        for (auto& thread : threads)
        {
            thread.join();
        }

        keepDispatching = false;
        dispatchThread.join();

        //printf("Took %f seconds\n", timer.GetDeltaTimeInSeconds());
    }

    void ParallelCreateTest(const ParallelInstanceTestCases& testCase)
    {
        // This is the original test scenario from when InstanceDatabase was first implemented
        //                           threads, AssetIds,  seconds
        ParallelInstanceCreateHelper(8, 100, 5, testCase);

        // This value is checked in as 1 so this test doesn't take too much time, but can be increased locally to soak the test.
        const size_t attempts = 1;

        for (size_t i = 0; i < attempts; ++i)
        {
            //printf("Attempt %zu of %zu... \n", i, attempts);

            // The idea behind this series of tests is that there are two threads sharing one Instance, and both threads try to
            // create or release that instance at the same time.
            // At the time, this set of scenarios has something like a 10% failure rate.
            const uint32_t iterations = 1000;
            //                           threads, AssetIds, iterations
            ParallelInstanceCreateHelper(2, 1, iterations, testCase);
            ParallelInstanceCreateHelper(4, 1, iterations, testCase);
            ParallelInstanceCreateHelper(8, 1, iterations, testCase);
        }

        for (size_t i = 0; i < attempts; ++i)
        {
            //printf("Attempt %zu of %zu... \n", i, attempts);

            // Here we try a bunch of different threadCount:assetCount ratios to be thorough
            const uint32_t iterations = 1000;
            //                           threads, AssetIds, iterations
            ParallelInstanceCreateHelper(2, 1, iterations, testCase);
            ParallelInstanceCreateHelper(4, 1, iterations, testCase);
            ParallelInstanceCreateHelper(4, 2, iterations, testCase);
            ParallelInstanceCreateHelper(4, 4, iterations, testCase);
            ParallelInstanceCreateHelper(8, 1, iterations, testCase);
            ParallelInstanceCreateHelper(8, 2, iterations, testCase);
            ParallelInstanceCreateHelper(8, 3, iterations, testCase);
            ParallelInstanceCreateHelper(8, 4, iterations, testCase);
        }
    }

    TEST_F(InstanceDatabaseTest, ParallelInstanceCreate)
    {
        ParallelCreateTest(ParallelInstanceTestCases::Create);
    }

    TEST_F(InstanceDatabaseTest, ParallelInstanceCreateAndDeferRemoval)
    {
        ParallelCreateTest(ParallelInstanceTestCases::CreateAndDeferRemoval);
    }

    TEST_F(InstanceDatabaseTest, ParallelInstanceCreateAndOrphan)
    {
        ParallelCreateTest(ParallelInstanceTestCases::CreateAndOrphan);
    }

    TEST_F(InstanceDatabaseTest, ParallelInstanceCreateDeferRemovalAndOrphan)
    {
        ParallelCreateTest(ParallelInstanceTestCases::CreateDeferRemovalAndOrphan);
    }

    TEST_F(InstanceDatabaseTest, InstanceCreateNoDatabase)
    {
        bool m_deleted = false;

        {
            Instance<TestInstanceB> instance = aznew TestInstanceB(nullptr);
            EXPECT_FALSE(instance->GetId().IsValid());

            // Tests whether the deleter actually calls delete properly without
            // a parent database.
            instance->m_onDeleteCallback = [&m_deleted]()
            {
                m_deleted = true;
            };
        }

        EXPECT_TRUE(m_deleted);
    }

    TEST_F(InstanceDatabaseTest, InstanceCreateMultipleDatabases)
    {
        // create a second instance database.
        {
            InstanceHandler<TestInstanceB> instanceHandler;
            instanceHandler.m_createFunction = [](AssetData* assetData)
            {
                EXPECT_TRUE(azrtti_istypeof<TestAssetType>(assetData));
                return aznew TestInstanceB(static_cast<TestAssetType*>(assetData));
            };
            InstanceDatabase<TestInstanceB>::Create(azrtti_typeid<TestAssetType>(), instanceHandler);
        }

        auto& assetManager = AssetManager::Instance();
        auto& instanceDatabaseA = InstanceDatabase<TestInstanceA>::Instance();
        auto& instanceDatabaseB = InstanceDatabase<TestInstanceB>::Instance();

        {
            Asset<TestAssetType> someAsset = assetManager.CreateAsset<TestAssetType>(s_assetId0, AZ::Data::AssetLoadBehavior::Default);

            // Run the creation tests on 'A' first.

            Instance<TestInstanceA> instanceA = instanceDatabaseA.Find(s_instanceId0);
            EXPECT_EQ(instanceA, nullptr);

            instanceA = instanceDatabaseA.FindOrCreate(s_instanceId0, someAsset);
            EXPECT_NE(instanceA, nullptr);

            Instance<TestInstanceA> instanceA2 = instanceDatabaseA.FindOrCreate(s_instanceId0, someAsset);
            EXPECT_EQ(instanceA, instanceA2);

            Instance<TestInstanceA> instanceA3 = instanceDatabaseA.Find(s_instanceId0);
            EXPECT_EQ(instanceA, instanceA3);

            // Run the same test on 'B' to make sure it works independently.

            Instance<TestInstanceB> instanceB = instanceDatabaseB.Find(s_instanceId0);
            EXPECT_EQ(instanceB, nullptr);

            instanceB = instanceDatabaseB.FindOrCreate(s_instanceId0, someAsset);
            EXPECT_NE(instanceB, nullptr);

            Instance<TestInstanceB> instanceB2 = instanceDatabaseB.FindOrCreate(s_instanceId0, someAsset);
            EXPECT_EQ(instanceB, instanceB2);

            Instance<TestInstanceB> instanceB3 = instanceDatabaseB.Find(s_instanceId0);
            EXPECT_EQ(instanceB, instanceB3);
        }

        InstanceDatabase<TestInstanceB>::Destroy();
    }

} // namespace UnitTest
