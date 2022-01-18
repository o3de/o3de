/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabBuilderTests.h>
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
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Mocks/DataTypes/MockIGraphObject.h>
#include <SceneAPI/SceneCore/SceneCoreStandaloneAllocator.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/SceneDataStandaloneAllocator.h>

#include <PrefabGroup/PrefabBehaviorTests.inl>

// a mock AZ::Render::EditorMeshComponent
namespace AZ::Render
{
    struct EditorMeshComponent
        : public AZ::Component
    {
        AZ_COMPONENT(EditorMeshComponent, "{DCE68F6E-2E16-4CB4-A834-B6C2F900A7E9}");
        void Activate() override {}
        void Deactivate() override {}

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<AZ::Render::EditorMeshComponent>();
            }

            if (BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<EditorMeshComponent>("AZ::Render::EditorMeshComponent");
            }

        }
    };

    struct EditorMeshComponentHelper
        : public AZ::ComponentDescriptorHelper<EditorMeshComponent>
    {
        AZ_CLASS_ALLOCATOR(EditorMeshComponentHelper, AZ::SystemAllocator, 0);

        void Reflect(AZ::ReflectContext* reflection) const override
        {
            AZ::Render::EditorMeshComponent::Reflect(reflection);
        }
    };
}

namespace UnitTest
{
    class PrefabBehaviorTests
        : public PrefabBuilderTests
    {
    public:
        static void SetUpTestCase()
        {
            // Allocator needed by SceneCore
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>().IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>().Create();
            }
            AZ::SceneAPI::SceneCoreStandaloneAllocator::Initialize(AZ::Environment::GetInstance());
            AZ::SceneAPI::SceneDataStandaloneAllocator::Initialize(AZ::Environment::GetInstance());
        }

        static void TearDownTestCase()
        {
            AZ::SceneAPI::SceneDataStandaloneAllocator::TearDown();
            AZ::SceneAPI::SceneCoreStandaloneAllocator::TearDown();
            AZ::AllocatorInstance<AZ::SystemAllocator>().Destroy();
        }

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

        AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> CreateMockScene()
        {
            using namespace AZ::SceneAPI;

            auto scene = AZStd::make_shared<Containers::Scene>("mock_scene");
            scene->SetManifestFilename("ManifestFilename");
            scene->SetSource("Source", AZ::Uuid::CreateRandom());
            scene->SetWatchFolder("WatchFolder");

            /*---------------------------------------\
                        Root
                         |
                         1
                         |
                         2
                       /   \
                ------3m    7
               /  /  /        \
              6  5  4t         8m-------
                                \   \   \
                                 9t 10  11
            \---------------------------------------*/

            // Build up the graph
            auto root = scene->GetGraph().GetRoot();
            scene->GetGraph().SetContent(root, AZStd::make_shared<DataTypes::MockIGraphObject>(0));
            auto index1 = scene->GetGraph().AddChild(root, "1", AZStd::make_shared<DataTypes::MockIGraphObject>(1));
            auto index2 = scene->GetGraph().AddChild(index1, "2", AZStd::make_shared<DataTypes::MockIGraphObject>(2));
            auto index3 = scene->GetGraph().AddChild(index2, "3", AZStd::make_shared<AZ::SceneData::GraphData::MeshData>());
            auto index4 = scene->GetGraph().AddChild(index3, "4", AZStd::make_shared<MockTransform>());
            auto index5 = scene->GetGraph().AddChild(index3, "5", AZStd::make_shared<DataTypes::MockIGraphObject>(5));
            auto index6 = scene->GetGraph().AddChild(index3, "6", AZStd::make_shared<DataTypes::MockIGraphObject>(6));
            auto index7 = scene->GetGraph().AddChild(index2, "7", AZStd::make_shared<DataTypes::MockIGraphObject>(7));
            auto index8 = scene->GetGraph().AddChild(index7, "8", AZStd::make_shared<AZ::SceneData::GraphData::MeshData>());
            auto index9 = scene->GetGraph().AddChild(index8, "9", AZStd::make_shared<MockTransform>());
            auto index10 = scene->GetGraph().AddChild(index8, "10", AZStd::make_shared<DataTypes::MockIGraphObject>(10));
            auto index11 = scene->GetGraph().AddChild(index8, "11", AZStd::make_shared<DataTypes::MockIGraphObject>(11));

            scene->GetGraph().MakeEndPoint(index4);
            scene->GetGraph().MakeEndPoint(index5);
            scene->GetGraph().MakeEndPoint(index6);
            scene->GetGraph().MakeEndPoint(index9);
            scene->GetGraph().MakeEndPoint(index10);
            scene->GetGraph().MakeEndPoint(index11);

            return scene;
        }

        // mock classes and structures

        struct MockTransform
            : public AZ::SceneAPI::DataTypes::ITransform
        {
            AZ::Matrix3x4 m_matrix;

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

    TEST_F(PrefabBehaviorTests, PrefabBehavior_UpdateManifestWithEmtpyScene_DoesNotFail)
    {
        using namespace AZ::SceneAPI;
        using namespace AZ::SceneAPI::Events;

        Containers::Scene scene("empty_scene");
        AssetImportRequest::ManifestAction action = AssetImportRequest::ManifestAction::ConstructDefault;
        AssetImportRequest::RequestingApplication requester = {};

        Behaviors::PrefabGroupBehavior prefabGroupBehavior;
        ProcessingResult result = ProcessingResult::Failure;
        AssetImportRequestBus::BroadcastResult(result, &AssetImportRequestBus::Events::UpdateManifest, scene, action, requester);
        EXPECT_NE(result,  ProcessingResult::Failure);
    }

    TEST_F(PrefabBehaviorTests, PrefabBehavior_UpdateManifestWithEmtpyScene_Ignored)
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

        auto scene = CreateMockScene();
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
}
