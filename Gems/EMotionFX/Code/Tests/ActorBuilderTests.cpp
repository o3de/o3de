/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "InitSceneAPIFixture.h"
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>

#include <SceneAPI/SceneCore/Mocks/Containers/MockScene.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Pipeline/RCExt/Actor/ActorBuilder.h>
#include <EMotionFX/Pipeline/RCExt/ExportContexts.h>
#include <EMotionFX/Pipeline/SceneAPIExt/Groups/ActorGroup.h>
#include <Integration/System/SystemCommon.h>

#include <GFxFramework/MaterialIO/Material.h>

#include <Tests/Matchers.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <AzTest/Utils.h>

namespace AZ::GFxFramework
{
    class MockMaterialGroup
        : public MaterialGroup
    {
    public:
        // ActorBuilder::GetMaterialInfoForActorGroup tries to read a source file first, and if that fails
        //  (likely because it doesn't exist), tries to read one product file, followed by another. If they all fail,
        //  none of those files exist, so the function fails. The material read function called by
        //  GetMaterialInfoForActorGroup is mocked to return true after a set number of false returns to mimic the
        //  behavior we would see if each given file is on disk.
        MOCK_METHOD1(ReadMtlFile, bool(const char* filename));
        MOCK_METHOD0(GetMaterialCount, size_t());
    };
} // namespace AZ

namespace EMotionFX
{
    namespace Pipeline
    {
        class MockActorBuilder
            : public ActorBuilder
        {
        public:
            AZ_COMPONENT(MockActorBuilder, "{0C2537B5-6628-4076-BB09-CA1E57E59252}", EMotionFX::Pipeline::ActorBuilder)

            int m_numberReadFailsBeforeSuccess = 0;
            int m_materialCount = 0;

        protected:
            void InstantiateMaterialGroup() override
            {
                auto materialGroup = AZStd::make_shared<AZ::GFxFramework::MockMaterialGroup>();
                EXPECT_CALL(*materialGroup, GetMaterialCount())
                    .WillRepeatedly(testing::Return(m_materialCount));
                EXPECT_CALL(*materialGroup, ReadMtlFile(::testing::_))
                    .WillRepeatedly(testing::Return(true));
                if (m_numberReadFailsBeforeSuccess)
                {
                    EXPECT_CALL(*materialGroup, ReadMtlFile(::testing::_))
                        .Times(m_numberReadFailsBeforeSuccess)
                        .WillRepeatedly(testing::Return(false))
                        .RetiresOnSaturation();
                }
                m_materialGroup = AZStd::move(materialGroup);
            }
        };

        class MockMaterialRule
            : public AZ::SceneAPI::DataTypes::IMaterialRule
        {
        public:
            bool RemoveUnusedMaterials() const override
            {
                return true;
            }

            bool UpdateMaterials() const override
            {
                return true;
            }
        };
    } // namespace Pipeline
    // This fixture is responsible for creating the scene description used by
    // the actor builder pipeline tests
    using ActorBuilderPipelineFixtureBase = InitSceneAPIFixture<
        AZ::MemoryComponent,
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        EMotionFX::Integration::SystemComponent,
        EMotionFX::Pipeline::MockActorBuilder
    >;

    class ActorBuilderPipelineFixture
        : public ActorBuilderPipelineFixtureBase
    {
    public:
        void SetUp() override
        {
            ActorBuilderPipelineFixtureBase::SetUp();

            m_actor = EMotionFX::ActorFactory::CreateAndInit<Actor>("testActor");

            // Set up the scene graph
            m_scene = new AZ::SceneAPI::Containers::MockScene("MockScene");
            m_scene->SetOriginalSceneOrientation(AZ::SceneAPI::Containers::Scene::SceneOrientation::ZUp);
            AZStd::string testSourceFile;
            AzFramework::StringFunc::Path::Join(this->GetAssetFolder().c_str(), "TestFile.fbx", testSourceFile);
            AzFramework::StringFunc::Path::Normalize(testSourceFile);
            m_scene->SetSource(testSourceFile, AZ::Uuid::CreateRandom());

            AZ::SceneAPI::Containers::SceneGraph& graph = m_scene->GetGraph();

            AZStd::shared_ptr<AZ::SceneData::GraphData::BoneData> boneData = AZStd::make_shared<AZ::SceneData::GraphData::BoneData>();
            graph.AddChild(graph.GetRoot(), "testRootBone", boneData);

            // Set up our base shape
            AZStd::shared_ptr<AZ::SceneData::GraphData::MeshData> meshData = AZStd::make_shared<AZ::SceneData::GraphData::MeshData>();
            AZStd::vector<AZ::Vector3> unmorphedVerticies
            {
                AZ::Vector3(0, 0, 0),
                AZ::Vector3(1, 0, 0),
                AZ::Vector3(0, 1, 0)
            };
            for (const AZ::Vector3& vertex : unmorphedVerticies)
            {
                meshData->AddPosition(vertex);
            }
            meshData->AddNormal(AZ::Vector3(0, 0, 1));
            meshData->AddNormal(AZ::Vector3(0, 0, 1));
            meshData->AddNormal(AZ::Vector3(0, 0, 1));
            meshData->SetVertexIndexToControlPointIndexMap(0, 0);
            meshData->SetVertexIndexToControlPointIndexMap(1, 1);
            meshData->SetVertexIndexToControlPointIndexMap(2, 2);
            meshData->AddFace(0, 1, 2);
            AZ::SceneAPI::Containers::SceneGraph::NodeIndex meshNodeIndex = graph.AddChild(graph.GetRoot(), "testMesh", meshData);
        }

        AZ::SceneAPI::Events::ProcessingResult Process(EMotionFX::Pipeline::Group::ActorGroup& actorGroup, AZStd::vector<AZStd::string>& materialReferences)
        {
            AZ::SceneAPI::Events::ProcessingResultCombiner result;
            AZStd::string workingDir = this->GetAssetFolder();
            EMotionFX::Pipeline::ActorBuilderContext actorBuilderContext(*m_scene, workingDir, actorGroup, m_actor.get(), materialReferences, AZ::RC::Phase::Construction);
            result += AZ::SceneAPI::Events::Process(actorBuilderContext);
            result += AZ::SceneAPI::Events::Process<EMotionFX::Pipeline::ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Filling);
            result += AZ::SceneAPI::Events::Process<EMotionFX::Pipeline::ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Finalizing);
            return result.GetResult();
        }

        void TearDown() override
        {
            delete m_scene;
            m_scene = nullptr;

            ActorBuilderPipelineFixtureBase::TearDown();
        }

        void TestSuccessCase(const AZStd::vector<AZStd::string>& expectedMaterialReferences)
        {
            // Set up the actor group, which controls which parts of the scene graph
            // are used to generate the actor
            EMotionFX::Pipeline::Group::ActorGroup actorGroup;
            actorGroup.SetName("testActor");
            actorGroup.SetSelectedRootBone("testRootBone");
            actorGroup.GetSceneNodeSelectionList().AddSelectedNode("testMesh");
            actorGroup.GetBaseNodeSelectionList().AddSelectedNode("testMesh");

            // do something here to make sure there are material rules in the actor?
            if (!expectedMaterialReferences.empty())
            {
                AZStd::shared_ptr<EMotionFX::Pipeline::MockMaterialRule> materialRule = AZStd::make_shared<EMotionFX::Pipeline::MockMaterialRule>();
                actorGroup.GetRuleContainer().AddRule(materialRule);
            }

            AZStd::vector<AZStd::string> materialReferences;

            const AZ::SceneAPI::Events::ProcessingResult result = Process(actorGroup, materialReferences);
            ASSERT_EQ(result, AZ::SceneAPI::Events::ProcessingResult::Success) << "Failed to build actor";

            for (auto& materialReference : materialReferences)
            {
                AzFramework::StringFunc::Path::Normalize(materialReference);
            }
            EXPECT_THAT(
                materialReferences,
                ::testing::Pointwise(StrEq(), expectedMaterialReferences)
            );
        }

        AZStd::unique_ptr<Actor> m_actor;
        AZ::SceneAPI::Containers::Scene* m_scene = nullptr;
    };

    TEST_F(ActorBuilderPipelineFixture, ActorBuilder_MaterialReferences_NoReferences)
    {
        // Set up the actor group, which controls which parts of the scene graph
        // are used to generate the actor
        TestSuccessCase({});
    }

    TEST_F(ActorBuilderPipelineFixture, ActorBuilder_MaterialReferences_OneSourceReference_ExpectAbsolutePath)
    {
        AZStd::string expectedMaterialReference;
        AZ::StringFunc::Path::Join(this->GetAssetFolder().c_str(), "TestFile.mtl", expectedMaterialReference);

        Pipeline::MockActorBuilder* actorBuilderComponent = GetSystemEntity()->FindComponent<Pipeline::MockActorBuilder>();
        actorBuilderComponent->m_numberReadFailsBeforeSuccess = 0;
        actorBuilderComponent->m_materialCount = 1;
        TestSuccessCase({expectedMaterialReference});
    }

    TEST_F(ActorBuilderPipelineFixture, ActorBuilder_MaterialReferences_OneProductReference_ExpectRelativeMaterialPath)
    {
        AZStd::string expectedMaterialReference = "testActor.mtl";
        AZStd::to_lower(expectedMaterialReference.begin(), expectedMaterialReference.end());

        Pipeline::MockActorBuilder* actorBuilderComponent = GetSystemEntity()->FindComponent<Pipeline::MockActorBuilder>();
        actorBuilderComponent->m_numberReadFailsBeforeSuccess = 1;
        actorBuilderComponent->m_materialCount = 1;
        TestSuccessCase({expectedMaterialReference});
    }

    TEST_F(ActorBuilderPipelineFixture, ActorBuilder_MaterialReferences_OneProductReference_ExpectRelativeDccPath)
    {
        AZStd::string expectedMaterialReference = "testActor.dccmtl";
        AZStd::to_lower(expectedMaterialReference.begin(), expectedMaterialReference.end());

        Pipeline::MockActorBuilder* actorBuilderComponent = GetSystemEntity()->FindComponent<Pipeline::MockActorBuilder>();
        actorBuilderComponent->m_numberReadFailsBeforeSuccess = 2;
        actorBuilderComponent->m_materialCount = 1;
        TestSuccessCase({expectedMaterialReference});
    }
} // namespace EMotionFX
