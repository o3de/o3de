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

#include "precompiled.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzTest/AzTest.h>
#include <Builder/ScriptCanvasBuilderWorker.h>

#include "ScriptCanvas/Asset/RuntimeAsset.h"

struct MockAsset
    : public AZ::Data::AssetData
{
    AZ_RTTI(MockAsset, "{D1E5A5DA-89D3-4B1F-8194-3E84CA6991DF}", AZ::Data::AssetData);

    static void Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<MockAsset>()
                ->Field("value", &MockAsset::m_value);
        }
    }

    int m_value = 0;
};

struct MockAssetRefComponent
    : public AZ::Component
{
    AZ_COMPONENT(MockAssetRefComponent, "{EE1F3C90-2301-483D-AE28-9381BBFE18FB}");

    static void Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);

        if (serializeContext)
        {
            serializeContext->Class<MockAssetRefComponent, AZ::Component>()
                ->Field("asset", &MockAssetRefComponent::m_asset);
        }
    }

    void Activate() override {}
    void Deactivate() override {}

    AZ::Data::Asset<MockAsset> m_asset;
};

class ScriptCanvasBuilderTests
    : public UnitTest::AllocatorsFixture
    , public AZ::ComponentApplicationBus::Handler
{
protected:

    //////////////////////////////////////////////////////////////////////////
    // ComponentApplicationMessages
    AZ::ComponentApplication* GetApplication() override { return nullptr; }
    void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
    void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
    bool AddEntity(AZ::Entity*) override { return true; }
    bool RemoveEntity(AZ::Entity*) override { return true; }
    bool DeleteEntity(const AZ::EntityId&) override { return true; }
    AZ::Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
    AZ::SerializeContext* GetSerializeContext() override { return m_serializeContext; }
    AZ::BehaviorContext*  GetBehaviorContext() override { return nullptr; }
    AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
    const char* GetExecutableFolder() const override { return nullptr; }
    const char* GetAppRoot() const override { return nullptr; }
    AZ::Debug::DrillerManager* GetDrillerManager() override { return nullptr; }
    void EnumerateEntities(const AZ::ComponentApplicationRequests::EntityCallback& /*callback*/) override {}
    void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
    //////////////////////////////////////////////////////////////////////////

    void SetUp() override
    {
        UnitTest::AllocatorsFixture::SetUp();

        AZ::AllocatorInstance<AZ::PoolAllocator>::Create();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Create();

        m_serializeContext = aznew AZ::SerializeContext(true, true);
        AZ::ComponentApplicationBus::Handler::BusConnect();

        m_mockAssetDescriptor = MockAssetRefComponent::CreateDescriptor();
        MockAssetRefComponent::Reflect(m_serializeContext);
        MockAsset::Reflect(m_serializeContext);
        ScriptCanvas::RuntimeData::Reflect(m_serializeContext);
        ScriptCanvas::GraphData::Reflect(m_serializeContext);
        ScriptCanvas::VariableData::Reflect(m_serializeContext);
        AZ::Entity::Reflect(m_serializeContext);
        AZ::Data::AssetManager::Descriptor desc;
        AZ::Data::AssetManager::Create(desc);
        AZ::Data::AssetManager::Instance().RegisterHandler(aznew AzFramework::GenericAssetHandler<MockAsset>("Mock Asset", "Other", "mockasset"), AZ::AzTypeInfo<MockAsset>::Uuid());

        AZ::Data::AssetManager::Instance().RegisterHandler(aznew AzFramework::GenericAssetHandler<ScriptCanvas::RuntimeAsset>("ScriptCanvas::RuntimeAsset", "Other", "mockasset"), azrtti_typeid<ScriptCanvas::RuntimeAsset>());
    }

    void TearDown() override
    {
        AZ::ComponentApplicationBus::Handler::BusDisconnect();

        AZ::Data::AssetManager::Destroy();

        delete m_mockAssetDescriptor;

        delete m_serializeContext;

        AZ::AllocatorInstance<AZ::PoolAllocator>::Destroy();
        AZ::AllocatorInstance<AZ::ThreadPoolAllocator>::Destroy();

        UnitTest::AllocatorsFixture::TearDown();
    }

    AZ::SerializeContext* m_serializeContext = nullptr;
    AZ::ComponentDescriptor* m_mockAssetDescriptor = nullptr;
};

// Just test for one case to verify the call to ScriptCanvasBuilder::Worker::GatherProductDependencies works.
// SerializationDependencyTests handles testing other asset reference types.
TEST_F(ScriptCanvasBuilderTests, ScriptCanvasWithAssetReference_GatherProductDependencies_DependencyFound)
{
    MockAssetRefComponent* assetComponent = aznew MockAssetRefComponent;
    AZ::Data::AssetId testAssetId("{CAAC5458-0738-43F6-A2BD-4E315C64BFD3}", 71);
    assetComponent->m_asset =
        AZ::Data::AssetManager::Instance().CreateAsset<MockAsset>(testAssetId, AZ::Data::AssetLoadBehavior::Default);

    AZ::Entity* graphEntity = aznew AZ::Entity;
    graphEntity->AddComponent(assetComponent);

    ScriptCanvas::RuntimeData runtimeData;
    runtimeData.m_graphData.m_nodes.emplace(graphEntity);

    AZ::Data::Asset<ScriptCanvas::RuntimeAsset> runtimeAsset;
    runtimeAsset.Create(AZ::Uuid::CreateRandom());
    runtimeAsset.Get()->SetData(runtimeData);

    AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
    AssetBuilderSDK::ProductPathDependencySet productPathDependencySet;
    bool gatherResults = ScriptCanvasBuilder::Worker::GatherProductDependencies(
        *m_serializeContext,
        runtimeAsset,
        productDependencies,
        productPathDependencySet);
    ASSERT_TRUE(gatherResults);
    ASSERT_EQ(productDependencies.size(), 1);
    ASSERT_EQ(productDependencies[0].m_dependencyId, testAssetId);
    ASSERT_EQ(productPathDependencySet.size(), 0);
}
