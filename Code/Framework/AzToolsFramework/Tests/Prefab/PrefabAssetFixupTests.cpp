/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Component/Component.h>
#include <Prefab/PrefabTestFixture.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>

namespace UnitTest
{
    using PrefabInstantiateTest = PrefabTestFixture;

    struct MockAsset : AZ::Data::AssetData
    {
        AZ_RTTI(MockAsset, "{DAB98A3F-1714-4B95-AACB-8C150B0D0628}", AZ::Data::AssetData);

        AZ_CLASS_ALLOCATOR(MockAsset, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MockAsset>()->Field("data", &MockAsset::m_data);
            }
        }
        float m_data = 1.f;
    };

    struct MockAssetComponent : AZ::Component
    {
        AZ_COMPONENT(MockAssetComponent, "{D81B0D06-B495-479E-832A-A63079FD6D37}");

        static void Reflect(AZ::ReflectContext* context)
        {
            MockAsset::Reflect(context);

            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MockAssetComponent>()
                    ->Field("asset", &MockAssetComponent::m_asset);
            }
        }

        void Activate() override{}
        void Deactivate() override{}

        AZ::Data::Asset<MockAsset> m_asset;
    };

    class MockAssetHandler : public AZ::Data::AssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(MockAssetHandler, AZ::SystemAllocator, 0);

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override
        {
            (void)id;
            EXPECT_TRUE(type == azrtti_typeid<MockAsset>());
            if (type == azrtti_typeid<MockAsset>())
            {
                return aznew MockAsset();
            }
            return nullptr;
        }

        LoadResult LoadAssetData(const AZ::Data::Asset<AZ::Data::AssetData>&, AZStd::shared_ptr<AZ::Data::AssetDataStream>, const AZ::Data::AssetFilterCB&) override
        {
            return LoadResult::Error;
        }

        void DestroyAsset(AZ::Data::AssetPtr ptr) override
        {
            EXPECT_TRUE(ptr->GetType() == azrtti_typeid<MockAsset>());
            delete ptr;
        }

        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override
        {
            assetTypes.push_back(azrtti_typeid<MockAsset>());
        }
    };

    struct PrefabFixupTest : PrefabInstantiateTest
    {
        void SetUpEditorFixtureImpl() override
        {
            PrefabInstantiateTest::SetUpEditorFixtureImpl();

            AZ::SerializeContext* context = nullptr;

            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            ASSERT_NE(context, nullptr);

            MockAssetComponent::Reflect(context);

            AZ::Data::AssetManager::Instance().RegisterHandler(&m_handler, azrtti_typeid<MockAsset>());

            auto entity = aznew AZ::Entity();
            auto mockAssetComponent = entity->CreateComponent<MockAssetComponent>();

            mockAssetComponent->m_asset =
                AZ::Data::Asset<MockAsset>(AZ::Uuid::CreateNull(), AZ::Data::AssetType::CreateNull(), "test.asset");

            auto newInstance = AZ::Interface<PrefabSystemComponentInterface>::Get()->CreatePrefab({ entity }, {}, "test.prefab");

            AZStd::string prefabString;
            ASSERT_TRUE(m_prefabLoaderInterface->SaveTemplateToString(newInstance->GetTemplateId(), prefabString));
            m_prefabSystemComponent->RemoveAllTemplates();

            AZ::Outcome<PrefabDom, AZStd::string> readPrefabFileResult = AZ::JsonSerializationUtils::ReadJsonString(prefabString);

            ASSERT_TRUE(readPrefabFileResult.IsSuccess());

            AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                m_assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, "test.asset", azrtti_typeid<MockAsset>(),
                true); // True to register the asset and generate an AssetId for lookup

            m_prefabDom = readPrefabFileResult.TakeValue();
        }

        void TearDownEditorFixtureImpl() override
        {
            PrefabInstantiateTest::TearDownEditorFixtureImpl();

            AZ::Data::AssetManager::Instance().UnregisterHandler(&m_handler);
        }

        void CheckInstance(const Instance& instance)
        {
            const AZ::Entity* loadedEntity = nullptr;
            instance.GetConstEntities(
                [&loadedEntity](const AZ::Entity& entity)
                {
                    loadedEntity = &entity;

                    return false;
                });

            auto loadedComponent = loadedEntity->FindComponent<MockAssetComponent>();

            ASSERT_NE(loadedComponent, nullptr);

            ASSERT_STREQ(loadedComponent->m_asset.GetHint().c_str(), "test.asset");
            ASSERT_EQ(loadedComponent->m_asset->GetId(), m_assetId);
        }

        MockAssetHandler m_handler;
        PrefabDom m_prefabDom;
        AZ::Data::AssetId m_assetId;
    };

    TEST_F(PrefabFixupTest, Test_LoadInstanceFromPrefabDom_Overload1)
    {
        Instance instance;
        ASSERT_TRUE(PrefabDomUtils::LoadInstanceFromPrefabDom(instance, m_prefabDom));

        CheckInstance(instance);
    }

    TEST_F(PrefabFixupTest, Test_LoadInstanceFromPrefabDom_Overload2)
    {
        Instance instance;
        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> referencedAssets;
        ASSERT_TRUE(PrefabDomUtils::LoadInstanceFromPrefabDom(instance, m_prefabDom, referencedAssets));

        CheckInstance(instance);
    }

    TEST_F(PrefabFixupTest, Test_LoadInstanceFromPrefabDom_Overload3)
    {
        Instance instance;
        AZStd::vector<AZ::Data::Asset<AZ::Data::AssetData>> referencedAssets;
        Instance::EntityList entityList;
        (PrefabDomUtils::LoadInstanceFromPrefabDom(instance, entityList, m_prefabDom));

        CheckInstance(instance);
    }
}
