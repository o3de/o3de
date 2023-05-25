/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

#include <Atom/RPI.Edit/Common/ConvertibleSource.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

#include <Common/AnyAssetBuilder.h>

#include <Tests.Builders/BuilderTestFixture.h>

namespace UnitTest
{
    using namespace AZ;
    
    // Basic test class which is also used for output class of convertible class test
    class Test1
    {
    public:
        AZ_TYPE_INFO(Test1, "{A3369968-6E98-4319-A4CA-A0E2CF9F2E7C}");
        AZ_CLASS_ALLOCATOR(Test1, AZ::SystemAllocator);

        static void Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Test1>()
                    ->Version(1)
                    ->Field("m_data", &Test1::m_data)
                    ->Field("m_isConverted", &Test1::m_isConverted)
                    ;
            }
        }

        AZStd::string m_data = "Test1";
        bool m_isConverted = false;
    };

    // Test class with convertible source
    class Test2Source :
        public RPI::ConvertibleSource
    {
    public:
        AZ_TYPE_INFO(Test2Source, "{D472B405-F688-4EAF-A361-D8D1C63E303D}");
        AZ_CLASS_ALLOCATOR(Test2Source, AZ::SystemAllocator);

        static void Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Test2Source, ConvertibleSource>()
                    ->Version(1)
                    ;
            }
        }

        // ConvertibleSource overrides...
        bool Convert(TypeId& outTypeId, AZStd::shared_ptr<void>& outData) const override
        {
            // convert this to Test1
            outTypeId = AzTypeInfo<Test1>::Uuid();
            auto data = aznew Test1();
            data->m_isConverted = true;
            outData = AZStd::shared_ptr<void>(data);
            return true;
        }
    };

    class TestAssetData
        : public Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(TestAssetData, AZ::SystemAllocator);
        AZ_RTTI(TestAssetData, "{A7D2C40A-2559-4DF7-A308-D52286EE16D8}", Data::AssetData);
    };

    // Test class type with AssetId and Asset reference 
    class TestAssetIdReference
    {
    public:
        AZ_TYPE_INFO(TestAssetIdReference, "{87DC6B1E-4660-4AEA-AEE1-6F50EF7FA0D7}");
        AZ_CLASS_ALLOCATOR(TestAssetIdReference, AZ::SystemAllocator);

        virtual ~TestAssetIdReference() = default;

        static void Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestAssetIdReference>()
                    ->Version(1)
                    // Not supported by Json Serializer yet. LY-105721
                    //->Field("m_asset", &TestAssetIdReference::m_asset) 
                    ->Field("EmptyAssetId", &TestAssetIdReference::m_emptyAssetId)
                    ->Field("ValidAssetId", &TestAssetIdReference::m_validAssetId)
                    ->Field("DuplicateAssetId", &TestAssetIdReference::m_duplicateAssetId)
                    ->Field("AssetIdInContainer", &TestAssetIdReference::m_assetIdInContainer)
                    ;
            }
        }

        TestAssetIdReference()
            : m_asset(Data::AssetLoadBehavior::NoLoad)
        {
        }

        virtual void Init()
        {
            m_validAssetId = Data::AssetId(Uuid::CreateRandom(), 1);
            m_duplicateAssetId = m_validAssetId;
            m_asset = Data::Asset<Data::AssetData>(Data::AssetId(Uuid::CreateRandom(), 0), TestAssetData::TYPEINFO_Uuid());
            m_assetIdInContainer.push_back(Data::AssetId(Uuid::CreateRandom(), 0));
        }

        Data::Asset<TestAssetData> m_asset;
        Data::AssetId m_emptyAssetId;
        Data::AssetId m_validAssetId;
        Data::AssetId m_duplicateAssetId;
        AZStd::vector<Data::AssetId> m_assetIdInContainer;
        
        // The total amount of unique asset ids referenced by this class object
        // one from m_asset, one from m_validAssetId
        static const int s_uniqueAssetIdCount = 2;
    };

    class DerivedTestAssetIdReference
        : public TestAssetIdReference
    {
    public:
        AZ_TYPE_INFO(DerivedTestAssetIdReference, "{B5778901-A553-41B2-B411-CF8FBE2B1E10}");
        AZ_CLASS_ALLOCATOR(DerivedTestAssetIdReference, AZ::SystemAllocator);

        static void Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<DerivedTestAssetIdReference, TestAssetIdReference>()
                    ->Version(1)
                    ;
            }
        }
    };

    // Test class includes member which is a class that has asset id and asset reference as members
    class TestIndirectAssetIdReference
    {
    public:
        AZ_TYPE_INFO(TestIndirectAssetIdReference, "{402D2672-55CD-46B9-9387-E34D6B10F88A}");
        AZ_CLASS_ALLOCATOR(TestIndirectAssetIdReference, AZ::SystemAllocator);

        static void Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TestIndirectAssetIdReference>()
                    ->Version(1)
                    ->Field("m_object", &TestIndirectAssetIdReference::m_object)
                    ->Field("m_objectPtr", &TestIndirectAssetIdReference::m_objectPtr)
                    ->Field("m_objects", &TestIndirectAssetIdReference::m_objects)
                    ->Field("m_objectWithBase", &TestIndirectAssetIdReference::m_objectWithBase)
                    ;
            }
        }

        void Init()
        {
            m_object.Init();
            m_objectPtr = AZStd::make_unique<TestAssetIdReference>();
            m_objectPtr->Init();
            m_objects["Test4"].Init();
            m_objectWithBase.Init();
        }
        
        TestAssetIdReference m_object;
        AZStd::unique_ptr<TestAssetIdReference> m_objectPtr = nullptr;
        AZStd::unordered_map<AZStd::string, TestAssetIdReference> m_objects;
        DerivedTestAssetIdReference m_objectWithBase;

        static const int s_uniqueAssetIdCount = 4 * TestAssetIdReference::s_uniqueAssetIdCount;
    };

    class AnyAssetBuilderTests
        : public BuilderTestFixture
    {
    protected:

        RPI::AnyAssetHandler* m_assetHandler;

        void SetUp() override
        {
            BuilderTestFixture::SetUp();
            
            Test1::Reflect(m_context.get());
            Test2Source::Reflect(m_context.get());
            TestAssetIdReference::Reflect(m_context.get());
            DerivedTestAssetIdReference::Reflect(m_context.get());
            TestIndirectAssetIdReference::Reflect(m_context.get());
            m_assetHandler = new RPI::AnyAssetHandler();
            m_assetHandler->m_serializeContext = m_context.get();
            m_assetHandler->Register();
        }

        void TearDown() override
        {
            m_assetHandler->Unregister();
            delete m_assetHandler;
            BuilderTestFixture::TearDown();
        }

        //////////////////////////////////////////////////////////////////////////

        // Helper function to generate source AnyAsset and save it to specified folder
        template<typename T>
        void SaveClassToAnyAssetSourceFile(T& data, const AZStd::string& saveFileName)
        {
            JsonSerializationUtils::SaveObjectToFile<T>(&data, saveFileName);
        }

        Data::Asset<Data::AssetData> LoadAssetFromFile(const char* assetFile)
        {
            Data::Asset<Data::AssetData> outAsset(m_assetHandler->CreateAsset(Data::AssetId(Uuid::CreateRandom(), 1), Data::AssetType()),
                Data::AssetLoadBehavior::PreLoad);
            AZ::u64 fileLength = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(assetFile, fileLength);
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream = AZStd::make_shared<AZ::Data::AssetDataStream>();
            stream->Open(assetFile, 0, fileLength);
            stream->BlockUntilLoadComplete();
            m_assetHandler->LoadAssetData(outAsset, stream, {});
            stream->Close();

            // Force a file streamer flush to ensure that file handles don't remain used
            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            AZStd::binary_semaphore wait;
            AZ::IO::FileRequestPtr flushRequest = streamer->FlushCaches();
            streamer->SetRequestCompleteCallback(flushRequest, [&wait]([[maybe_unused]] AZ::IO::FileRequestHandle request)
                {
                    wait.release();
                });
            streamer->QueueRequest(flushRequest);
            wait.acquire();

            return outAsset;
        }
        
    };
    
    TEST_F(AnyAssetBuilderTests, ProcessJobBasic)
    {
        const char* testAssetName = "AnyAssetTest.source";
        AZ::Test::ScopedAutoTempDirectory productDir;
        AZ::Test::ScopedAutoTempDirectory sourceDir;
        AZ::IO::Path sourceFilePath = sourceDir.Resolve(testAssetName);

        // Basic test: test data before and after are same. Test data class doesn't have converter or asset reference.
        RPI::AnyAssetBuilder builder;
        AssetBuilderSDK::ProcessJobRequest request;
        AssetBuilderSDK::ProcessJobResponse response;

        // Initial job request
        request.m_fullPath = sourceFilePath.Native();
        request.m_tempDirPath = productDir.GetDirectory();

        Test1 test1;
        test1.m_data = "first";
        SaveClassToAnyAssetSourceFile<Test1>(test1, sourceFilePath.Native());

        // Process
        builder.ProcessJob(request, response);

        // verify job output
        EXPECT_TRUE(response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success);
        EXPECT_TRUE(response.m_outputProducts.size() == 1);
        EXPECT_TRUE(response.m_outputProducts[0].m_dependencies.size() == 0);

        // verify input and output data are same
        auto outAsset = LoadAssetFromFile(response.m_outputProducts[0].m_productFileName.c_str());
        auto outTest1 = static_cast<RPI::AnyAsset*>(outAsset.GetData())->GetDataAs<Test1>();
        EXPECT_TRUE(test1.m_data == outTest1->m_data);
    }

    TEST_F(AnyAssetBuilderTests, ProcessJobConvert)
    {
        const char* testAssetName = "AnyAssetTest.source";
        AZ::Test::ScopedAutoTempDirectory productDir;
        AZ::Test::ScopedAutoTempDirectory sourceDir;
        AZ::IO::Path sourceFilePath = sourceDir.Resolve(testAssetName);

        RPI::AnyAssetBuilder builder;
        AssetBuilderSDK::ProcessJobRequest request;
        AssetBuilderSDK::ProcessJobResponse response;

        // Initial job request 
        request.m_fullPath = sourceFilePath.Native();
        request.m_tempDirPath = productDir.GetDirectory();
        
        // Test data class which has a converter
        Test2Source test2;
        SaveClassToAnyAssetSourceFile<Test2Source>(test2, sourceFilePath.Native());

        builder.ProcessJob(request, response);
        EXPECT_TRUE(response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success);
        
        auto outAsset = LoadAssetFromFile(response.m_outputProducts[0].m_productFileName.c_str());
        auto outTest2 = static_cast<RPI::AnyAsset*>(outAsset.GetData())->GetDataAs<Test1>();
        EXPECT_TRUE(outTest2 != nullptr);
        EXPECT_TRUE(outTest2->m_isConverted);
    }

    TEST_F(AnyAssetBuilderTests, ProcessJobDependencyDirect)
    {
        const char* testAssetName = "AnyAssetTest.source";
        AZ::Test::ScopedAutoTempDirectory productDir;
        AZ::Test::ScopedAutoTempDirectory sourceDir;
        AZ::IO::Path sourceFilePath = sourceDir.Resolve(testAssetName);

        RPI::AnyAssetBuilder builder;
        AssetBuilderSDK::ProcessJobRequest request;
            AssetBuilderSDK::ProcessJobResponse response;

        // Initial job request once
        request.m_fullPath = sourceFilePath.Native();
        request.m_tempDirPath = productDir.GetDirectory();

        // Test class which has asset id and asset reference as member variables
        TestAssetIdReference objectHasAssetIds;
        objectHasAssetIds.Init();
        SaveClassToAnyAssetSourceFile<TestAssetIdReference>(objectHasAssetIds, sourceFilePath.Native());
        
        builder.ProcessJob(request, response);
        EXPECT_TRUE(response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success);

        auto outAsset = LoadAssetFromFile(response.m_outputProducts[0].m_productFileName.c_str());
        auto outTest3 = static_cast<RPI::AnyAsset*>(outAsset.GetData())->GetDataAs<TestAssetIdReference>();
        EXPECT_TRUE(outTest3 != nullptr);
        EXPECT_TRUE(response.m_outputProducts[0].m_dependencies.size() == objectHasAssetIds.s_uniqueAssetIdCount);
    }    
        
    TEST_F(AnyAssetBuilderTests, ProcessJobDependencyIndirect)
    {
        const char* testAssetName = "AnyAssetTest.source";
        AZ::Test::ScopedAutoTempDirectory productDir;
        AZ::Test::ScopedAutoTempDirectory sourceDir;
        AZ::IO::Path sourceFilePath = sourceDir.Resolve(testAssetName);

        RPI::AnyAssetBuilder builder;
        AssetBuilderSDK::ProcessJobRequest request;
        AssetBuilderSDK::ProcessJobResponse response;

        // Initial job request once
        request.m_fullPath = sourceFilePath.Native();
        request.m_tempDirPath = productDir.GetDirectory();
                
        // Test class includes member which its class has asset id and asset reference as children
        TestIndirectAssetIdReference test4;
        test4.Init();
        SaveClassToAnyAssetSourceFile<TestIndirectAssetIdReference>(test4, sourceFilePath.Native());

        builder.ProcessJob(request, response);
        EXPECT_TRUE(response.m_resultCode == AssetBuilderSDK::ProcessJobResult_Success);

        auto outAsset = LoadAssetFromFile(response.m_outputProducts[0].m_productFileName.c_str());
        auto outTest4 = static_cast<RPI::AnyAsset*>(outAsset.GetData())->GetDataAs<TestIndirectAssetIdReference>();
        EXPECT_TRUE(outTest4 != nullptr);
        EXPECT_TRUE(response.m_outputProducts[0].m_dependencies.size() == test4.s_uniqueAssetIdCount);
    }
} // namespace UnitTests
