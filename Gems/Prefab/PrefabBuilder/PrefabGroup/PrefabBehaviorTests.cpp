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
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/SceneCoreStandaloneAllocator.h>

#include <PrefabGroup/PrefabBehaviorTests.inl>

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
        }

        static void TearDownTestCase()
        {
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
        }

        void TearDown() override
        {
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
}
