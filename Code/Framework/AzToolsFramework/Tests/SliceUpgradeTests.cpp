/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(,"-Wdelete-non-virtual-dtor")

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Slice/SliceMetadataInfoComponent.h>
#include <AzCore/Slice/SliceAssetHandler.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <CustomSerializeContextTestFixture.h>
#include "SliceUpgradeTestsData.h"

namespace UnitTest
{
    class SliceUpgradeTest_MockCatalog final
        : public AZ::Data::AssetCatalog
        , public AZ::Data::AssetCatalogRequestBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(SliceUpgradeTest_MockCatalog, AZ::SystemAllocator);

        SliceUpgradeTest_MockCatalog()
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusConnect();
        }

        ~SliceUpgradeTest_MockCatalog() override
        {
            AZ::Data::AssetCatalogRequestBus::Handler::BusDisconnect();
            DisableCatalog();
        }

        //////////////////////////////////////////////////////////////////////////
        // AssetCatalogRequestBus
        AZ::Data::AssetInfo GetAssetInfoById(const AZ::Data::AssetId& id) override
        {
            AZ::Data::AssetInfo result;
            auto itr = m_assetInfoMap.find(id);
            if (itr != m_assetInfoMap.end())
            {
                result = itr->second;
            }
            return result;
        }
        //////////////////////////////////////////////////////////////////////////

        const AZ::Data::AssetInfo& GenerateSliceAssetInfo(AZ::Data::AssetId assetId, const char* assetHintName = "datapatch_test.slice")
        {
            EXPECT_TRUE(assetId.IsValid());
            AZ::Data::AssetInfo& assetInfo = m_assetInfoMap[assetId];
            assetInfo.m_assetId = assetId;
            assetInfo.m_assetType = AZ::AzTypeInfo<AZ::SliceAsset>::Uuid();
            assetInfo.m_relativePath = AZStd::string::format("%s-%s", assetId.ToString<AZStd::string>().c_str(), assetHintName);
            return assetInfo;
        }

        AZ::Data::AssetStreamInfo GetStreamInfoForLoad(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) override
        {
            EXPECT_EQ(assetType, AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
            AZ::Data::AssetStreamInfo info;
            info.m_streamFlags = AZ::IO::OpenMode::ModeRead;

            auto assetInfoItr = m_assetInfoMap.find(assetId);
            if (assetInfoItr != m_assetInfoMap.end())
            {
                info.m_streamName = assetInfoItr->second.m_relativePath;
            }

            if (!info.m_streamName.empty())
            {
                info.m_dataLen = static_cast<size_t>(AZ::IO::SystemFile::Length(info.m_streamName.c_str()));
            }
            return info;
        }

        AZ::Data::AssetStreamInfo GetStreamInfoForSave(const AZ::Data::AssetId& assetId, const AZ::Data::AssetType& assetType) override
        {
            AZ::Data::AssetStreamInfo info;
            info = GetStreamInfoForLoad(assetId, assetType);
            info.m_streamFlags = AZ::IO::OpenMode::ModeWrite;
            return info;
        }

    private:
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::AssetInfo> m_assetInfoMap;
    };

    class SliceUpgradeTest : public CustomSerializeContextTestFixture
    {
    protected:
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_sliceDescriptor;
        AZStd::unique_ptr<SliceUpgradeTest_MockCatalog> m_mockCatalog;
        AZStd::unique_ptr<AZ::IO::Streamer> m_streamer;

        AZStd::unique_ptr<AZ::SliceComponent> m_rootSliceComponent;
        AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::SliceAsset>> m_sliceAssets;
        AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<char>> m_sliceStreams;

    public:
        void SetUp() override
        {
            CustomSerializeContextTestFixture::SetUp();

            m_streamer = AZStd::make_unique<AZ::IO::Streamer>(AZStd::thread_desc{}, AZ::StreamerComponent::CreateStreamerStack());
            AZ::Interface<AZ::IO::IStreamer>::Register(m_streamer.get());

            m_sliceDescriptor.reset(AZ::SliceComponent::CreateDescriptor());
            m_sliceDescriptor->Reflect(m_serializeContext.get());
            AZ::SliceMetadataInfoComponent::Reflect(m_serializeContext.get());
            AzFramework::SimpleAssetReferenceBase::Reflect(m_serializeContext.get());
            AZ::Entity::Reflect(m_serializeContext.get());
            AZ::DataPatch::Reflect(m_serializeContext.get());

            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);
            AZ::Data::AssetManager::Instance().RegisterHandler(aznew AZ::SliceAssetHandler(m_serializeContext.get()), AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());
            m_mockCatalog.reset(aznew SliceUpgradeTest_MockCatalog());
            AZ::Data::AssetManager::Instance().RegisterCatalog(m_mockCatalog.get(), AZ::AzTypeInfo<AZ::SliceAsset>::Uuid());

            m_rootSliceComponent.reset(aznew AZ::SliceComponent);
            m_rootSliceComponent->Instantiate();
        }

        void TearDown() override
        {
            m_rootSliceComponent.reset();
            {
                AZStd::unordered_map<AZ::Data::AssetId, AZ::Data::Asset<AZ::SliceAsset>> clean_sliceAssets;
                m_sliceAssets.swap(clean_sliceAssets);
                AZStd::unordered_map<AZ::Data::AssetId, AZStd::vector<char>> clean_sliceStreams;
                m_sliceStreams.swap(clean_sliceStreams);
            }

            m_mockCatalog.reset();
            AZ::Data::AssetManager::Destroy();

            m_sliceDescriptor.reset();
            m_serializeContext.reset();

            AZ::Interface<AZ::IO::IStreamer>::Unregister(m_streamer.get());
            m_streamer.reset();
            CustomSerializeContextTestFixture::TearDown();
        }

        void SaveSliceAssetToStream(AZ::Data::AssetId sliceAssetId)
        {
            auto sliceAssetItr = m_sliceAssets.find(sliceAssetId);
            ASSERT_NE(sliceAssetItr, m_sliceAssets.end());

            AZ::Entity* sliceAssetEntity = sliceAssetItr->second.GetAs<AZ::SliceAsset>()->GetEntity();

            AZStd::vector<char>& buf = m_sliceStreams[sliceAssetId];
            buf.clear();
            AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&buf);
            AZ::ObjectStream* objStream = AZ::ObjectStream::Create(&stream, *m_serializeContext, AZ::ObjectStream::ST_XML);
            objStream->WriteClass(sliceAssetEntity);
            EXPECT_TRUE(objStream->Finalize());
        }

        void SaveRawSliceAssetXML(AZ::Data::AssetId sliceAssetId, const char* sliceStr, size_t sliceStrSize)
        {
            // Create empty slice asset placeholder which will be filled.
            const AZ::Data::AssetInfo& assetInfo = m_mockCatalog->GenerateSliceAssetInfo(sliceAssetId);
            AZ::Data::Asset<AZ::SliceAsset> sliceAssetHolder =
                AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(assetInfo.m_assetId, AZ::Data::AssetLoadBehavior::Default);
            m_sliceAssets.emplace(assetInfo.m_assetId, sliceAssetHolder);

            AZStd::vector<char>& buf = m_sliceStreams[sliceAssetId];
            buf.resize_no_construct(sliceStrSize);
            memcpy(buf.data(), sliceStr, sliceStrSize);
        }

        /**
         * We "simulate" slice operations by doing everything without involving disk operations.
         *
         * If the argument `entity` is NOT from an existing slice instance, SaveAsSlice transfers
         * the ownership of `entity` to the newly created slice asset, so don't use or delete it
         * after calling SaveAsSlice.
         */
        AZ::Data::AssetId SaveAsSlice(AZ::Entity* entity, const AZ::Uuid& newAssetUuid = AZ::Uuid::CreateRandom(), const char* assetHintName = "datapatch_test.slice")
        {
            AZ::Entity* sliceEntity = aznew AZ::Entity();
            AZ::SliceComponent* sliceComponent = nullptr;

            AZ::SliceComponent::SliceInstanceAddress sliceInstAddress = m_rootSliceComponent->FindSlice(entity);
            if (sliceInstAddress.IsValid())
            {
                AZ::SliceComponent* tempSliceComponent = aznew AZ::SliceComponent();
                // borrow the slice instance for making nested slice
                sliceInstAddress = tempSliceComponent->AddSliceInstance(sliceInstAddress.GetReference(), sliceInstAddress.GetInstance());
                AZ::SliceComponent::SliceInstanceToSliceInstanceMap sourceToCloneSliceInstanceMap;
                sliceComponent = tempSliceComponent->Clone(*m_serializeContext, &sourceToCloneSliceInstanceMap);
                // return the slice instance back
                m_rootSliceComponent->AddSliceInstance(sliceInstAddress.GetReference(), sliceInstAddress.GetInstance());
                delete tempSliceComponent;
            }
            else
            {
                sliceComponent = aznew AZ::SliceComponent();
                sliceComponent->SetSerializeContext(m_serializeContext.get());
                sliceComponent->AddEntity(entity);
            }
            sliceComponent->SetSerializeContext(m_serializeContext.get());
            sliceEntity->AddComponent(sliceComponent);
            sliceEntity->Init();
            sliceEntity->Activate();

            const AZ::Data::AssetInfo& assetInfo = m_mockCatalog->GenerateSliceAssetInfo(AZ::Data::AssetId(newAssetUuid, 1), assetHintName);
            AZ::Data::Asset<AZ::SliceAsset> sliceAssetHolder =
                AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(assetInfo.m_assetId, AZ::Data::AssetLoadBehavior::Default);
            sliceAssetHolder.GetAs<AZ::SliceAsset>()->SetData(sliceEntity, sliceComponent);

            // Hold on to sliceAssetHolder so it's not ref-counted away.
            m_sliceAssets.emplace(assetInfo.m_assetId, sliceAssetHolder);

            // Serialize the slice to a stream, so later we can de-serialize it back with different data versions.
            SaveSliceAssetToStream(assetInfo.m_assetId);

            return assetInfo.m_assetId;
        }

        AZ::Entity* InstantiateSlice(AZ::Data::AssetId sliceAssetId)
        {
            auto sliceAssetItr = m_sliceAssets.find(sliceAssetId);
            EXPECT_NE(sliceAssetItr, m_sliceAssets.end());
            AZ::SliceComponent::SliceInstanceAddress sliceInstAddress = m_rootSliceComponent->AddSlice(sliceAssetItr->second);

            m_rootSliceComponent->Instantiate();

            const AZ::SliceComponent::InstantiatedContainer* entityContainer = sliceInstAddress.GetInstance()->GetInstantiated();
            // For convenience reason, only single entity slices are allowed for now.
            EXPECT_EQ(entityContainer->m_entities.size(), 1);
            return entityContainer->m_entities[0];
        }

        void ReloadSliceAssetFromStream(AZ::Data::AssetId sliceAssetId)
        {
            auto sliceAssetItr = m_sliceAssets.find(sliceAssetId);
            ASSERT_NE(sliceAssetItr, m_sliceAssets.end());
            auto sliceStreamItr = m_sliceStreams.find(sliceAssetId);
            ASSERT_NE(sliceStreamItr, m_sliceStreams.end());

            AZ::IO::ByteContainerStream<AZStd::vector<char>> stream(&sliceStreamItr->second);
            stream.Seek(0, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            AZ::Entity* newSliceAssetEntity = AZ::Utils::LoadObjectFromStream<AZ::Entity>(stream, m_serializeContext.get());
            ASSERT_NE(newSliceAssetEntity, nullptr);
            AZ::SliceComponent* newSliceAssetComponent = newSliceAssetEntity->FindComponent<AZ::SliceComponent>();
            ASSERT_NE(newSliceAssetComponent, nullptr);
            newSliceAssetComponent->SetSerializeContext(m_serializeContext.get());

            sliceAssetItr->second.GetAs<AZ::SliceAsset>()->SetData(newSliceAssetEntity, newSliceAssetComponent);
        }
    };

    TEST_F(SliceUpgradeTest, IntermmediateDataTypeChange)
    {
        TestDataA::Reflect(m_serializeContext.get());
        AzToolsFramework::Components::EditorComponentBase::Reflect(m_serializeContext.get());
        TestComponentA_V0::Reflect(m_serializeContext.get());
        AZ::Entity* entityA = aznew AZ::Entity();
        TestComponentA_V0* component = entityA->CreateComponent<TestComponentA_V0>();
        component->m_data.m_val = TestDataA_ExpectedVal;
        AZ::Data::AssetId sliceAssetId = SaveAsSlice(entityA);
        entityA = nullptr;

        AZ::Entity* instantiatedSliceEntity0 = InstantiateSlice(sliceAssetId);
        TestComponentA_V0* testComponentA = instantiatedSliceEntity0->FindComponent<TestComponentA_V0>();
        AZ_TEST_ASSERT(testComponentA != nullptr);
        AZ_TEST_ASSERT(testComponentA->m_data.m_val == TestDataA_ExpectedVal);

        const float TestDataA_OverrideVal = 2.5f;

        // Create a nested slice with overridding value.
        testComponentA->m_data.m_val = TestDataA_OverrideVal;
        AZ::Data::AssetId nestedSliceAssetId = SaveAsSlice(instantiatedSliceEntity0);
        m_rootSliceComponent->RemoveEntity(instantiatedSliceEntity0, true, true);
        instantiatedSliceEntity0 = nullptr;

        AZ::Entity* instantiatedNestedSliceEntity0 = InstantiateSlice(nestedSliceAssetId);
        testComponentA = instantiatedNestedSliceEntity0->FindComponent<TestComponentA_V0>();
        AZ_TEST_ASSERT(testComponentA->m_data.m_val == TestDataA_OverrideVal);

        m_rootSliceComponent->RemoveEntity(instantiatedNestedSliceEntity0, true, true);
        instantiatedNestedSliceEntity0 = nullptr;

        // Replace TestComponentA_V0 in Serialization context with TestComponentA_V1.
        m_serializeContext->EnableRemoveReflection();
        TestComponentA_V0::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        NewTestDataA::Reflect(m_serializeContext.get());
        TestComponentA_V1::Reflect(m_serializeContext.get());

        ReloadSliceAssetFromStream(nestedSliceAssetId);

        instantiatedNestedSliceEntity0 = InstantiateSlice(nestedSliceAssetId);
        TestComponentA_V1* testComponentA_V1 = instantiatedNestedSliceEntity0->FindComponent<TestComponentA_V1>();
        AZ_TEST_ASSERT(testComponentA_V1 != nullptr);
        EXPECT_EQ(testComponentA_V1->m_data.m_val, TestDataA_OverrideVal);
    }

    TEST_F(SliceUpgradeTest, TypeChangeInUnorderedMap)
    {
        TestDataB_V0::Reflect(m_serializeContext.get());
        AzToolsFramework::Components::EditorComponentBase::Reflect(m_serializeContext.get());
        TestComponentB_V0::Reflect(m_serializeContext.get());
        AZ::Entity* entityA = aznew AZ::Entity();
        TestComponentB_V0* componentB = entityA->CreateComponent<TestComponentB_V0>();
        componentB->m_unorderedMap.emplace(17, TestDataB_V0(17));
        componentB->m_unorderedMap.emplace(29, TestDataB_V0(29));
        componentB->m_unorderedMap.emplace(37, TestDataB_V0(37));
        AZ::Data::AssetId sliceAssetId = SaveAsSlice(entityA);
        entityA = nullptr;
        componentB = nullptr;

        AZ::Entity* instantiatedSliceEntity0 = InstantiateSlice(sliceAssetId);
        componentB = instantiatedSliceEntity0->FindComponent<TestComponentB_V0>();
        ASSERT_NE(componentB, nullptr);

        EXPECT_EQ(componentB->m_unorderedMap.size(), 3);

        auto foundItr = componentB->m_unorderedMap.find(17);
        EXPECT_NE(foundItr, componentB->m_unorderedMap.end());
        EXPECT_EQ(foundItr->second.m_data, 17);

        foundItr = componentB->m_unorderedMap.find(29);
        EXPECT_NE(foundItr, componentB->m_unorderedMap.end());
        EXPECT_EQ(foundItr->second.m_data, 29);

        foundItr = componentB->m_unorderedMap.find(37);
        EXPECT_NE(foundItr, componentB->m_unorderedMap.end());
        EXPECT_EQ(foundItr->second.m_data, 37);

        // Creat a nested slice with overridding value.
        componentB->m_unorderedMap[29].m_data = 92;
        AZ::Data::AssetId nestedSliceAssetId = SaveAsSlice(instantiatedSliceEntity0);
        m_rootSliceComponent->RemoveEntity(instantiatedSliceEntity0, true, true);
        instantiatedSliceEntity0 = nullptr;

        AZ::Entity* instantiatedNestedSliceEntity0 = InstantiateSlice(nestedSliceAssetId);
        componentB = instantiatedNestedSliceEntity0->FindComponent<TestComponentB_V0>();
        ASSERT_NE(componentB, nullptr);

        EXPECT_EQ(componentB->m_unorderedMap.size(), 3);
        foundItr = componentB->m_unorderedMap.find(29);
        EXPECT_NE(foundItr, componentB->m_unorderedMap.end());
        EXPECT_EQ(foundItr->second.m_data, 92);

        m_serializeContext->EnableRemoveReflection();
        TestComponentB_V0::Reflect(m_serializeContext.get());
        TestDataB_V0::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        TestDataB_V1::Reflect(m_serializeContext.get());
        TestComponentB_V0_1::Reflect(m_serializeContext.get());

        ReloadSliceAssetFromStream(sliceAssetId);
        ReloadSliceAssetFromStream(nestedSliceAssetId);

        instantiatedNestedSliceEntity0 = InstantiateSlice(nestedSliceAssetId);
        TestComponentB_V0_1* componentB1 = instantiatedNestedSliceEntity0->FindComponent<TestComponentB_V0_1>();
        ASSERT_NE(componentB1, nullptr);

        EXPECT_EQ(componentB1->m_unorderedMap.size(), 3);

        auto foundItr_B1 = componentB1->m_unorderedMap.find(17);
        EXPECT_NE(foundItr_B1, componentB1->m_unorderedMap.end());
        EXPECT_EQ(foundItr_B1->second.m_info, 30.5f);

        foundItr_B1 = componentB1->m_unorderedMap.find(29);
        EXPECT_NE(foundItr_B1, componentB1->m_unorderedMap.end());
        EXPECT_EQ(foundItr_B1->second.m_info, 105.5f);

        foundItr_B1 = componentB1->m_unorderedMap.find(37);
        EXPECT_NE(foundItr_B1, componentB1->m_unorderedMap.end());
        EXPECT_EQ(foundItr_B1->second.m_info, 50.5f);
    }

    TEST_F(SliceUpgradeTest, TypeChangeInVector)
    {
        TestDataB_V0::Reflect(m_serializeContext.get());
        AzToolsFramework::Components::EditorComponentBase::Reflect(m_serializeContext.get());
        TestComponentC_V0::Reflect(m_serializeContext.get());
        AZ::Entity* entityA = aznew AZ::Entity();
        TestComponentC_V0* componentC = entityA->CreateComponent<TestComponentC_V0>();
        componentC->m_vec.push_back(TestDataB_V0(17));
        componentC->m_vec.push_back(TestDataB_V0(29));
        componentC->m_vec.push_back(TestDataB_V0(37));
        AZ::Data::AssetId sliceAssetId = SaveAsSlice(entityA);
        entityA = nullptr;
        componentC = nullptr;

        AZ::Entity* instantiatedSliceEntity0 = InstantiateSlice(sliceAssetId);
        componentC = instantiatedSliceEntity0->FindComponent<TestComponentC_V0>();
        ASSERT_NE(componentC, nullptr);

        EXPECT_EQ(componentC->m_vec.size(), 3);

        EXPECT_EQ(componentC->m_vec[0].m_data, 17);
        EXPECT_EQ(componentC->m_vec[1].m_data, 29);
        EXPECT_EQ(componentC->m_vec[2].m_data, 37);

        // Creat a nested slice with overridding value.
        componentC->m_vec[1].m_data = 92;
        AZ::Data::AssetId nestedSliceAssetId = SaveAsSlice(instantiatedSliceEntity0);
        m_rootSliceComponent->RemoveEntity(instantiatedSliceEntity0, true, true);
        instantiatedSliceEntity0 = nullptr;

        AZ::Entity* instantiatedNestedSliceEntity0 = InstantiateSlice(nestedSliceAssetId);
        componentC = instantiatedNestedSliceEntity0->FindComponent<TestComponentC_V0>();
        ASSERT_NE(componentC, nullptr);

        EXPECT_EQ(componentC->m_vec.size(), 3);
        EXPECT_EQ(componentC->m_vec[1].m_data, 92);

        m_serializeContext->EnableRemoveReflection();
        TestComponentC_V0::Reflect(m_serializeContext.get());
        TestDataB_V0::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        TestDataB_V1::Reflect(m_serializeContext.get());
        TestComponentC_V0_1::Reflect(m_serializeContext.get());

        ReloadSliceAssetFromStream(sliceAssetId);
        ReloadSliceAssetFromStream(nestedSliceAssetId);

        instantiatedNestedSliceEntity0 = InstantiateSlice(nestedSliceAssetId);
        TestComponentC_V0_1* componentC1 = instantiatedNestedSliceEntity0->FindComponent<TestComponentC_V0_1>();
        ASSERT_NE(componentC1, nullptr);

        EXPECT_EQ(componentC1->m_vec.size(), 3);

        EXPECT_EQ(componentC1->m_vec[0].m_info, 30.5f);
        EXPECT_EQ(componentC1->m_vec[1].m_info, 105.5f);
        EXPECT_EQ(componentC1->m_vec[2].m_info, 50.5f);
    }

    TEST_F(SliceUpgradeTest, UpgradeSkipVersion_TypeChange_FloatToDouble)
    {
        // 1. Create an entity with a TestComponentE_V4 with the default value for m_data
        AzToolsFramework::Components::EditorComponentBase::Reflect(m_serializeContext.get());
        TestComponentE_V4::Reflect(m_serializeContext.get());
        AZ::Entity* testEntity = aznew AZ::Entity();
        TestComponentE_V4* componentEV4 = testEntity->CreateComponent<TestComponentE_V4>();
        componentEV4->m_data = V4_DefaultData;

        // 2. Create a slice out of our default entity configuration
        AZ::Data::AssetId sliceAssetId = SaveAsSlice(testEntity);

        // 3. Clean everything up
        componentEV4 = nullptr;
        testEntity = nullptr;

        // 4. Instantiate the slice we just created and verify that it contains default data
        AZ::Entity* instantiatedSliceEntity = InstantiateSlice(sliceAssetId);
        componentEV4 = instantiatedSliceEntity->FindComponent<TestComponentE_V4>();
        ASSERT_NE(componentEV4, nullptr);
        EXPECT_FLOAT_EQ(componentEV4->m_data, V4_DefaultData);

        // 5. Override the data in our new slice and save it as a nested slice.
        componentEV4->m_data = V4_OverrideData;
        AZ::Data::AssetId nestedSliceAssetId = SaveAsSlice(instantiatedSliceEntity);

        // 6. Clean everything up
        m_rootSliceComponent->RemoveEntity(instantiatedSliceEntity, true, true);
        componentEV4 = nullptr;
        instantiatedSliceEntity = nullptr;

        // 7. Instantiate the nested slice we just created and verify that it contains overridden data
        instantiatedSliceEntity = InstantiateSlice(nestedSliceAssetId);
        componentEV4 = instantiatedSliceEntity->FindComponent<TestComponentE_V4>();
        ASSERT_NE(componentEV4, nullptr);
        EXPECT_FLOAT_EQ(componentEV4->m_data, V4_OverrideData);

        // 8. Clean everything up
        m_rootSliceComponent->RemoveEntity(instantiatedSliceEntity, true, true);
        componentEV4 = nullptr;
        instantiatedSliceEntity = nullptr;

        // 9. Remove TestComponentE_V4 from the serialize context and add TestComponentE_V5
        m_serializeContext->EnableRemoveReflection();
        TestComponentE_V4::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        TestComponentE_V5::Reflect(m_serializeContext.get());

        // 10. Reload our slice assets
        ReloadSliceAssetFromStream(sliceAssetId);
        ReloadSliceAssetFromStream(nestedSliceAssetId);

        // 11. Instantiate our nested slice and verify that the V4->V5 upgrade has
        // been applied to the data patch and then patch has been properly applied
        instantiatedSliceEntity = InstantiateSlice(nestedSliceAssetId);
        TestComponentE_V5* componentEV5 = instantiatedSliceEntity->FindComponent<TestComponentE_V5>();
        ASSERT_NE(componentEV5, nullptr);
        EXPECT_EQ(componentEV5->m_data, V5_ExpectedData);

        // 12. Clean everything up
        m_rootSliceComponent->RemoveEntity(instantiatedSliceEntity, true, true);
        componentEV5 = nullptr;
        instantiatedSliceEntity = nullptr;

        // 13. Remove TestComponentE_V5 from the serialize context and add TestComponentE_V6_1
        m_serializeContext->EnableRemoveReflection();
        TestComponentE_V5::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        TestComponentE_V6_1::Reflect(m_serializeContext.get());

        // 14. Reload our slice assets
        ReloadSliceAssetFromStream(sliceAssetId);
        ReloadSliceAssetFromStream(nestedSliceAssetId);

        // 15. Instantiate our nested slice and verify that the V4->V5 and V5->V6 upgrades have
        // been applied to the data patch and then patch has been properly applied
        instantiatedSliceEntity = InstantiateSlice(nestedSliceAssetId);
        TestComponentE_V6_1* componentEV6_1 = instantiatedSliceEntity->FindComponent<TestComponentE_V6_1>();
        ASSERT_NE(componentEV6_1, nullptr);
        EXPECT_DOUBLE_EQ(componentEV6_1->m_data, V6_ExpectedData_NoSkip);

        // 16. Clean everything up
        m_rootSliceComponent->RemoveEntity(instantiatedSliceEntity, true, true);
        componentEV6_1 = nullptr;
        instantiatedSliceEntity = nullptr;

        // 17. Remove TestComponentE_V6_1 from the serialize context and add TestComponentE_V6_2
        m_serializeContext->EnableRemoveReflection();
        TestComponentE_V6_1::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        TestComponentE_V6_2::Reflect(m_serializeContext.get());

        // 18. Reload our slice assets
        ReloadSliceAssetFromStream(sliceAssetId);
        ReloadSliceAssetFromStream(nestedSliceAssetId);

        // 19. Instantiate our nested slice and verify that the V4->V6 upgrade has
        // been applied to the data patch and then patch has been properly applied
        instantiatedSliceEntity = InstantiateSlice(nestedSliceAssetId);
        TestComponentE_V6_2* componentEV6_2 = instantiatedSliceEntity->FindComponent<TestComponentE_V6_2>();
        ASSERT_NE(componentEV6_2, nullptr);
        EXPECT_TRUE(AZ::IsClose(componentEV6_2->m_data, V6_ExpectedData_Skip, 0.000001));

        // 20. Clean everything up
        m_rootSliceComponent->RemoveEntity(instantiatedSliceEntity, true, true);
        componentEV6_2 = nullptr;
        instantiatedSliceEntity = nullptr;
    }
 
    TEST_F(SliceUpgradeTest, TypeChangeTests)
    {
        // TEST TYPES
        SliceUpgradeTestAsset::Reflect(m_serializeContext.get());
        AzFramework::SimpleAssetReference<SliceUpgradeTestAsset>::Register(*m_serializeContext.get());

        AzToolsFramework::Components::EditorComponentBase::Reflect(m_serializeContext.get());
        TestComponentD_V1::Reflect(m_serializeContext.get());
        AZ::Entity* entity = aznew AZ::Entity();
        entity->CreateComponent<TestComponentD_V1>();
        // Supply a specific Asset Guid to help with debugging
        AZ::Data::AssetId sliceAssetId = SaveAsSlice(entity, AZ::Uuid{ "{10000000-0000-0000-0000-000000000000}" }, "datapatch_base.slice");
        entity = nullptr;

        AZ::Entity* instantiatedSliceEntity = InstantiateSlice(sliceAssetId);
        TestComponentD_V1* testComponent = instantiatedSliceEntity->FindComponent<TestComponentD_V1>();
        ASSERT_NE(testComponent, nullptr);
        EXPECT_EQ(testComponent->m_firstData, Value1_Initial);
        EXPECT_EQ(testComponent->m_secondData, Value2_Initial);
        EXPECT_EQ(testComponent->m_asset, AssetPath_Initial);

        // Create a nested slice with overridden data.
        testComponent->m_firstData = Value1_Override;
        testComponent->m_secondData = Value2_Override;
        testComponent->m_asset = AssetPath_Override;
        AZ::Data::AssetId nestedSliceAssetId = SaveAsSlice(instantiatedSliceEntity, AZ::Uuid{ "{20000000-0000-0000-0000-000000000000}" }, "datapatch_nested.slice");
        m_rootSliceComponent->RemoveEntity(instantiatedSliceEntity, true, true);
        instantiatedSliceEntity = nullptr;

        AZ::Entity* instantiatedNestedSliceEntity = InstantiateSlice(nestedSliceAssetId);
        testComponent = instantiatedNestedSliceEntity->FindComponent<TestComponentD_V1>();
        EXPECT_EQ(testComponent->m_firstData, Value1_Override);
        EXPECT_EQ(testComponent->m_secondData, Value2_Override);
        EXPECT_EQ(testComponent->m_asset, AssetPath_Override);

        m_rootSliceComponent->RemoveEntity(instantiatedNestedSliceEntity, true, true);
        instantiatedNestedSliceEntity = nullptr;

        // Replace TestComponentD_V1 in Serialization context with TestComponentD_V2.
        m_serializeContext->EnableRemoveReflection();
        TestComponentD_V1::Reflect(m_serializeContext.get());
        m_serializeContext->DisableRemoveReflection();
        TestComponentD_V2::Reflect(m_serializeContext.get());

        ReloadSliceAssetFromStream(sliceAssetId);
        ReloadSliceAssetFromStream(nestedSliceAssetId);

        instantiatedNestedSliceEntity = InstantiateSlice(nestedSliceAssetId);
        TestComponentD_V2* newTestComponent = instantiatedNestedSliceEntity->FindComponent<TestComponentD_V2>();
        ASSERT_NE(newTestComponent, nullptr);
        EXPECT_EQ(newTestComponent->m_firstData, Value1_Final);
        EXPECT_EQ(newTestComponent->m_secondData, Value2_Final);
        EXPECT_EQ(newTestComponent->m_asset.GetAssetPath(), AZStd::string(AssetPath_Override));
    }
} // namespace UnitTest

AZ_POP_DISABLE_WARNING
