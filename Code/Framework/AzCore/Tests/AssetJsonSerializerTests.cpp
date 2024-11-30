/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetJsonSerializer.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Tests/Asset/MockLoadAssetCatalogAndHandler.h>
#include <Tests/Serialization/Json/JsonSerializerConformityTests.h>

namespace JsonSerializationTests
{
    class TestAssetData final
        : public AZ::Data::AssetData
    {
    public:
        AZ_CLASS_ALLOCATOR(TestAssetData, AZ::SystemAllocator);
        AZ_RTTI(TestAssetData, "{90BCCF83-D453-4A70-973D-57C2ACD04661}", AZ::Data::AssetData);

        TestAssetData() = default;
        TestAssetData(const AZ::Data::AssetId& assetId, AZ::Data::AssetData::AssetStatus status)
            : AZ::Data::AssetData(assetId, status)
        {}
        ~TestAssetData() override = default;
    };

    class TestSerializedAssetTracker
        : public BaseJsonSerializerFixture
    {
    public:
        void SetUp() override
        {
            BaseJsonSerializerFixture::SetUp();

            // Set up the Job Manager with 1 thread so that the Asset Manager is able to load assets.
            AZ::JobManagerDesc jobDesc;
            AZ::JobManagerThreadDesc threadDesc;
            jobDesc.m_workerThreads.push_back(threadDesc);
            m_jobManager = aznew AZ::JobManager(jobDesc);
            m_jobContext = aznew AZ::JobContext(*m_jobManager);
            AZ::JobContext::SetGlobalContext(m_jobContext);

            AZ::Data::AssetManager::Descriptor descriptor;
            AZ::Data::AssetManager::Create(descriptor);
            auto CreateTestAssetData = []() -> AZ::Data::AssetPtr
            {
                return aznew TestAssetData();
            };
            auto DestroyTestAssetData = [](AZ::Data::AssetPtr assetPtr)
            {
                delete assetPtr;
            };
            m_catalogAndHandler = AZStd::make_unique<UnitTest::MockLoadAssetCatalogAndHandler>(
                AZStd::unordered_set<AZ::Data::AssetId>{}, azrtti_typeid<TestAssetData>(),
                AZStd::move(CreateTestAssetData), AZStd::move(DestroyTestAssetData));

            m_serializeContext->RegisterGenericType<AZ::Data::Asset<TestAssetData>>();
            m_jsonRegistrationContext->Serializer<AZ::Data::AssetJsonSerializer>()->HandlesType<AZ::Data::Asset>();
        }

        void TearDown() override
        {
            m_jsonRegistrationContext->EnableRemoveReflection();
            m_jsonRegistrationContext->Serializer<AZ::Data::AssetJsonSerializer>()->HandlesType<AZ::Data::Asset>();
            m_jsonRegistrationContext->DisableRemoveReflection();

            m_catalogAndHandler.reset();
            AZ::Data::AssetManager::Destroy();

            AZ::JobContext::SetGlobalContext(nullptr);
            delete m_jobContext;
            delete m_jobManager;

            BaseJsonSerializerFixture::TearDown();
        }

    private:
        AZStd::unique_ptr<UnitTest::MockLoadAssetCatalogAndHandler> m_catalogAndHandler;
        AZ::JobManager* m_jobManager{ nullptr };
        AZ::JobContext* m_jobContext{ nullptr };
    };

    TEST_F(TestSerializedAssetTracker, AssetTracker_Callback_Works)
    {
        auto assetCallback = [](AZ::Data::Asset<AZ::Data::AssetData>& asset)
        {
            if (!asset.GetId().IsValid() && !asset.GetHint().empty())
            {
                if (asset.GetHint() == "test/path/foo.asset")
                {
                    asset.SetHint("passed");
                }
            }
        };
        auto tracker = AZ::Data::SerializedAssetTracker{};
        tracker.SetAssetFixUp(assetCallback);

        AZ::JsonDeserializerSettings settings;
        settings.m_metadata.Add(tracker);
        settings.m_registrationContext = this->m_jsonRegistrationContext.get();
        settings.m_serializeContext = this->m_serializeContext.get();

        AZStd::string_view assetHintOnlyTestAsset = R"(
                {
                    "assetHint" : "test/path/foo.asset"
                })";
        rapidjson::Document jsonDom;
        jsonDom.Parse(assetHintOnlyTestAsset.data());

        AZ::Data::Asset<TestAssetData> instance;
        auto result = AZ::JsonSerialization::Load(instance, jsonDom, settings);
        EXPECT_NE(result.GetProcessing(), AZ::JsonSerializationResult::Processing::Halted);
        EXPECT_STREQ(instance.GetHint().c_str(), "passed");
    }

    class AssetSerializerTestDescription final
        : public JsonSerializerConformityTestDescriptor<AZ::Data::Asset<TestAssetData>>
    {
    public:
        using Asset = AZ::Data::Asset<TestAssetData>;

        void SetUp() override
        {
            // Set up the Job Manager with 1 thread so that the Asset Manager is able to load assets.
            AZ::JobManagerDesc jobDesc;
            AZ::JobManagerThreadDesc threadDesc;
            jobDesc.m_workerThreads.push_back(threadDesc);
            m_jobManager = aznew AZ::JobManager(jobDesc);
            m_jobContext = aznew AZ::JobContext(*m_jobManager);
            AZ::JobContext::SetGlobalContext(m_jobContext);

            AZ::Data::AssetManager::Descriptor descriptor;
            AZ::Data::AssetManager::Create(descriptor);

            // Add the Asset Id, used for the Asset Conformity test to the Mock Catalog
            const AZ::Data::AssetId id{ "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}", 1 };

            auto CreateTestAssetData = []() -> AZ::Data::AssetPtr
            {
                return aznew TestAssetData();
            };
            auto DestroyTestAssetData = [](AZ::Data::AssetPtr assetPtr)
            {
                delete assetPtr;
            };
            m_catalogAndHandler = AZStd::make_unique<UnitTest::MockLoadAssetCatalogAndHandler>(
                AZStd::unordered_set<AZ::Data::AssetId>{ id }, azrtti_typeid<TestAssetData>(),
                AZStd::move(CreateTestAssetData), AZStd::move(DestroyTestAssetData));
        }

        void TearDown() override
        {
            m_catalogAndHandler.reset();
            AZ::Data::AssetManager::Destroy();

            AZ::JobContext::SetGlobalContext(nullptr);
            delete m_jobContext;
            delete m_jobManager;
        }

        using JsonSerializerConformityTestDescriptor<AZ::Data::Asset<TestAssetData>>::Reflect;
        void Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context) override
        {
            context->RegisterGenericType<Asset>();
        }

        AZStd::shared_ptr<AZ::BaseJsonSerializer> CreateSerializer() override
        {
            return AZStd::make_shared<AZ::Data::AssetJsonSerializer>();
        }

        AZStd::shared_ptr<Asset> CreateDefaultInstance() override
        {
            return AZStd::make_shared<Asset>();
        }

        AZStd::shared_ptr<Asset> CreatePartialDefaultInstance() override
        {
            AZ::Data::AssetId id{ "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}", 0};

            auto instance = AZStd::make_shared<Asset>();
            instance->Create(id, false);
            return instance;
        }

        AZStd::shared_ptr<Asset> CreateFullySetInstance() override
        {
            AZ::Data::AssetId id{ "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}", 1 };

            auto instance = AZStd::make_shared<Asset>();
            instance->Create(id, false);
            instance->SetHint("TestFile");
            instance->SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::PreLoad);
            return instance;
        }

        AZStd::string_view GetJsonForPartialDefaultInstance() override
        {
            return R"(
                {
                    "assetId" :
                    {
                        "guid": "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}"
                    }
                })";
        }

        AZStd::string_view GetJsonForFullySetInstance() override
        {
            return R"(
                {
                    "assetId" :
                    {
                        "guid": "{BBEAC89F-8BAD-4A9D-BF6E-D0DF84A8DFD6}",
                        "subId": 1
                    },
                    "loadBehavior": "PreLoad",
                    "assetHint": "TestFile"
                })";
        }

        void ConfigureFeatures(JsonSerializerConformityTestDescriptorFeatures& features) override
        {
            features.EnableJsonType(rapidjson::kObjectType);
            features.m_typeToInject = rapidjson::kNullType;
            // Assets are not fully registered with the Serialize Context for historical reasons. Due to the missing
            // information the Json Serializer Conformity Tests can't run the subsection of tests that explicitly
            // require the missing information.
            features.m_enableNewInstanceTests = false;
        }

        bool AreEqual(const Asset& lhs, const Asset& rhs) override
        {
            return lhs == rhs;
        }

        void AddSystemComponentDescriptors(BaseJsonSerializerFixture::ComponentContainer& systemComponents) override
        {
            systemComponents.push_back(AZ::AssetManagerComponent::CreateDescriptor());
            systemComponents.push_back(AZ::JobManagerComponent::CreateDescriptor());
            systemComponents.push_back(AZ::StreamerComponent::CreateDescriptor());
        }

    private:
        AZStd::unique_ptr<UnitTest::MockLoadAssetCatalogAndHandler> m_catalogAndHandler;
        AZ::JobManager* m_jobManager{ nullptr };
        AZ::JobContext* m_jobContext{ nullptr };
    };

    using AssetConformityTestTypes = ::testing::Types<AssetSerializerTestDescription>;
    IF_JSON_CONFORMITY_ENABLED(INSTANTIATE_TYPED_TEST_CASE_P(Asset, JsonSerializerConformityTests, AssetConformityTestTypes));
} // namespace JsonSerializationTests
