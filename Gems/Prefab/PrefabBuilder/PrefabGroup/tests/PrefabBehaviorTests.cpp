/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabBuilderTests.h>
#include <PrefabGroup/tests/PrefabBehaviorTestMocks.h>
#include <PrefabGroup/IPrefabGroup.h>
#include <PrefabGroup/PrefabGroup.h>
#include <PrefabGroup/PrefabGroupBehavior.h>
#include <AzTest/Utils.h>
#include <Tests/AssetSystemMocks.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Asset/AssetSystemComponent.h>
#include <AzToolsFramework/Prefab/Procedural/ProceduralPrefabAsset.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

#include <PrefabGroup/tests/PrefabBehaviorTests.inl>

namespace UnitTest
{
    class PrefabBehaviorTests
        : public PrefabBuilderTests
    {
    public:
        void SetUp() override
        {
            PrefabBuilderTests::SetUp();
            m_prefabGroupBehavior = AZStd::make_unique<AZ::SceneAPI::Behaviors::PrefabGroupBehavior>();
            m_prefabGroupBehavior->Activate();

            // Mocking the asset system replacing the AssetSystem::AssetSystemComponent
            AZ::Entity* systemEntity = m_app.FindEntity(AZ::SystemEntityId);
            systemEntity->FindComponent<AzToolsFramework::AssetSystem::AssetSystemComponent>()->Deactivate();
            using namespace testing;
            ON_CALL(m_assetSystemRequestMock, GetSourceInfoBySourcePath(_, _, _)).WillByDefault([](auto* path, auto& info, auto&)
            {
                return PrefabBehaviorTests::OnGetSourceInfoBySourcePath(path, info);
            });
            m_assetSystemRequestMock.BusConnect();

            m_componentDescriptorHelperEditorMeshComponent = AZStd::make_unique<AZ::Render::EditorMeshComponentHelper>();
            m_componentDescriptorHelperEditorMeshComponent->Reflect(m_app.GetSerializeContext());
            m_componentDescriptorHelperEditorMeshComponent->Reflect(m_app.GetBehaviorContext());
        }

        void TearDown() override
        {
            m_componentDescriptorHelperEditorMeshComponent.reset();

            m_assetSystemRequestMock.BusDisconnect();

            m_prefabGroupBehavior->Deactivate();
            m_prefabGroupBehavior.reset();

            PrefabBuilderTests::TearDown();
        }

        // mock classes and structures

        struct MockTransform : public AZ::SceneAPI::DataTypes::ITransform
        {
            AZ::Matrix3x4 m_matrix = AZ::Matrix3x4::CreateIdentity();

            AZ::Matrix3x4& GetMatrix() override
            {
                return m_matrix;
            }

            const AZ::Matrix3x4& GetMatrix() const
            {
                return m_matrix;
            }
        };

        struct TestPreExportEventContext
        {
            TestPreExportEventContext()
                : m_scene("test_context")
            {
                using namespace AZ::SceneAPI::Events;
                m_preExportEventContext = AZStd::make_unique<PreExportEventContext>(m_productList, m_outputDirectory, m_scene, "mock");
            }

            void SetOutputDirectory(AZStd::string outputDirectory)
            {
                using namespace AZ::SceneAPI::Events;
                m_outputDirectory = AZStd::move(outputDirectory);
                m_preExportEventContext = AZStd::make_unique<PreExportEventContext>(m_productList, m_outputDirectory, m_scene, "mock");
            }

            AZStd::unique_ptr<AZ::SceneAPI::Events::PreExportEventContext> m_preExportEventContext;
            AZ::SceneAPI::Events::ExportProductList m_productList;
            AZStd::string m_outputDirectory;
            AZ::SceneAPI::Containers::Scene m_scene;
        };

        class MockGraphMetaInfoBus
            : public AZ::SceneAPI::Events::GraphMetaInfoBus::Handler
        {
        public:
            MockGraphMetaInfoBus()
            {
                AZ::SceneAPI::Events::GraphMetaInfoBus::Handler::BusConnect();
            }

            ~MockGraphMetaInfoBus()
            {
                AZ::SceneAPI::Events::GraphMetaInfoBus::Handler::BusDisconnect();
            }

            MOCK_CONST_METHOD2(GetAppliedPolicyNames, void(AZStd::set<AZStd::string>&, const AZ::SceneAPI::Containers::Scene&));
        };

        // Helpers

        static bool OnGetSourceInfoBySourcePath(AZStd::string_view sourcePath, AZ::Data::AssetInfo& assetInfo)
        {
            if (sourcePath == AZStd::string_view("mock"))
            {
                assetInfo.m_assetId = AZ::Uuid::CreateRandom();
                assetInfo.m_assetType = azrtti_typeid<AZ::Prefab::ProceduralPrefabAsset>();
                assetInfo.m_relativePath = "mock/path";
                assetInfo.m_sizeBytes = 0;
            }
            return true;
        }

        const AZ::Entity* FindEntityByName(
            const AzToolsFramework::Prefab::Instance& instance,
            const AZStd::string& entityName)
        {
            const AZ::Entity* result = nullptr;
            instance.GetConstEntities(
                [&result, entityName](const AZ::Entity& entity)
                {
                    if (entity.GetName() != entityName)
                    {
                        return true;
                    }
                    else
                    {
                        result = &entity;
                        return false;
                    }
                });
            return result;
        }

        AZStd::shared_ptr<MockTransform> CreateMockTransform(AZ::Matrix3x4& matrix)
        {
            AZStd::shared_ptr<MockTransform> mocTransform = AZStd::make_shared<MockTransform>();
            AZ::Matrix3x4& mocMatrix = mocTransform->GetMatrix();
            mocMatrix = matrix;

            return mocTransform;
        }

        bool IsChildOfParent(
            const AzToolsFramework::Prefab::Instance& instance,
            const AzToolsFramework::Prefab::EntityAlias& childName,
            const AzToolsFramework::Prefab::EntityAlias& parentName)
        {
            const AZ::Entity* childEntity = FindEntityByName(instance, childName);
            const AZ::Entity* parentEntity = FindEntityByName(instance, parentName);

            if (childEntity && parentEntity)
            {
                AzToolsFramework::Components::TransformComponent* childTransform =
                    childEntity->FindComponent<AzToolsFramework::Components::TransformComponent>();

                if (childTransform)
                {
                    return (childTransform->GetParentId() == parentEntity->GetId());
                }
            }

            return false;
        }

        AZStd::unique_ptr<AZ::SceneAPI::Behaviors::PrefabGroupBehavior> m_prefabGroupBehavior;
        testing::NiceMock<UnitTests::MockAssetSystemRequest> m_assetSystemRequestMock;
        AZStd::unique_ptr<AZ::Render::EditorMeshComponentHelper> m_componentDescriptorHelperEditorMeshComponent;
    };

    TEST_F(PrefabBehaviorTests, PrefabBehavior_EmptyContextIgnored_Works)
    {
        auto context = TestPreExportEventContext{};

        auto result = AZ::SceneAPI::Events::ProcessingResult::Failure;
        AZ::SceneAPI::Events::CallProcessorBus::BroadcastResult(
            result,
            &AZ::SceneAPI::Events::CallProcessorBus::Events::Process,
            context.m_preExportEventContext.get());

        EXPECT_EQ(result, AZ::SceneAPI::Events::ProcessingResult::Ignored);
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_SimplePrefab_Works)
    {
        auto context = TestPreExportEventContext{};

        // check for the file at <temp_directory>/mock/fake_prefab.procprefab
        AZ::Test::ScopedAutoTempDirectory tempDir;
        context.SetOutputDirectory(tempDir.GetDirectory());

        auto jsonOutcome = AZ::JsonSerializationUtils::ReadJsonString(Data::jsonPrefab);
        ASSERT_TRUE(jsonOutcome);

        // Register the asset to generate an AssetId in the catalog
        AZ::Data::AssetId assetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            assetId, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath, "fake_prefab.procprefab",
            azrtti_typeid<AZ::Prefab::ProceduralPrefabAsset>(), true);

        auto prefabGroup = AZStd::make_shared<AZ::SceneAPI::SceneData::PrefabGroup>();
        prefabGroup.get()->SetId(AZ::Uuid::CreateRandom());
        prefabGroup.get()->SetName("fake_prefab");
        prefabGroup.get()->SetPrefabDom(AZStd::move(jsonOutcome.GetValue()));
        context.m_scene.GetManifest().AddEntry(prefabGroup);
        context.m_scene.SetSource("mock", AZ::Uuid::CreateRandom());

        auto result = AZ::SceneAPI::Events::ProcessingResult::Failure;
        AZ::SceneAPI::Events::CallProcessorBus::BroadcastResult(
            result,
            &AZ::SceneAPI::Events::CallProcessorBus::Events::Process,
            context.m_preExportEventContext.get());

        EXPECT_EQ(result, AZ::SceneAPI::Events::ProcessingResult::Success);

        AZStd::string pathStr;
        AzFramework::StringFunc::Path::ConstructFull(tempDir.GetDirectory(), "mock/fake_prefab.procprefab", pathStr, true);
        if (!AZ::IO::SystemFile::Exists(pathStr.c_str()))
        {
            AZ_Warning("testing", false, "The product asset (%s) is missing", pathStr.c_str());
        }
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_UpdateManifestWithEmptyScene_DoesNotFail)
    {
        using namespace AZ::SceneAPI;
        using namespace AZ::SceneAPI::Events;

        Containers::Scene scene("empty_scene");
        AssetImportRequest::ManifestAction action = AssetImportRequest::ManifestAction::ConstructDefault;
        AssetImportRequest::RequestingApplication requester = {};

        ProcessingResult result = ProcessingResult::Failure;
        AssetImportRequestBus::BroadcastResult(result, &AssetImportRequestBus::Events::UpdateManifest, scene, action, requester);
        EXPECT_NE(result, ProcessingResult::Failure);
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_UpdateManifestWithEmptyScene_Ignored)
    {
        using namespace AZ::SceneAPI;
        using namespace AZ::SceneAPI::Events;

        Containers::Scene scene("empty_scene");
        AssetImportRequest::ManifestAction action = AssetImportRequest::ManifestAction::Update;
        AssetImportRequest::RequestingApplication requester = {};

        Behaviors::PrefabGroupBehavior prefabGroupBehavior;
        ProcessingResult result = ProcessingResult::Failure;
        AssetImportRequestBus::BroadcastResult(result, &AssetImportRequestBus::Events::UpdateManifest, scene, action, requester);
        EXPECT_EQ(result, ProcessingResult::Ignored);
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_UpdateManifestMockScene_CreatesPrefab)
    {
        using namespace AZ::SceneAPI;
        using namespace AZ::SceneAPI::Events;

        #if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        auto scene = CreateMockScene("Manifest", "C:/o3de/watch.folder/manifest_src_file.xml", "C:/o3de/watch.folder");
        #else
        auto scene = CreateMockScene("Manifest", "//o3de/watch.folder/manifest_src_file.xml", "//o3de/watch.folder");
        #endif
        AssetImportRequest::ManifestAction action = AssetImportRequest::ManifestAction::ConstructDefault;
        AssetImportRequest::RequestingApplication requester = {};

        Behaviors::PrefabGroupBehavior prefabGroupBehavior;
        ProcessingResult result = ProcessingResult::Failure;
        AssetImportRequestBus::BroadcastResult(result, &AssetImportRequestBus::Events::UpdateManifest, *scene, action, requester);

        EXPECT_EQ(result, ProcessingResult::Success);
        EXPECT_EQ(scene->GetManifest().GetEntryCount(), 3);

        EXPECT_TRUE(azrtti_istypeof<AZ::SceneAPI::DataTypes::IMeshGroup>(scene->GetManifest().GetValue(0).get()));
        EXPECT_TRUE(azrtti_istypeof<AZ::SceneAPI::DataTypes::IMeshGroup>(scene->GetManifest().GetValue(1).get()));
        EXPECT_TRUE(azrtti_istypeof<AZ::SceneAPI::DataTypes::IPrefabGroup>(scene->GetManifest().GetValue(2).get()));

        // The mesh group names are expected to be just the file name relative to the watch folder and not any absolute path
        for (size_t i = 0; i < scene->GetManifest().GetEntryCount(); i++)
        {
            if (azrtti_istypeof<AZ::SceneAPI::DataTypes::IMeshGroup>(scene->GetManifest().GetValue(i).get()))
            {
                AZ::SceneAPI::DataTypes::IMeshGroup* meshGroup = reinterpret_cast<AZ::SceneAPI::DataTypes::IMeshGroup*>(scene->GetManifest().GetValue(i).get());
                AZStd::string groupName = meshGroup->GetName();
                EXPECT_TRUE(groupName.starts_with("default_mock_"));
            }
        }
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_EntityHierarchy_MatchesSceneNodeHierarchy)
    {
        using namespace AZ::SceneAPI;
        using namespace AZ::SceneAPI::Events;

#if AZ_TRAIT_OS_USE_WINDOWS_FILE_PATHS
        auto scene = CreateEmptyMockSceneWithRoot("Manifest", "C:/o3de/watch.folder/manifest_src_file.xml", "C:/o3de/watch.folder");
#else
        auto scene = CreateEmptyMockSceneWithRoot("Manifest", "//o3de/watch.folder/manifest_src_file.xml", "//o3de/watch.folder");
#endif
        /*---------------------------------------\
        Notes on the graph hierarchy:
        - 3m is a node with mesh data. Proc prefab builder will create an entity for this node
        - 4t contains the transform data for the mesh. It's a child of 3m because a node can only contain one data item.
          Proc prefab builder will apply the data of the first child transform to the mesh entity's transform component
        - 5t is a standalone child transform node of the mesh node. Proc prefab builder will create an entity for this node

                    Root
                     |
                     1
                     |
                     2t
                   /   \
                  3m    6t
                 /  \
                4t  5t
        \---------------------------------------*/

        AZ::Matrix3x4 nonIdentityMatrix = AZ::Matrix3x4::CreateScale(AZ::Vector3(10.0f));

        // Build up the graph
        auto root = scene->GetGraph().GetRoot();
        auto index1 = scene->GetGraph().AddChild(root, "1", AZStd::make_shared<DataTypes::MockIGraphObject>(1));
        auto index2 = scene->GetGraph().AddChild(index1, "2", CreateMockTransform(nonIdentityMatrix));
        auto index3 = scene->GetGraph().AddChild(index2, "3", AZStd::make_shared<AZ::SceneData::GraphData::MeshData>());
        auto index4 = scene->GetGraph().AddChild(index3, "4", CreateMockTransform(nonIdentityMatrix));
        auto index5 = scene->GetGraph().AddChild(index3, "5", CreateMockTransform(nonIdentityMatrix));
        auto index6 = scene->GetGraph().AddChild(index2, "6", CreateMockTransform(nonIdentityMatrix));

        scene->GetGraph().MakeEndPoint(index4);
        scene->GetGraph().MakeEndPoint(index5);
        scene->GetGraph().MakeEndPoint(index6);

        AssetImportRequest::ManifestAction action = AssetImportRequest::ManifestAction::ConstructDefault;
        AssetImportRequest::RequestingApplication requester = {};

        Behaviors::PrefabGroupBehavior prefabGroupBehavior;
        ProcessingResult result = ProcessingResult::Failure;
        AssetImportRequestBus::BroadcastResult(result, &AssetImportRequestBus::Events::UpdateManifest, *scene, action, requester);

        constexpr size_t expectedEntryCount = 2;

        EXPECT_EQ(result, ProcessingResult::Success);
        EXPECT_EQ(scene->GetManifest().GetEntryCount(), expectedEntryCount);

        EXPECT_TRUE(azrtti_istypeof<AZ::SceneAPI::DataTypes::IPrefabGroup>(scene->GetManifest().GetValue(expectedEntryCount - 1).get()));

        AZ::SceneAPI::DataTypes::IPrefabGroup* prefabGroup =
            reinterpret_cast<AZ::SceneAPI::DataTypes::IPrefabGroup*>(scene->GetManifest().GetValue(expectedEntryCount - 1).get());
        AzToolsFramework::Prefab::PrefabDomConstReference prefabDomRef = prefabGroup->GetPrefabDomRef();
        ASSERT_TRUE(prefabDomRef.has_value());

        // Check that the entity hierarchy of the prefab group is correct.
        // Each mesh and each transform not associated with any mesh should have a unique entity
        AzToolsFramework::Prefab::Instance instance;
        ASSERT_TRUE(AzToolsFramework::Prefab::PrefabDomUtils::LoadInstanceFromPrefabDom(instance, *prefabDomRef));
        EXPECT_TRUE(IsChildOfParent(instance, "3", "2")); // Mesh entity is child of a transform entity
        EXPECT_TRUE(IsChildOfParent(instance, "6", "2")); // Transform entity is child of another transform entity
        EXPECT_FALSE(IsChildOfParent(instance, "4", "3")); // First transform entity is not a child of the mesh entity
        EXPECT_TRUE(IsChildOfParent(instance, "5", "3")); // Second transform entity is child of the mesh entity
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_UpdateManifest_ToggleWorks)
    {
        using namespace AZ::SceneAPI;
        using namespace AZ::SceneAPI::Events;

        ASSERT_TRUE(AZ::SettingsRegistry::Get());
        AZ::SettingsRegistry::Get()->Set("/O3DE/Preferences/Prefabs/CreateDefaults", false);

        auto scene = CreateMockScene();
        AssetImportRequest::ManifestAction action = AssetImportRequest::ManifestAction::ConstructDefault;
        AssetImportRequest::RequestingApplication requester = {};

        Behaviors::PrefabGroupBehavior prefabGroupBehavior;
        ProcessingResult result = ProcessingResult::Failure;
        AssetImportRequestBus::BroadcastResult(result, &AssetImportRequestBus::Events::UpdateManifest, *scene, action, requester);
        EXPECT_EQ(result, ProcessingResult::Ignored);
        EXPECT_EQ(scene->GetManifest().GetEntryCount(), 0);
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_IgnoreActors_ToggleTrueReturnsIgnored)
    {
        using namespace AZ::SceneAPI;
        using namespace AZ::SceneAPI::Events;

        ::testing::NiceMock<MockGraphMetaInfoBus> mockGraphMetaInfoBus;

        ON_CALL(mockGraphMetaInfoBus, GetAppliedPolicyNames)
            .WillByDefault([](AZStd::set<AZStd::string>& appliedPolicies, const Containers::Scene&)
            {
                appliedPolicies.insert("ActorGroupBehavior");
            });

        ASSERT_TRUE(AZ::SettingsRegistry::Get());
        AZ::SettingsRegistry::Get()->Set("/O3DE/Preferences/Prefabs/IgnoreActors", true);

        auto scene = CreateMockScene();
        AssetImportRequest::ManifestAction action = AssetImportRequest::ManifestAction::ConstructDefault;
        AssetImportRequest::RequestingApplication requester = {};

        Behaviors::PrefabGroupBehavior prefabGroupBehavior;
        ProcessingResult result = ProcessingResult::Failure;
        AssetImportRequestBus::BroadcastResult(result, &AssetImportRequestBus::Events::UpdateManifest, *scene, action, requester);
        EXPECT_EQ(result, ProcessingResult::Ignored);
        EXPECT_EQ(scene->GetManifest().GetEntryCount(), 0);
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_IgnoreActors_ToggleTrueReturnsSuccessWhenNoActorDetected)
    {
        using namespace AZ::SceneAPI;
        using namespace AZ::SceneAPI::Events;

        ::testing::NiceMock<MockGraphMetaInfoBus> mockGraphMetaInfoBus;

        ASSERT_TRUE(AZ::SettingsRegistry::Get());
        AZ::SettingsRegistry::Get()->Set("/O3DE/Preferences/Prefabs/IgnoreActors", true);

        auto scene = CreateMockScene();
        AssetImportRequest::ManifestAction action = AssetImportRequest::ManifestAction::ConstructDefault;
        AssetImportRequest::RequestingApplication requester = {};

        Behaviors::PrefabGroupBehavior prefabGroupBehavior;
        ProcessingResult result = ProcessingResult::Failure;
        AssetImportRequestBus::BroadcastResult(result, &AssetImportRequestBus::Events::UpdateManifest, *scene, action, requester);
        EXPECT_EQ(result, ProcessingResult::Success);
        EXPECT_EQ(scene->GetManifest().GetEntryCount(), 3);
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_IgnoreActors_ToggleFalseReturnsSuccess)
    {
        using namespace AZ::SceneAPI;
        using namespace AZ::SceneAPI::Events;

        ::testing::NiceMock<MockGraphMetaInfoBus> mockGraphMetaInfoBus;

        ON_CALL(mockGraphMetaInfoBus, GetAppliedPolicyNames)
            .WillByDefault([](AZStd::set<AZStd::string>& appliedPolicies, const Containers::Scene&)
            {
                appliedPolicies.insert("ActorGroupBehavior");
            });

        ASSERT_TRUE(AZ::SettingsRegistry::Get());
        AZ::SettingsRegistry::Get()->Set("/O3DE/Preferences/Prefabs/IgnoreActors", false);

        auto scene = CreateMockScene();
        AssetImportRequest::ManifestAction action = AssetImportRequest::ManifestAction::ConstructDefault;
        AssetImportRequest::RequestingApplication requester = {};

        Behaviors::PrefabGroupBehavior prefabGroupBehavior;
        ProcessingResult result = ProcessingResult::Failure;
        AssetImportRequestBus::BroadcastResult(result, &AssetImportRequestBus::Events::UpdateManifest, *scene, action, requester);
        EXPECT_EQ(result, ProcessingResult::Success);
        EXPECT_EQ(scene->GetManifest().GetEntryCount(), 3);
    }
}
