/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PrefabBuilderTests.h"
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

namespace UnitTest
{
    AZ::Entity* CreateEntity(const char* entityName, AZStd::initializer_list<AZ::Component*> componentsToAdd = {})
    {
        // Circumvent the EntityContext system and generate a new entity with a transformcomponent
        AZ::Entity* newEntity = aznew AZ::Entity(entityName);
        newEntity->CreateComponent(AZ::TransformComponentTypeId);
        newEntity->Init();

        for (auto&& component : componentsToAdd)
        {
            newEntity->AddComponent(component);
        }

        newEntity->Activate();

        return newEntity;
    }

    TEST_F(PrefabBuilderTests, SourceDependencies)
    {
        static constexpr const char* ChildPrefabPath = "child.prefab";
        using namespace AzToolsFramework::Prefab;

        AZStd::vector<AZStd::unique_ptr<Instance>> childInstances;

        auto* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
        auto* prefabLoaderInterface = AZ::Interface<PrefabLoaderInterface>::Get();

        ASSERT_NE(prefabSystemComponentInterface, nullptr);
        ASSERT_NE(prefabLoaderInterface, nullptr);

        // Create a child entity and a prefab containing it
        childInstances.push_back(prefabSystemComponentInterface->CreatePrefab({CreateEntity("child")}, {}, ChildPrefabPath));

        // Create a parent entity and a prefab for it, pass in the child prefab for it to reference
        auto parentInstance = prefabSystemComponentInterface->CreatePrefab({CreateEntity("parent")}, AZStd::move(childInstances), "parent.prefab");
        
        AZStd::string serializedInstance;

        // Save to a string so we can load it as a PrefabDom and so that the nested instance becomes a Source file reference
        ASSERT_TRUE(prefabLoaderInterface->SaveTemplateToString(parentInstance->GetTemplateId(), serializedInstance));

        AZ::Outcome<PrefabDom, AZStd::string> readPrefabFileResult = AZ::JsonSerializationUtils::ReadJsonString(serializedInstance);

        ASSERT_TRUE(readPrefabFileResult.IsSuccess());

        // Now that we have a PrefabDom, test extracting the Source file reference as a Source Dependency
        auto sourceFileDependencies = AZ::Prefab::PrefabBuilderComponent::GetSourceDependencies(readPrefabFileResult.TakeValue());

        ASSERT_EQ(sourceFileDependencies.size(), 1);
        EXPECT_STREQ(sourceFileDependencies[0].m_sourceFileDependencyPath.c_str(), ChildPrefabPath);
    }

    TEST_F(PrefabBuilderTests, ProductDependencies)
    {
        constexpr const char* ChildPrefabPath = "child.prefab";
        using namespace AzToolsFramework::Prefab;

        AZ::Uuid TestAssetUuid("{7725567D-D420-46C2-B481-E0F79212CD34}");
        AZ::Data::AssetId TestAssetId(TestAssetUuid, 0);
        
        AZStd::vector<AZStd::unique_ptr<Instance>> childInstances;

        auto* prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();

        ASSERT_NE(prefabSystemComponentInterface, nullptr);

        auto* component = aznew TestComponent();
        AZ::Data::Asset<AZ::Data::AssetData> asset = AZ::Data::Asset<TestAsset>(TestAssetId, azrtti_typeid<TestAsset>());
        component->m_asset = asset;
        AZ::Entity* childEntity = CreateEntity("child", {component});

        // Create a child prefab with an entity that has an Asset<T> reference on it
        childInstances.push_back(prefabSystemComponentInterface->CreatePrefab(
            {childEntity}, {}, ChildPrefabPath, AZStd::unique_ptr<AZ::Entity>(CreateEntity("Container"))));

        // Create a parent prefab that has a nested instance reference to the child prefab
        auto parentInstance =
            prefabSystemComponentInterface->CreatePrefab({CreateEntity("parent")}, AZStd::move(childInstances), "parent.prefab", AZStd::unique_ptr<AZ::Entity>(CreateEntity("Container")));

        AZStd::string serializedInstance;
        
        TestPrefabBuilderComponent prefabBuilderComponent;
        prefabBuilderComponent.Activate();
        
        AZStd::vector<AssetBuilderSDK::JobProduct> jobProducts;
        auto&& prefabDom = prefabSystemComponentInterface->FindTemplateDom(parentInstance->GetTemplateId());

        ASSERT_TRUE(prefabBuilderComponent.ProcessPrefab({AZ::Crc32("pc")}, "parent.prefab", "unused", AZ::Uuid(), prefabDom, jobProducts));

        ASSERT_EQ(jobProducts.size(), 1);
        ASSERT_EQ(jobProducts[0].m_dependencies.size(), 1);
        ASSERT_EQ(jobProducts[0].m_dependencies[0].m_dependencyId, TestAssetId);

        prefabBuilderComponent.Deactivate();
    }

    AZStd::pair<size_t, size_t> GetFingerprint(AzToolsFramework::Prefab::PrefabDom& dom)
    {
        TestPrefabBuilderComponent prefabBuilderComponent;

        prefabBuilderComponent.Activate();
        size_t builderFingerprint = prefabBuilderComponent.CalculateBuilderFingerprint();
        size_t fingerprint = prefabBuilderComponent.CalculatePrefabFingerprint(dom);
        prefabBuilderComponent.Deactivate();

        return {fingerprint, builderFingerprint};
    }

    TEST_F(PrefabBuilderTests, FingerprintTest)
    {
        auto* prefabSystemComponentInterface = AZ::Interface<AzToolsFramework::Prefab::PrefabSystemComponentInterface>::Get();

        ASSERT_NE(prefabSystemComponentInterface, nullptr);
        
        auto* component = aznew TestComponent();
        AZ::Entity* entity = CreateEntity("test", {component});

        auto parentInstance = prefabSystemComponentInterface->CreatePrefab(
            {entity}, {}, "test.prefab",
            AZStd::unique_ptr<AZ::Entity>(CreateEntity("Container")));

        AZStd::vector<AssetBuilderSDK::JobProduct> jobProducts;
        auto&& prefabDom = prefabSystemComponentInterface->FindTemplateDom(parentInstance->GetTemplateId());
        
        auto [v0Dom, v0Builder] = GetFingerprint(prefabDom);
        auto [sanityDom, sanityBuilder] = GetFingerprint(prefabDom);

        // Make sure the fingerprint is stable without changes
        ASSERT_EQ(v0Dom, sanityDom);
        ASSERT_EQ(v0Builder, sanityBuilder);

        AZ::SerializeContext* context = m_app.GetSerializeContext();

        // Unreflect VersionChangingData, change its version, and reflect it again
        context->EnableRemoveReflection();
        VersionChangingData::Reflect(context);
        VersionChangingData::m_version = 1;
        context->DisableRemoveReflection();
        VersionChangingData::Reflect(context);

        // Get the new fingerprint and check that it changed
        auto [v1Dom, v1Builder] = GetFingerprint(prefabDom);

        ASSERT_NE(v0Dom, v1Dom); // Verify the fingerprint for the object changed
        ASSERT_NE(v0Builder, v1Builder); // Verify the fingerprint for the entire builder changed
    }

    AZStd::unique_ptr<AZ::IO::GenericStream> TestPrefabBuilderComponent::GetOutputStream(const AZ::IO::Path&) const
    {
        return AZStd::make_unique<SelfContainedMemoryStream>();
    }

    void PrefabBuilderTests::SetUp()
    {
        AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
        auto projectPathKey =
            AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
        registry->Set(projectPathKey, "AutomatedTesting");
        AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

        AZ::ComponentApplication::Descriptor desc;
        m_app.Start(desc);
        m_app.CreateReflectionManager();

        m_testComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>{TestComponent::CreateDescriptor()};
        m_testComponentDescriptor->Reflect(m_app.GetSerializeContext());
        TestAsset::Reflect(m_app.GetSerializeContext());
        VersionChangingData::Reflect(m_app.GetSerializeContext());

        // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
        // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash
        // in the unit tests.
        AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

        AZ::Data::AssetManager::Instance().RegisterHandler(&m_assetHandler, azrtti_typeid<TestAsset>());
    }

    void PrefabBuilderTests::TearDown()
    {
        AZ::Data::AssetManager::Instance().UnregisterHandler(&m_assetHandler);
        m_testComponentDescriptor = nullptr;

        m_app.Stop();
    }
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
