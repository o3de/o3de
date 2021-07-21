/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    static const InstanceId s_instanceId0{ Uuid("{5B29FE2B-6B41-48C9-826A-C723951B0560}") };
    static const InstanceId s_instanceId1{ Uuid("{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}") };
    static const InstanceId s_instanceId2{ Uuid("{EE99215B-7AB4-4757-B8AF-F78BD4903AC4}") };
    static const InstanceId s_instanceId3{ Uuid("{D9CDAB04-D206-431E-BDC0-1DD615D56197}") };

    static const AssetId s_assetId0{ Uuid("{5B29FE2B-6B41-48C9-826A-C723951B0560}") };
    static const AssetId s_assetId1{ Uuid("{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}") };
    static const AssetId s_assetId2{ Uuid("{EE99215B-7AB4-4757-B8AF-F78BD4903AC4}") };
    static const AssetId s_assetId3{ Uuid("{D9CDAB04-D206-431E-BDC0-1DD615D56197}") };

    // test asset type
    class TestAssetType
        : public AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(TestAssetType, AZ::SystemAllocator, 0);
        AZ_RTTI(TestAssetType, "{73D60606-BDE5-44F9-9420-5649FE7BA5B8}", AssetData);

        TestAssetType()
        {
            m_status = AssetStatus::Ready;
        }
    };

    class TestInstanceA
        : public InstanceData
    {
    public:
        AZ_INSTANCE_DATA(TestInstanceA, "{65CBF1C8-F65F-4A84-8A11-B510BC435DB0}");
        AZ_CLASS_ALLOCATOR(TestInstanceA, AZ::SystemAllocator, 0);

        TestInstanceA(TestAssetType* asset)
            : m_asset{asset, AZ::Data::AssetLoadBehavior::Default}
        {}

        Asset<TestAssetType> m_asset;
    };

    class TestInstanceB
        : public InstanceData
    {
    public:
        AZ_INSTANCE_DATA(TestInstanceB, "{4ED0A8BF-7800-44B2-AC73-2CB759C61C37}");
        AZ_CLASS_ALLOCATOR(TestInstanceB, AZ::SystemAllocator, 0);

        TestInstanceB(TestAssetType* asset)
            : m_asset{asset, AZ::Data::AssetLoadBehavior::Default }
        {}

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
    class MyAssetHandler
        : public AssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MyAssetHandler, AZ::SystemAllocator, 0);

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

    class InstanceDatabaseTest
        : public AllocatorsFixture
    {
    protected:
        MyAssetHandler<TestAssetType>* m_assetHandler;
    public:

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

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

            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorsFixture::TearDown();
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

    void ParallelInstanceCreateHelper(size_t threadCountMax, size_t assetIdCount, size_t durationSeconds)
    {
        printf("Testing threads=%zu assetIds=%zu ... ", threadCountMax, assetIdCount);

        AZ::Debug::Timer timer;
        timer.Stamp();

        auto& assetManager = AssetManager::Instance();
        auto& instanceManager = InstanceDatabase<TestInstanceA>::Instance();

        AZStd::vector<Uuid> guids;
        AZStd::vector<Asset<TestAssetType>> assets;
        
        for (size_t i = 0; i < assetIdCount; ++i)
        {
            Uuid guid = Uuid::CreateRandom();

            guids.emplace_back(guid);

            // Pre-create asset so we don't attempt to load it from the catalog.
            assets.emplace_back(assetManager.CreateAsset<TestAssetType>(guid, AZ::Data::AssetLoadBehavior::Default));
        }


        AZStd::vector<AZStd::thread> threads;
        AZStd::mutex mutex;
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
            threads.emplace_back([&instanceManager, &threadCount, &cv, &guids, &assets, &durationSeconds]()
            {
                AZ::Debug::Timer timer;
                timer.Stamp();

                while(timer.GetDeltaTimeInSeconds() < durationSeconds)
                {
                    const size_t index = rand() % guids.size();
                    const Uuid uuid = guids[index];
                    const InstanceId instanceId{uuid};
                    const AssetId assetId{uuid};

                    Instance<TestInstanceA> instance = instanceManager.FindOrCreate(instanceId, Asset<TestAssetType>(assetId, azrtti_typeid<TestAssetType>()));
                    EXPECT_NE(instance, nullptr);
                    EXPECT_EQ(instance->GetId(), instanceId);
                    EXPECT_EQ(instance->m_asset, assets[index]);
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
            timedOut = (AZStd::cv_status::timeout == cv.wait_until(lock, AZStd::chrono::system_clock::now() + AZStd::chrono::seconds(durationSeconds * 2)));
        }

        EXPECT_TRUE(threadCount == 0) << "One or more threads appear to be deadlocked at " << timer.GetDeltaTimeInSeconds() << " seconds";

        for (auto& thread : threads)
        {
            thread.join();
        }

        keepDispatching = false;
        dispatchThread.join();

        printf("Took %f seconds\n", timer.GetDeltaTimeInSeconds());
    }

    TEST_F(InstanceDatabaseTest, ParallelInstanceCreate)
    {
        // This is the original test scenario from when InstanceDatabase was first implemented
        //                           threads, AssetIds,  seconds 
        ParallelInstanceCreateHelper(      8,      100,     5  );

        // This value is checked in as 1 so this test doesn't take too much time, but can be increased locally to soak the test.
        const size_t attempts = 1;

        for (size_t i = 0; i < attempts; ++i)
        {
            printf("Attempt %zu of %zu... \n", i, attempts);

            // The idea behind this series of tests is that there are two threads sharing one Instance, and both threads try to
            // create or release that instance at the same time. 
            // At the time, this set of scenarios has something like a 10% failure rate.
            const size_t duration = 2;
            //                           threads, AssetIds, seconds  
            ParallelInstanceCreateHelper(2, 1, duration);
            ParallelInstanceCreateHelper(4, 1, duration);
            ParallelInstanceCreateHelper(8, 1, duration);
        }

        for (size_t i = 0; i < attempts; ++i)
        {
            printf("Attempt %zu of %zu... \n", i, attempts);

            // Here we try a bunch of different threadCount:assetCount ratios to be thorough
            const size_t duration = 2;
            //                           threads, AssetIds, seconds  
            ParallelInstanceCreateHelper(2, 1, duration);
            ParallelInstanceCreateHelper(4, 1, duration);
            ParallelInstanceCreateHelper(4, 2, duration);
            ParallelInstanceCreateHelper(4, 4, duration);
            ParallelInstanceCreateHelper(8, 1, duration);
            ParallelInstanceCreateHelper(8, 2, duration);
            ParallelInstanceCreateHelper(8, 3, duration);
            ParallelInstanceCreateHelper(8, 4, duration);
        }
    }

    TEST_F(InstanceDatabaseTest, InstanceCreateNoDatabase)
    {
        bool m_deleted = false;

        {
            Instance<TestInstanceB> instance = aznew TestInstanceB(nullptr);
            EXPECT_FALSE(instance->GetId().IsValid());

            // Tests whether the deleter actually calls delete properly without
            // a parent database.
            instance->m_onDeleteCallback = [this, &m_deleted] () { m_deleted = true; };
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


    class InstanceDatabaseTestWithMultipleSubclasses
        : public AllocatorsFixture
    {
    protected:
        // We have "BaseAsset" with subclasses "FooAsset" and "BarAsset",
        // and corresponding "BaseInstance" with subclasses "FooInstance" and "BarInstance".
        // There is one "InstanceDatabse<BaseInstance>" that can create instances of both subtypes.

        class BaseAsset
            : public AssetData
        {
        public:
            AZ_CLASS_ALLOCATOR(BaseAsset, AZ::SystemAllocator, 0);
            AZ_RTTI(FooAsset, "{35B443A6-D8ED-4C3C-A3F0-D642251F0AA5}", AssetData);

            BaseAsset()
            {
                m_status = AssetStatus::Ready;
            }
        };

        class BaseInstance
            : public InstanceData
        {
        public:
            AZ_INSTANCE_DATA(BaseInstance, "{EFEC3406-2CB7-462E-A676-C22177E143E6}");
            AZ_CLASS_ALLOCATOR(BaseInstance, AZ::SystemAllocator, 0);

            BaseInstance(BaseAsset* asset)
                : m_asset{ asset, AZ::Data::AssetLoadBehavior::Default }
            {}

            Asset<BaseAsset> m_asset;
        };

        class FooAsset
            : public BaseAsset
        {
        public:
            AZ_CLASS_ALLOCATOR(FooAsset, AZ::SystemAllocator, 0);
            AZ_RTTI(FooAsset, "{74BAE278-3DCA-4ADD-807E-2A6873F9EA3C}", BaseAsset);
        };

        class BarAsset
            : public BaseAsset
        {
        public:
            AZ_CLASS_ALLOCATOR(BarAsset, AZ::SystemAllocator, 0);
            AZ_RTTI(FooAsset, "{2BCD66F5-768B-4569-9FC2-DE92ABC9C0BF}", BaseAsset);
        };

        class FooInstance
            : public BaseInstance
        {
        public:
            AZ_RTTI(FooInstance, "{B5487509-5518-4591-AC96-03E623A584B7}", BaseInstance);
            AZ_CLASS_ALLOCATOR(FooInstance, AZ::SystemAllocator, 0);

            FooInstance(BaseAsset* asset)
                : BaseInstance(asset)
            {
                EXPECT_TRUE(azrtti_typeid<FooAsset>() == asset->GetType());
            }
        };

        class BarInstance
            : public BaseInstance
        {
        public:
            AZ_RTTI(BarInstance, "{CE9C844A-625D-4899-B7DB-8127D4618D25}", BaseInstance);
            AZ_CLASS_ALLOCATOR(BarInstance, AZ::SystemAllocator, 0);

            BarInstance(BaseAsset* asset)
                : BaseInstance(asset)
            {
                EXPECT_TRUE(azrtti_typeid<BarAsset>() == asset->GetType());
            }
        };

        MyAssetHandler<FooAsset> m_fooAssetHandler;
        MyAssetHandler<BarAsset> m_barAssetHandler;

    public:

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            AllocatorInstance<PoolAllocator>::Create();
            AllocatorInstance<ThreadPoolAllocator>::Create();

            // create the asset database
            {
                AssetManager::Descriptor desc;
                AssetManager::Create(desc);
            }

            // create the instance database
            {
                InstanceDatabase<BaseInstance>::Create(azrtti_typeid<BaseAsset>());

                InstanceHandler<BaseInstance> fooHandler;
                fooHandler.m_createFunction = [](AssetData* assetData)
                {
                    EXPECT_TRUE(azrtti_istypeof<FooAsset>(assetData));
                    return aznew FooInstance(static_cast<FooAsset*>(assetData));
                };
                InstanceDatabase<BaseInstance>::Instance().AddHandler(azrtti_typeid<FooAsset>(), fooHandler);

                // Using a different overload of AddHandler()
                InstanceDatabase<BaseInstance>::Instance().AddHandler(azrtti_typeid<BarAsset>(), [](AssetData* assetData)
                {
                    EXPECT_TRUE(azrtti_istypeof<BarAsset>(assetData));
                    return aznew BarInstance(static_cast<BarAsset*>(assetData));
                });
            }

            AssetManager::Instance().RegisterHandler(&m_fooAssetHandler, AzTypeInfo<FooAsset>::Uuid());
            AssetManager::Instance().RegisterHandler(&m_barAssetHandler, AzTypeInfo<BarAsset>::Uuid());
        }

        void TearDown() override
        {
            AssetManager::Instance().UnregisterHandler(&m_fooAssetHandler);
            AssetManager::Instance().UnregisterHandler(&m_barAssetHandler);
            AssetManager::Destroy();

            InstanceDatabase<BaseInstance>::Destroy();

            AllocatorInstance<ThreadPoolAllocator>::Destroy();
            AllocatorInstance<PoolAllocator>::Destroy();
            AllocatorsFixture::TearDown();
        }
    };

    TEST_F(InstanceDatabaseTestWithMultipleSubclasses, InstanceCreate)
    {
        auto& assetManager = AssetManager::Instance();
        auto& instanceDatabase = InstanceDatabase<BaseInstance>::Instance();

        Asset<FooAsset> fooAsset = assetManager.CreateAsset<FooAsset>(s_assetId0, AZ::Data::AssetLoadBehavior::Default);
        Asset<BarAsset> barAsset = assetManager.CreateAsset<BarAsset>(s_assetId1, AZ::Data::AssetLoadBehavior::Default);

        // Run the creation tests on 'A' first.

        Instance<BaseInstance> fooInstanceA = instanceDatabase.Find(s_instanceId0);
        EXPECT_EQ(fooInstanceA, nullptr);

        Instance<BaseInstance> barInstanceA = instanceDatabase.Find(s_instanceId1);
        EXPECT_EQ(barInstanceA, nullptr);

        fooInstanceA = instanceDatabase.FindOrCreate(s_instanceId0, fooAsset);
        EXPECT_NE(fooInstanceA, nullptr);
        EXPECT_EQ(fooInstanceA->m_asset, fooAsset);
        EXPECT_TRUE(azrtti_typeid<FooInstance>() == fooInstanceA->RTTI_GetType());
        EXPECT_EQ(fooInstanceA, instanceDatabase.Find(s_instanceId0));

        barInstanceA = instanceDatabase.FindOrCreate(s_instanceId1, barAsset);
        EXPECT_NE(barInstanceA, nullptr);
        EXPECT_EQ(barInstanceA->m_asset, barAsset);
        EXPECT_TRUE(azrtti_typeid<BarInstance>() == barInstanceA->RTTI_GetType());
        EXPECT_EQ(barInstanceA, instanceDatabase.Find(s_instanceId1));

        // Run the same test on 'B' to make sure it works independently.

        Instance<BaseInstance> fooInstanceB = instanceDatabase.Find(s_instanceId2);
        EXPECT_EQ(fooInstanceB, nullptr);

        Instance<BaseInstance> barInstanceB = instanceDatabase.Find(s_instanceId3);
        EXPECT_EQ(barInstanceB, nullptr);

        fooInstanceB = instanceDatabase.FindOrCreate(s_instanceId2, fooAsset);
        EXPECT_NE(fooInstanceB, nullptr);
        EXPECT_EQ(fooInstanceB->m_asset, fooAsset);
        EXPECT_TRUE(azrtti_typeid<FooInstance>() == fooInstanceB->RTTI_GetType());
        EXPECT_EQ(fooInstanceB, instanceDatabase.Find(s_instanceId2));

        barInstanceB = instanceDatabase.FindOrCreate(s_instanceId3, barAsset);
        EXPECT_NE(barInstanceB, nullptr);
        EXPECT_EQ(barInstanceB->m_asset, barAsset);
        EXPECT_TRUE(azrtti_typeid<BarInstance>() == barInstanceB->RTTI_GetType());
        EXPECT_EQ(barInstanceB, instanceDatabase.Find(s_instanceId3));

        // Make sure the instances are unique

        EXPECT_NE(fooInstanceA, fooInstanceB);
        EXPECT_NE(barInstanceA, barInstanceB);
    }
    
    TEST_F(InstanceDatabaseTestWithMultipleSubclasses, TestError_AddHandler_AssetTypeIsNotSubclass)
    {
        MyAssetHandler<TestAssetType> testAssetHandler;
        AssetManager::Instance().RegisterHandler(&testAssetHandler, azrtti_typeid<TestAssetType>());
        
        // Register an instance handler with an unrelated asset type. This can't actually 
        // check the AssetType yet because all it has are AssetType GUIDs, no actual data.
        {
            InstanceHandler<BaseInstance> instanceHandler;
            instanceHandler.m_createFunction = [](AssetData* assetData)
            {
                return aznew BaseInstance(static_cast<BaseAsset*>(assetData));
            };

            AssetType unrelatedAssetType = azrtti_typeid<TestAssetType>();

            InstanceDatabase<BaseInstance>::Instance().AddHandler(unrelatedAssetType, instanceHandler);
        }

        // Try to use the unrelated handler. This is where we'll actually get an error.
        {
            AZ_TEST_START_ASSERTTEST;

            Asset<TestAssetType> testAsset = AssetManager::Instance().CreateAsset<TestAssetType>(s_assetId0, AZ::Data::AssetLoadBehavior::Default);
            
            EXPECT_EQ(nullptr, InstanceDatabase<BaseInstance>::Instance().FindOrCreate(s_instanceId0, testAsset));

            AZ_TEST_STOP_ASSERTTEST(1);
        }

        AssetManager::Instance().UnregisterHandler(&testAssetHandler);
    }

    TEST_F(InstanceDatabaseTestWithMultipleSubclasses, TestError_AddHandler_AlreadyExists)
    {
        InstanceHandler<BaseInstance> instanceHandler;
        instanceHandler.m_createFunction = [](AssetData*)
        {
            return nullptr; // Doesn't matter
        };

        AZ_TEST_START_ASSERTTEST;

        // The SetUp() function already registered a handler for FooAsset so this should fail
        InstanceDatabase<BaseInstance>::Instance().AddHandler(azrtti_typeid<FooAsset>(), instanceHandler);
        InstanceDatabase<BaseInstance>::Instance().AddHandler(azrtti_typeid<FooAsset>(), [](AssetData*) { return nullptr; });

        AZ_TEST_STOP_ASSERTTEST(2);
    }
}
