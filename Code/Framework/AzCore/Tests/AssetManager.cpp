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
#include <Tests/Asset/TestAssetTypes.h>
#include <Tests/SerializeContextFixture.h>
#include <Tests/TestCatalog.h>

using namespace AZ;
using namespace AZ::Data;

namespace UnitTest
{

    class AssetManagerSystemTest
        : public BaseAssetManagerTest
    {
    public:
        // Initialize the Job Manager with 2 threads for the Asset Manager to use.
        size_t GetNumJobManagerThreads() const override { return 2; }
    };

#if !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_CREATE_DESTROY_TEST
    TEST_F(AssetManagerSystemTest, AssetManager_CreateDestroy_TriviallyWorks)
    {
        // Trvially validate that we can create and destroy an asset manager instance, and that it's only ready while it's created.

        // Before creation, IsReady() should be false.
        EXPECT_FALSE(AssetManager::IsReady());

        AssetManager::Descriptor desc;
        AssetManager::Create(desc);

        // After creation, the system should be ready and queryable via Instance().
        EXPECT_TRUE(AssetManager::IsReady());
        AssetManager::Instance();

        AssetManager::Destroy();

        // After destruction, these should fail again
        EXPECT_FALSE(AssetManager::IsReady());
    }
#endif // !AZ_TRAIT_DISABLE_FAILED_ASSET_MANAGER_CREATE_DESTROY_TEST

    TEST_F(AssetManagerSystemTest, AssetManager_SetInstance_TriviallyWorks)
    {
        // There shouldn't be an instance yet.
        EXPECT_FALSE(AssetManager::IsReady());

        // Create an instance and set it.
        AssetManager::Descriptor desc;
        auto testManager = aznew TestAssetManager(desc);
        EXPECT_TRUE(AssetManager::SetInstance(testManager));

        // Verify that the instance we get back is the one we created.
        auto& goodInstance = AssetManager::Instance();
        EXPECT_EQ(&goodInstance, testManager);

        AssetManager::Destroy();
    }

    TEST_F(AssetManagerSystemTest, AssetManager_SetInstance_AssertsWhenAlreadyCreated)
    {
        // Create an asset manager instance
        AssetManager::Descriptor desc;
        auto testManager = aznew TestAssetManager(desc);
        EXPECT_TRUE(AssetManager::SetInstance(testManager));

        // Create a second asset manager instance.  This will error because it's connecting two asset managers to the asset manager bus.
        AZ_TEST_START_TRACE_SUPPRESSION;
        auto testManager2 = aznew TestAssetManager(desc);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        // Try setting the instance without clearing the old one.  This will assert.
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_TRUE(AssetManager::SetInstance(testManager2));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        // Verify that the instance we get back is the second one we created.
        auto& instance = AssetManager::Instance();
        EXPECT_EQ(&instance, testManager2);

        // Delete the first instance here, since the second SetInstance call caused us to leak the memory.
        delete testManager;

        // Let the AssetManager clean up the second instance.
        AssetManager::Destroy();
    }

    TEST_F(AssetManagerSystemTest, AssetCallbacks_Clear)
    {
        // Helper function that always asserts. Used to make sure that clearing asset callbacks actually clears them.
        auto testAssetCallbacksClear = []()
        {
            EXPECT_TRUE(false);
        };

        AssetBusCallbacks* assetCB1 = aznew AssetBusCallbacks;

        // Test clearing the callbacks (using bind allows us to ignore all arguments)
        assetCB1->SetCallbacks(
            /*OnAssetReady*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetMoved*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetReloaded*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetSaved*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetUnloaded*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetError*/ AZStd::bind(testAssetCallbacksClear),
            /*OnAssetCanceled*/ AZStd::bind(testAssetCallbacksClear));
        assetCB1->ClearCallbacks();
        // Invoke all callback functions to make sure nothing is registered anymore.
        assetCB1->OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData>());
        assetCB1->OnAssetMoved(AZ::Data::Asset<AZ::Data::AssetData>(), nullptr);
        assetCB1->OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData>());
        assetCB1->OnAssetSaved(AZ::Data::Asset<AZ::Data::AssetData>(), true);
        assetCB1->OnAssetUnloaded(AZ::Data::AssetId(), AZ::Data::AssetType());
        assetCB1->OnAssetError(AZ::Data::Asset<AZ::Data::AssetData>());
        assetCB1->OnAssetCanceled(AZ::Data::AssetId());

        delete assetCB1;
    }

    class AssetManagerShutdownTest
        : public BaseAssetManagerTest
    {
    protected:
        static inline const AZ::Uuid MyAsset1Id{ "{5B29FE2B-6B41-48C9-826A-C723951B0560}" };
        static inline const AZ::Uuid MyAsset2Id{ "{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}" };
        static inline const AZ::Uuid MyAsset3Id{ "{622C3FC9-5AE2-4E52-AFA2-5F7095ADAB53}" };
        static inline const AZ::Uuid MyAsset4Id{ "{EE99215B-7AB4-4757-B8AF-F78BD4903AC4}" };
        static inline const AZ::Uuid MyAsset5Id{ "{D9CDAB04-D206-431E-BDC0-1DD615D56197}" };
        static inline const AZ::Uuid MyAsset6Id{ "{B2F139C3-5032-4B52-ADCA-D52A8F88E043}" };

        DataDrivenHandlerAndCatalog* m_assetHandlerAndCatalog;
        bool m_leakExpected = false;

        // Initialize the Job Manager with 2 threads for the Asset Manager to use.
        size_t GetNumJobManagerThreads() const override { return 2; }

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

            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset1Id, "MyAsset1.txt");
            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset2Id, "MyAsset2.txt");
            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset3Id, "MyAsset3.txt");
            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset4Id, "MyAsset4.txt");
            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset5Id, "MyAsset5.txt");
            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset6Id, "MyAsset6.txt");

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
            WriteAssetToDisk("MyAsset4.txt", MyAsset4Id.ToString<AZStd::string>().c_str());
            WriteAssetToDisk("MyAsset5.txt", MyAsset5Id.ToString<AZStd::string>().c_str());
            WriteAssetToDisk("MyAsset6.txt", MyAsset6Id.ToString<AZStd::string>().c_str());

            m_leakExpected = false;
        }

        void TearDown() override
        {
            // There is no call to AssetManager::Destroy here because it will be called by the various unit tests using this class.
            // Also, m_assetHandlerAndCatalog will either get deleted in the unit tests or by AssetManager::Destroy.

            if (m_leakExpected)
            {
                AZ::AllocatorManager::Instance().SetAllocatorLeaking(true);
            }

            BaseAssetManagerTest::TearDown();
        }

        void SetLeakExpected()
        {
            m_leakExpected = true;
        }
    };

    TEST_F(AssetManagerShutdownTest, AssetManagerShutdown_AsyncJobsInQueue_OK)
    {
        {
            Asset<AssetWithCustomData> asset1 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset2 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset2Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset3 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset4 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset4Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset5 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset5Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset6 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset6Id, AZ::Data::AssetLoadBehavior::Default);
        }

        // destroy asset manager
        AssetManager::Destroy();

    }

    TEST_F(AssetManagerShutdownTest, AssetManagerShutdown_AsyncJobsInQueueWithDelay_OK)
    {
        {
            Asset<AssetWithCustomData> asset1 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset2 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset2Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset3 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset4 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset4Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset5 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset5Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset6 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset6Id, AZ::Data::AssetLoadBehavior::Default);
        }

        // this should ensure that some jobs are actually running, while some are in queue
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));

        // destroy asset manager
        AssetManager::Destroy();

    }

    TEST_F(AssetManagerShutdownTest, AssetManagerShutdown_UnregisteringHandler_WhileJobsFlight_OK)
    {
        {
            Asset<AssetWithCustomData> asset1 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset2 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset2Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset3 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset4 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset4Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset5 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset5Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset6 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset6Id, AZ::Data::AssetLoadBehavior::Default);
        }

        // this should ensure that some jobs are actually running, while some are in queue
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));

        AssetManager::Instance().PrepareShutDown();

        AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);
        AssetManager::Instance().UnregisterCatalog(m_assetHandlerAndCatalog);

        // we have to manually delete the handler since we have already unregistered the handler
        delete m_assetHandlerAndCatalog;

        // destroy asset manager
        AssetManager::Destroy();

    }

    TEST_F(AssetManagerShutdownTest, AssetManagerShutdown_UnregisteringHandler_WhileJobsFlight_Assert)
    {
        {
            Asset<AssetWithCustomData> asset1 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset2 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset2Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset3 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset3Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset4 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset4Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset5 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset5Id, AZ::Data::AssetLoadBehavior::Default);
            Asset<AssetWithCustomData> asset6 = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset6Id, AZ::Data::AssetLoadBehavior::Default);

            // this should ensure that some jobs are actually running, while some are in queue
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(5));

            AssetManager::Instance().PrepareShutDown();
            // we are unregistering the handler that has still not destroyed all of its active assets
            AZ_TEST_START_TRACE_SUPPRESSION;
            AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);
            // unregistering should have caused an error for every one of those 6 assets.
            AZ_TEST_STOP_TRACE_SUPPRESSION(6);
            AssetManager::Instance().UnregisterCatalog(m_assetHandlerAndCatalog);
            AZ_TEST_START_TRACE_SUPPRESSION;
        }
        AZ_TEST_STOP_TRACE_SUPPRESSION(6); // all the above assets ref count will go to zero here but we have already unregistered the handler

        // we have to manually delete the handler since we have already unregistered the handler
        // we do not expect asserts here as they will have already notified of problems up above.
        delete m_assetHandlerAndCatalog;

        // destroy asset manager
        AssetManager::Destroy();

        SetLeakExpected();
    }


    class MyAssetActiveAssetCountHandler
        : public AssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MyAssetActiveAssetCountHandler, AZ::SystemAllocator, 0);

        //////////////////////////////////////////////////////////////////////////
        // AssetHandler
        AssetPtr CreateAsset(const AssetId& id, const AssetType& type) override
        {
            (void)id;
            EXPECT_TRUE(type == AzTypeInfo<AssetWithCustomData>::Uuid());

            return aznew AssetWithCustomData(id);
        }
        Data::AssetHandler::LoadResult LoadAssetData(const Asset<AssetData>& asset, AZStd::shared_ptr<AssetDataStream> stream,
            const AssetFilterCB& assetLoadFilterCB) override
        {
            (void)assetLoadFilterCB;
            EXPECT_TRUE(asset.GetType() == AzTypeInfo<AssetWithCustomData>::Uuid());
            EXPECT_TRUE(asset.Get() != nullptr && asset.Get()->GetType() == AzTypeInfo<AssetWithCustomData>::Uuid());
            size_t assetDataSize = static_cast<size_t>(stream->GetLength());
            AssetWithCustomData* myAsset = asset.GetAs<AssetWithCustomData>();
            myAsset->m_data = reinterpret_cast<char*>(azmalloc(assetDataSize + 1));
            AZStd::string input = AZStd::string::format("Asset<id=%s, type=%s>", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetType().ToString<AZStd::string>().c_str());
            stream->Read(assetDataSize, myAsset->m_data);
            myAsset->m_data[assetDataSize] = 0;

            return (azstricmp(input.c_str(), myAsset->m_data) == 0) ?
                Data::AssetHandler::LoadResult::LoadComplete :
                Data::AssetHandler::LoadResult::Error;
        }
        bool SaveAssetData(const Asset<AssetData>& asset, IO::GenericStream* stream) override
        {
            EXPECT_TRUE(asset.GetType() == AzTypeInfo<AssetWithCustomData>::Uuid());
            AZStd::string output = AZStd::string::format("Asset<id=%s, type=%s>", asset.GetId().ToString<AZStd::string>().c_str(), asset.GetType().ToString<AZStd::string>().c_str());
            stream->Write(output.size(), output.c_str());
            return true;
        }
        void DestroyAsset(AssetPtr ptr) override
        {
            EXPECT_TRUE(ptr->GetType() == AzTypeInfo<AssetWithCustomData>::Uuid());
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AssetType>& assetTypes) override
        {
            assetTypes.push_back(AzTypeInfo<AssetWithCustomData>::Uuid());
        }
    };

    struct MyAssetHolder
    {
        AZ_TYPE_INFO(MyAssetHolder, "{1DA71115-547B-4f32-B230-F3C70608AD68}");
        AZ_CLASS_ALLOCATOR(MyAssetHolder, AZ::SystemAllocator, 0);
        Asset<AssetWithCustomData>  m_asset1;
        Asset<AssetWithCustomData>  m_asset2;
    };

    class AssetManagerTest
        : public BaseAssetManagerTest
    {
    protected:
        DataDrivenHandlerAndCatalog* m_assetHandlerAndCatalog;
        TestAssetManager* m_testAssetManager;
    public:

        static inline const AZ::Uuid MyAsset1Id{ "{5B29FE2B-6B41-48C9-826A-C723951B0560}" };
        static inline const AZ::Uuid MyAsset2Id{ "{BD354AE5-B5D5-402A-A12E-BE3C96F6522B}" };
        static inline const AZ::Uuid MyAsset3Id{ "{8398759D-5D84-4E71-B9E2-69F3C0822A30}" };

        // Initialize the Job Manager with a single thread for the Asset Manager to use.
        size_t GetNumJobManagerThreads() const override { return 1; }

        void SetUp() override
        {
            BaseAssetManagerTest::SetUp();

            // create the database
            AssetManager::Descriptor desc;
            m_testAssetManager = aznew TestAssetManager(desc);
            AssetManager::SetInstance(m_testAssetManager);

            // create and register an asset handler
            m_assetHandlerAndCatalog = aznew DataDrivenHandlerAndCatalog;
            m_assetHandlerAndCatalog->m_context = m_serializeContext;

            AssetWithCustomData::Reflect(*m_serializeContext);
            EmptyAssetWithInstanceCount::Reflect(*m_serializeContext);

            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset1Id, "MyAsset1.txt");
            m_assetHandlerAndCatalog->AddAsset<AssetWithCustomData>(MyAsset2Id, "MyAsset2.txt");
            m_assetHandlerAndCatalog->AddAsset<EmptyAssetWithInstanceCount>(MyAsset3Id, "MyAsset3.txt");
            AZStd::vector<AssetType> types;
            m_assetHandlerAndCatalog->GetHandledAssetTypes(types);
            for (const auto& type : types)
            {
                AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, type);
                AssetManager::Instance().RegisterCatalog(m_assetHandlerAndCatalog, type);
            }

            WriteAssetToDisk("MyAsset1.txt", MyAsset1Id.ToString<AZStd::string>().c_str());
            WriteAssetToDisk("MyAsset2.txt", MyAsset2Id.ToString<AZStd::string>().c_str());
        }

        void TearDown() override
        {
            // Manually release the handler
            AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);
            AssetManager::Instance().UnregisterCatalog(m_assetHandlerAndCatalog);
            delete m_assetHandlerAndCatalog;

            // destroy the database
            AssetManager::Destroy();

            BaseAssetManagerTest::TearDown();
        }

        void OnLoadedClassReady(void* classPtr, const Uuid& /*classId*/)
        {
            MyAssetHolder* assetHolder = reinterpret_cast<MyAssetHolder*>(classPtr);
            EXPECT_TRUE(assetHolder->m_asset1 && assetHolder->m_asset2);
            delete assetHolder;
        }

        void SaveObjects(ObjectStream* writer, MyAssetHolder* assetHolder)
        {
            bool success = true;
            success = writer->WriteClass(assetHolder);
            EXPECT_TRUE(success);
        }

        void OnDone(ObjectStream::Handle handle, bool success, bool* done)
        {
            (void)handle;
            EXPECT_TRUE(success);
            *done = true;
        }
    };


    // this test makes sure that saving and loading asset data remains symmetrical and does not lose fields.

    void Test_AssetSerialization(AssetId idToUse, AZ::DataStream::StreamType typeToUse)
    {
        SerializeContext context;
        AssetWithAssetReference::Reflect(context);
        AssetWithAssetReference myRef;
        AssetWithAssetReference myRefEmpty; // we always test with an empty (Default) ref too.
        AssetWithAssetReference myRef2; // to be read into

        // Create an asset with a fake asset reference, and set it not to load because it's fake
        {
            Asset<AssetWithCustomData> assetRef;
            ASSERT_TRUE(assetRef.Create(idToUse, AZ::Data::AssetLoadBehavior::NoLoad, false));
            myRef.m_asset = assetRef;
            EXPECT_EQ(myRef.m_asset.GetType(), azrtti_typeid<AssetWithCustomData>());
        }

        // Set the load behavior on the empty ref to Default instead of NoLoad.
        myRefEmpty.m_asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::Default);

        char memBuffer[4096];
        {
            // we are scoping the memory stream to avoid detritus staying behind in it.
            // let's not be nice about this.  Put garbage in the buffer so that it doesn't get away with
            // not checking the length of the incoming stream.
            memset(memBuffer, '<', AZ_ARRAY_SIZE(memBuffer));
            AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);
            ASSERT_TRUE(Utils::SaveObjectToStream(memStream, typeToUse, &myRef, &context));
            EXPECT_GT(memStream.GetLength(), 0); // something should have been written.
            memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            EXPECT_TRUE(Utils::LoadObjectFromStreamInPlace(memStream, myRef2, &context));

            EXPECT_EQ(myRef2.m_asset.GetType(), azrtti_typeid<AssetWithCustomData>());
            EXPECT_EQ(myRef2.m_asset.GetId(), idToUse);
            EXPECT_EQ(myRef2.m_asset.GetAutoLoadBehavior(), myRef.m_asset.GetAutoLoadBehavior());
        }

        {
            memset(memBuffer, '<', AZ_ARRAY_SIZE(memBuffer));
            AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);
            memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            ASSERT_TRUE(Utils::SaveObjectToStream(memStream, typeToUse, &myRefEmpty, &context));
            EXPECT_GT(memStream.GetLength(), 0); // something should have been written.

            memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            ASSERT_TRUE(Utils::LoadObjectFromStreamInPlace(memStream, myRef2, &context));

            EXPECT_EQ(myRef2.m_asset.GetType(), azrtti_typeid<AssetData>());
            EXPECT_EQ(myRef2.m_asset.GetId(), myRefEmpty.m_asset.GetId());
            EXPECT_EQ(myRef2.m_asset.GetAutoLoadBehavior(), myRefEmpty.m_asset.GetAutoLoadBehavior());
        }
    }

    TEST_F(AssetManagerTest, AssetSerializerTest)
    {
        auto assets = {
            AssetId("{3E971FD2-DB5F-4617-9061-CCD3606124D0}", 0x707a11ed),
            AssetId("{A482C6F3-9943-4C19-8970-974EFF6F1389}", 0x00000000),
        };

        for (int streamTypeIndex = 0; streamTypeIndex < static_cast<int>(AZ::DataStream::ST_MAX); ++streamTypeIndex)
        {
            for (auto asset : assets)
            {
                Test_AssetSerialization(asset, static_cast<AZ::DataStream::StreamType>(streamTypeIndex));
            }
        }
    }

    // Test for serialize class data which contains a reference asset which handler wasn't registered to AssetManager
    TEST_F(AssetManagerTest, AssetSerializerAssetReferenceTest)
    {
        auto assetId = AssetId("{3E971FD2-DB5F-4617-9061-CCD3606124D0}", 0);
        
        SerializeContext context;
        AssetWithAssetReference::Reflect(context);
        char memBuffer[4096];
        AZ::IO::MemoryStream memStream(memBuffer, AZ_ARRAY_SIZE(memBuffer), 0);


        // generate the data stream for the object contains a reference of asset
        AssetWithAssetReference toSave;
        {
            Asset<AssetWithCustomData> assetRef;
            ASSERT_TRUE(assetRef.Create(assetId, AZ::Data::AssetLoadBehavior::PreLoad, false));
            toSave.m_asset = assetRef;
        }
        Utils::SaveObjectToStream(memStream, DataStream::StreamType::ST_BINARY, &toSave, &context);
        toSave.m_asset.Release();

        // Unregister asset handler for AssetWithCustomData
        AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);

        Utils::FilterDescriptor desc;
        AssetWithAssetReference toRead;

        // return true if loading with none filter or ignore unknown classes filter
        memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZ_TEST_START_TRACE_SUPPRESSION;
        desc.m_flags = 0;
        ASSERT_TRUE(Utils::LoadObjectFromStreamInPlace(memStream, toRead, &context, desc));
        // LoadObjectFromStreamInPlace generates two errors. One is can't find the handler. Another one is can't load referenced asset.
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        // return true if loading with ignore unknown classes filter
        memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZ_TEST_START_TRACE_SUPPRESSION;
        desc.m_flags = ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES;
        ASSERT_TRUE(Utils::LoadObjectFromStreamInPlace(memStream, toRead, &context, desc));
        // LoadObjectFromStreamInPlace generates two errors. One is can't find the handler. Another one is can't load referenced asset.
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);

        // return false if loading with strict filter
        memStream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        AZ_TEST_START_TRACE_SUPPRESSION;
        desc.m_flags = ObjectStream::FILTERFLAG_STRICT;
        ASSERT_FALSE(Utils::LoadObjectFromStreamInPlace(memStream, toRead, &context, desc));
        // LoadObjectFromStreamInPlace generates two errors. One is can't find the handler. Another one is can't load referenced asset.
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }

    TEST_F(AssetManagerTest, AssetCanBeReleased)
    {
        const auto assetId = AssetId(Uuid::CreateRandom());
        Asset<EmptyAssetWithInstanceCount> asset = m_testAssetManager->CreateAsset<EmptyAssetWithInstanceCount>(assetId);

        EXPECT_NE(asset.Get(), nullptr);

        asset.Release();

        EXPECT_EQ(asset.Get(), nullptr);
        EXPECT_EQ(asset.GetId(), assetId);
        EXPECT_EQ(asset.GetType(), AzTypeInfo<EmptyAssetWithInstanceCount>::Uuid());
    }

    TEST_F(AssetManagerTest, AssetCanBeReset)
    {
        const auto assetId = AssetId(Uuid::CreateRandom());
        Asset<EmptyAssetWithInstanceCount> asset = m_testAssetManager->CreateAsset<EmptyAssetWithInstanceCount>(assetId);

        EXPECT_NE(asset.Get(), nullptr);

        asset.Reset();

        EXPECT_EQ(asset.Get(), nullptr);
        EXPECT_FALSE(asset.GetId().IsValid());
        EXPECT_TRUE(asset.GetType().IsNull());
    }

    TEST_F(AssetManagerTest, AssetPtrRefCount)
    {
        // Asset ptr tests.
        Asset<EmptyAssetWithInstanceCount> someAsset = AssetManager::Instance().CreateAsset<EmptyAssetWithInstanceCount>(Uuid::CreateRandom(), AZ::Data::AssetLoadBehavior::Default);
        EmptyAssetWithInstanceCount* someData = someAsset.Get();

        EXPECT_EQ(EmptyAssetWithInstanceCount::s_instanceCount, 1);

        // Construct with flags
        {
            Asset<EmptyAssetWithInstanceCount> assetWithFlags(AssetLoadBehavior::PreLoad);
            EXPECT_TRUE(!assetWithFlags.GetId().IsValid());
            EXPECT_EQ(assetWithFlags.GetType(), AzTypeInfo<EmptyAssetWithInstanceCount>::Uuid());
            EXPECT_EQ(assetWithFlags.GetAutoLoadBehavior(), AssetLoadBehavior::PreLoad);
        }

        // Construct w/ data (verify id & type)
        {
            Asset<EmptyAssetWithInstanceCount> assetWithData(someData, AZ::Data::AssetLoadBehavior::Default);
            EXPECT_EQ(someData->GetUseCount(), 2);
            EXPECT_TRUE(assetWithData.GetId().IsValid());
            EXPECT_EQ(assetWithData.GetType(), AzTypeInfo<EmptyAssetWithInstanceCount>::Uuid());
            EXPECT_EQ(assetWithData.GetAutoLoadBehavior(), AssetLoadBehavior::Default);
        }

        // Copy construct (verify id & type, acquisition of new data)
        {
            Asset<EmptyAssetWithInstanceCount> assetWithData = AssetManager::Instance().CreateAsset<EmptyAssetWithInstanceCount>(Uuid::CreateRandom(), AZ::Data::AssetLoadBehavior::Default);
            EmptyAssetWithInstanceCount* newData = assetWithData.Get();
            Asset<EmptyAssetWithInstanceCount> assetWithData2(assetWithData);

            // Underlying data should be used twice through a reference in both assets.
            EXPECT_EQ(assetWithData->GetUseCount(), 2);
            EXPECT_EQ(assetWithData.Get(), newData);
            EXPECT_EQ(assetWithData.Get(), assetWithData2.Get());
            // Every other value should also be copied between the two assets
            EXPECT_EQ(assetWithData.GetId(), assetWithData2.GetId());
            EXPECT_EQ(assetWithData.GetType(), assetWithData2.GetType());
            EXPECT_EQ(assetWithData.GetAutoLoadBehavior(), assetWithData2.GetAutoLoadBehavior());
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();

        EXPECT_EQ(EmptyAssetWithInstanceCount::s_instanceCount, 1);

        // Move construct (verify id & type, release of old data, acquisition of new)
        {
            Asset<EmptyAssetWithInstanceCount> assetWithData = AssetManager::Instance().CreateAsset<EmptyAssetWithInstanceCount>(Uuid::CreateRandom(), AZ::Data::AssetLoadBehavior::Default);
            EmptyAssetWithInstanceCount* origData = assetWithData.Get();
            AssetId origId = assetWithData.GetId();
            AssetType origType = assetWithData.GetType();

            Asset<EmptyAssetWithInstanceCount> assetWithData2(AZStd::move(assetWithData));

            // The original asset should only have default values now
            EXPECT_EQ(assetWithData.Get(), nullptr);
            EXPECT_EQ(assetWithData.GetId(), AssetId());
            EXPECT_EQ(assetWithData.GetType(), AssetType::CreateNull());
            EXPECT_EQ(assetWithData.GetAutoLoadBehavior(), AssetLoadBehavior::Default);
            // The new asset should have all of the original asset's values
            EXPECT_EQ(assetWithData2.Get(), origData);
            EXPECT_EQ(assetWithData2.GetId(), origId);
            EXPECT_EQ(assetWithData2.GetType(), origType);
            EXPECT_EQ(assetWithData2.GetAutoLoadBehavior(), assetWithData.GetAutoLoadBehavior());
            // The underlying data should only have one reference, which is the new asset.
            EXPECT_EQ(origData->GetUseCount(), 1);
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();
        EXPECT_EQ(EmptyAssetWithInstanceCount::s_instanceCount, 1);

        // Copy from r-value (verify id & type, release of old data, acquisition of new)
        {
            Asset<EmptyAssetWithInstanceCount> assetWithData = AssetManager::Instance().CreateAsset<EmptyAssetWithInstanceCount>(Uuid::CreateRandom(), AZ::Data::AssetLoadBehavior::Default);
            EmptyAssetWithInstanceCount* newData = assetWithData.Get();
            EXPECT_EQ(EmptyAssetWithInstanceCount::s_instanceCount, 2);
            Asset<EmptyAssetWithInstanceCount> assetWithData2 = AssetManager::Instance().CreateAsset<EmptyAssetWithInstanceCount>(Uuid::CreateRandom(), AZ::Data::AssetLoadBehavior::Default);
            EXPECT_EQ(EmptyAssetWithInstanceCount::s_instanceCount, 3);
            assetWithData2 = AZStd::move(assetWithData);

            // Allow the asset manager to purge assets on the dead list.
            AssetManager::Instance().DispatchEvents();
            EXPECT_EQ(EmptyAssetWithInstanceCount::s_instanceCount, 2);

            EXPECT_EQ(assetWithData.Get(), nullptr);
            EXPECT_EQ(assetWithData2.Get(), newData);
            EXPECT_EQ(newData->GetUseCount(), 1);
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();
        EXPECT_EQ(EmptyAssetWithInstanceCount::s_instanceCount, 1);

        {
            // Test copy of a different, but compatible asset type to make sure it takes on the correct info.
            Asset<AssetData> genericAsset(someData, AZ::Data::AssetLoadBehavior::Default);
            EXPECT_TRUE(genericAsset.GetId().IsValid());
            EXPECT_EQ(genericAsset.GetType(), AzTypeInfo<EmptyAssetWithInstanceCount>::Uuid());

            // Test copy of a different incompatible asset type to make sure error is caught, and no data is populated.
            AZ_TEST_START_TRACE_SUPPRESSION;
            Asset<AssetWithCustomData> incompatibleAsset(someData, AZ::Data::AssetLoadBehavior::Default);
            AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            EXPECT_EQ(incompatibleAsset.Get(), nullptr);         // Verify data assignment was rejected
            EXPECT_TRUE(!incompatibleAsset.GetId().IsValid());         // Verify asset Id was not assigned
            EXPECT_EQ(AzTypeInfo<AssetWithCustomData>::Uuid(), incompatibleAsset.GetType());         // Verify asset ptr type is still the original template type.
        }

        // Allow the asset manager to purge assets on the dead list.
        AssetManager::Instance().DispatchEvents();

        EXPECT_EQ(EmptyAssetWithInstanceCount::s_instanceCount, 1);
        EXPECT_EQ(someData->GetUseCount(), 1);

        AssetManager::Instance().DispatchEvents();
    }

    TEST_F(AssetManagerTest, AssignAssetData_NoExistingAsset_DoesNotCrash)
    {
        Asset<AssetWithCustomData> someAsset(new AssetWithCustomData(Uuid::CreateRandom(), AZ::Data::AssetData::AssetStatus::Ready),
            AZ::Data::AssetLoadBehavior::Default); // the data is now "owned" by this, it's not a copy
        AssetManager::Instance().AssignAssetData(someAsset);
    }

    TEST_F(AssetManagerTest, AssetManager_UnregisterHandler_OnlyErrorsForAssetsCreatedByAssetManager)
    {
        // Unregister fixture handler(MyAssetHandlerAndCatalog) until the end of the test
        AssetManager::Instance().UnregisterHandler(m_assetHandlerAndCatalog);

        MyAssetActiveAssetCountHandler testHandler;
        AssetManager::Instance().RegisterHandler(&testHandler, AzTypeInfo<AssetWithCustomData>::Uuid());
        {
            // Unmanaged asset created in Scope #1
            Asset<AssetWithCustomData> nonAssetManagerManagedAsset(aznew AssetWithCustomData(), AZ::Data::AssetLoadBehavior::Default);
            {
                // Managed asset created in Scope #2
                Asset<AssetWithCustomData> assetManagerManagedAsset1;
                assetManagerManagedAsset1.Create(MyAsset1Id);
                Asset<AssetWithCustomData> assetManagerManagedAsset2 = AssetManager::Instance().CreateAsset(MyAsset2Id, azrtti_typeid<AssetWithCustomData>(), AZ::Data::AssetLoadBehavior::Default);

                // There are still two assets handled by the AssetManager so it should error once
                // An assert will occur if the AssetHandler is unregistered and there are still active assets
                // We expect TWO assertions here since there will be TWO entries in the map
                // one from Create and one from GetAsset.
                AZ_TEST_START_TRACE_SUPPRESSION;
                AssetManager::Instance().UnregisterHandler(&testHandler);
                AZ_TEST_STOP_TRACE_SUPPRESSION(2);
                // Re-register AssetHandler and let the managed assets ref count hit zero which will remove them from the AssetManager
                AssetManager::Instance().RegisterHandler(&testHandler, AzTypeInfo<AssetWithCustomData>::Uuid());
            }

            // Unregistering the AssetHandler now should result in 0 error messages since the m_assetHandlerAndCatalog::m_nActiveAsset count should be 0.
            AZ_TEST_START_TRACE_SUPPRESSION;
            AssetManager::Instance().UnregisterHandler(&testHandler);
            AZ_TEST_STOP_TRACE_SUPPRESSION(0);
            //Re-register AssetHandler and let the block scope end for the non managed asset.
            AssetManager::Instance().RegisterHandler(&testHandler, AzTypeInfo<AssetWithCustomData>::Uuid());
        }

        // Unregister the TestAssetHandler one last time. The unmanaged asset has already been destroyed.
        // The m_assetHandlerAndCatalog::m_nActiveAsset count should still be 0 as the it did not manage the nonAssetManagerManagedAsset object
        AZ_TEST_START_TRACE_SUPPRESSION;
        AssetManager::Instance().UnregisterHandler(&testHandler);
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);

        // Re-register the fixture handler so that the UnitTest fixture is able to cleanup the AssetManager without errors
        AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<AssetWithCustomData>::Uuid());
        AssetManager::Instance().RegisterHandler(m_assetHandlerAndCatalog, AzTypeInfo<EmptyAssetWithInstanceCount>::Uuid());
    }

    TEST_F(AssetManagerTest, AssetManager_SuspendResumeAssetRelease)
    {
        auto asset = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AssetLoadBehavior::Default);
        asset.BlockUntilLoadComplete();
        AssetManager::Instance().DispatchEvents();

        AssetManager::Instance().SuspendAssetRelease();

        asset = {};

        AssetManager::Instance().DispatchEvents();

        auto&& assets = m_testAssetManager->GetAssets();

        EXPECT_EQ(assets.size(), 1);
        EXPECT_NE(assets.find(MyAsset1Id), assets.end());

        AssetManager::Instance().ResumeAssetRelease();
        
        // Sleep to allow for the assets to release
        int retryCount = 100;
        while ((--retryCount>0) && assets.size() > 0)
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(10));
        }

        EXPECT_EQ(assets.size(), 0);
    }

    TEST_F(AssetManagerTest, AssetManager_SuspendResumeAssetRelease_ReusedAssetIsNotReleased)
    {
        auto asset = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AssetLoadBehavior::Default);
        asset.BlockUntilLoadComplete();

        AssetManager::Instance().SuspendAssetRelease();

        asset = {};

        AssetManager::Instance().DispatchEvents();

        asset = AssetManager::Instance().GetAsset<AssetWithCustomData>(MyAsset1Id, AssetLoadBehavior::Default);

        auto&& assets = m_testAssetManager->GetAssets();

        AssetManager::Instance().ResumeAssetRelease();

        EXPECT_EQ(assets.size(), 1);
        EXPECT_NE(assets.find(MyAsset1Id), assets.end());
    }
}
