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
#include <AzCore/std/string/conversions.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyManagerComponent.h>

#include <SceneAPI/SceneCore/Mocks/Containers/MockScene.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>

#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphSetupInstance.h>
#include <EMotionFX/Source/MorphTarget.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Pipeline/RCExt/Actor/ActorBuilder.h>
#include <EMotionFX/Pipeline/RCExt/Actor/MorphTargetExporter.h>
#include <EMotionFX/Pipeline/RCExt/ExportContexts.h>
#include <EMotionFX/Pipeline/SceneAPIExt/Groups/ActorGroup.h>
#include <EMotionFX/Pipeline/SceneAPIExt/Rules/MorphTargetRule.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    // This fixture is responsible for creating the scene description used by
    // the morph target pipeline tests

    using MorphTargetPipelineFixtureBase = InitSceneAPIFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        EMotionFX::Integration::SystemComponent,
        EMotionFX::Pipeline::ActorBuilder,
        EMotionFX::Pipeline::MorphTargetExporter
    >;

    class MorphTargetPipelineFixture
        : public MorphTargetPipelineFixtureBase
    {
    public:
        void SetUp() override
        {
            MorphTargetPipelineFixtureBase::SetUp();

            m_actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(0);

            // Set up the scene graph
            m_scene = new AZ::SceneAPI::Containers::MockScene("MockScene");
            m_scene->SetOriginalSceneOrientation(AZ::SceneAPI::Containers::Scene::SceneOrientation::ZUp);

            AZ::SceneAPI::Containers::SceneGraph& graph = m_scene->GetGraph();

            AZStd::shared_ptr<AZ::SceneData::GraphData::BoneData> boneData = AZStd::make_shared<AZ::SceneData::GraphData::BoneData>();
            graph.AddChild(graph.GetRoot(), "testRootBone", boneData);

            // Set up our base shape
            AZStd::shared_ptr<AZ::SceneData::GraphData::MeshData> meshData = AZStd::make_shared<AZ::SceneData::GraphData::MeshData>();
            AZStd::vector<AZ::Vector3> unmorphedVerticies
            {
                AZ::Vector3(0.0f, 0.0f, 0.0f),
                AZ::Vector3(1.0f, 0.0f, 0.0f),
                AZ::Vector3(0.0f, 1.0f, 0.0f)
            };
            for (const AZ::Vector3& vertex : unmorphedVerticies)
            {
                meshData->AddPosition(vertex);
            }
            meshData->AddNormal(AZ::Vector3(0.0f, 0.0f, 1.0f));
            meshData->AddNormal(AZ::Vector3(0.0f, 0.0f, 1.0f));
            meshData->AddNormal(AZ::Vector3(0.0f, 0.0f, 1.0f));
            meshData->SetVertexIndexToControlPointIndexMap(0, 0);
            meshData->SetVertexIndexToControlPointIndexMap(1, 1);
            meshData->SetVertexIndexToControlPointIndexMap(2, 2);
            meshData->AddFace(0, 1, 2);
            AZ::SceneAPI::Containers::SceneGraph::NodeIndex meshNodeIndex = graph.AddChild(graph.GetRoot(), "testMesh", meshData);

            // Set up the morph targets
            AZStd::vector<AZStd::vector<AZ::Vector3>> morphedVertices
            {{
                {
                    // Morph target 1
                    AZ::Vector3(0.0f, 0.0f, 0.0f),
                    AZ::Vector3(1.0f, 0.0f, 1.0f), // this one is different
                    AZ::Vector3(0.0f, 1.0f, 0.0f)
                },
                {
                    // Morph target 2
                    AZ::Vector3(0.0f, 0.0f, 0.0f),
                    AZ::Vector3(1.0f, 0.0f, 0.0f),
                    AZ::Vector3(0.0f, 1.0f, 1.0f)  // this one is different
                },
            }};
            const size_t morphTargetCount = morphedVertices.size();
            for (size_t morphIndex = 0; morphIndex < morphTargetCount; ++morphIndex)
            {
                AZStd::shared_ptr<AZ::SceneData::GraphData::BlendShapeData> blendShapeData = AZStd::make_shared<AZ::SceneData::GraphData::BlendShapeData>();
                const AZStd::vector<AZ::Vector3>& verticesForThisMorph = morphedVertices.at(morphIndex);
                const uint vertexCount = static_cast<uint>(verticesForThisMorph.size());
                for (uint vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                {
                    blendShapeData->AddPosition(verticesForThisMorph.at(vertexIndex));
                    blendShapeData->AddNormal(AZ::Vector3::CreateAxisZ());
                    blendShapeData->SetVertexIndexToControlPointIndexMap(vertexIndex, vertexIndex);
                }
                blendShapeData->AddFace({{0, 1, 2}});
                AZStd::string morphTargetName("testMorphTarget");
                morphTargetName += AZStd::to_string(static_cast<int>(morphIndex));
                graph.AddChild(meshNodeIndex, morphTargetName.c_str(), blendShapeData);
            }
        }

        AZ::SceneAPI::Events::ProcessingResult Process(EMotionFX::Pipeline::Group::ActorGroup& actorGroup)
        {
            AZ::SceneAPI::Events::ProcessingResultCombiner result;
            AZStd::vector<AZStd::string> materialReferences;
            EMotionFX::Pipeline::ActorBuilderContext actorBuilderContext(*m_scene, "tmp", actorGroup, m_actor.get(), materialReferences, AZ::RC::Phase::Construction);
            result += AZ::SceneAPI::Events::Process(actorBuilderContext);
            result += AZ::SceneAPI::Events::Process<EMotionFX::Pipeline::ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Filling);
            result += AZ::SceneAPI::Events::Process<EMotionFX::Pipeline::ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Finalizing);
            return result.GetResult();
        }

        void TearDown() override
        {
            m_actor.reset();
            delete m_scene;

            MorphTargetPipelineFixtureBase::TearDown();
        }

        EMotionFX::Mesh* GetMesh(const EMotionFX::Actor* actor)
        {
            Skeleton* skeleton = actor->GetSkeleton();
            EMotionFX::Mesh* mesh = nullptr;

            const size_t numNodes = skeleton->GetNumNodes();
            for (size_t nodeNum = 0; nodeNum < numNodes; ++nodeNum)
            {
                if (mesh)
                {
                    // We should only have one node that has a mesh
                    EXPECT_FALSE(actor->GetMesh(0, nodeNum)) << "More than one mesh found on built actor";
                }
                else
                {
                    mesh = actor->GetMesh(0, nodeNum);
                    AZ_Printf("EMotionFX", "%s node name", skeleton->GetNode(nodeNum)->GetName());
                }
            }
            return mesh;
        }

        AZStd::unique_ptr<Actor> m_actor;
        AZ::SceneAPI::Containers::Scene* m_scene;
    };

    class MorphTargetCreationTestFixture : public MorphTargetPipelineFixture,
                                           public ::testing::WithParamInterface<std::vector<std::string>>
    {
    };

    TEST_P(MorphTargetCreationTestFixture, TestMorphTargetCreation)
    {
        const std::vector<std::string>& selectedMorphTargets = GetParam();

        // Set up the actor group, which controls which parts of the scene graph
        // are used to generate the actor
        EMotionFX::Pipeline::Group::ActorGroup actorGroup;
        actorGroup.SetSelectedRootBone("testRootBone");

        // TODO: replace the test with atom mesh.
        // actorGroup.GetSceneNodeSelectionList().AddSelectedNode("testMesh");
        // actorGroup.GetBaseNodeSelectionList().AddSelectedNode("testMesh");

        AZStd::shared_ptr<EMotionFX::Pipeline::Rule::MorphTargetRule> morphTargetRule = AZStd::make_shared<EMotionFX::Pipeline::Rule::MorphTargetRule>();
        for (const std::string& selectedMorphTarget : selectedMorphTargets)
        {
            morphTargetRule->GetSceneNodeSelectionList().AddSelectedNode(("testMesh." + selectedMorphTarget).c_str());
        }
        actorGroup.GetRuleContainer().AddRule(morphTargetRule);

        const AZ::SceneAPI::Events::ProcessingResult result = Process(actorGroup);
        ASSERT_EQ(result, AZ::SceneAPI::Events::ProcessingResult::Success) << "Failed to build actor";

        const MorphSetup* morphSetup = m_actor->GetMorphSetup(0);
        if (selectedMorphTargets.empty())
        {
            ASSERT_FALSE(morphSetup) << "A morph setup was created when the blend shape rule specified no nodes";
            // That's all we can verify for the case where no morph targets
            // were selected for export
            return;
        }

        ASSERT_TRUE(morphSetup) << "No morph setup was created";
        const size_t expectedNumMorphTargets = selectedMorphTargets.size();
        ASSERT_EQ(morphSetup->GetNumMorphTargets(), expectedNumMorphTargets) << "Morph setup should contain " << expectedNumMorphTargets << " morph target(s)";

        EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance> actorInstance = EMotionFX::Integration::EMotionFXPtr<EMotionFX::ActorInstance>::MakeFromNew(EMotionFX::ActorInstance::Create(m_actor.get()));

        const Mesh* mesh = GetMesh(m_actor.get());

        // TODO: replace the test with atom mesh.
        if (!mesh)
        {
            return;
        }

        const size_t numMorphTargets = morphSetup->GetNumMorphTargets();
        for (size_t morphTargetIndex = 0; morphTargetIndex < numMorphTargets; ++morphTargetIndex)
        {
            const MorphTarget* morphTarget = morphSetup->GetMorphTarget(morphTargetIndex);
            EXPECT_STREQ(morphTarget->GetName(), selectedMorphTargets[morphTargetIndex].c_str()) << "Morph target's name is incorrect";

            const AZ::SceneAPI::Containers::SceneGraph& graph = m_scene->GetGraph();

            // Verify that the unmorphed vertices are what we expect
            const AZ::Vector3* const positions = static_cast<AZ::Vector3*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            AZStd::vector<AZ::Vector3> gotUnmorphedVertices;
            uint32 numVertices = mesh->GetNumVertices();
            for (uint32 vertexNum = 0; vertexNum < numVertices; ++vertexNum)
            {
                gotUnmorphedVertices.emplace_back(
                    positions[vertexNum].GetX(),
                    positions[vertexNum].GetY(),
                    positions[vertexNum].GetZ()
                );
            }

            // Get the unmorphed vertices from the scene data
            const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMeshData> meshData = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshData*>(graph.GetNodeContent(graph.Find("testMesh")));
            AZStd::vector<AZ::Vector3> expectedUnmorphedVertices;
            numVertices = meshData->GetVertexCount();
            for (uint vertexNum = 0; vertexNum < numVertices; ++vertexNum)
            {
                expectedUnmorphedVertices.emplace_back(meshData->GetPosition(meshData->GetControlPointIndex(vertexNum)));
            }
            EXPECT_EQ(gotUnmorphedVertices, expectedUnmorphedVertices);

            // Now apply the morph, and verify the morphed vertices against
            // what we expect
            actorInstance->GetMorphSetupInstance()->GetMorphTarget(morphTargetIndex)->SetManualMode(true);
            actorInstance->GetMorphSetupInstance()->GetMorphTarget(morphTargetIndex)->SetWeight(1.0f);
            actorInstance->UpdateTransformations(0.0f, true);
            actorInstance->UpdateMeshDeformers(0.0f);

            const AZ::Vector3* const morphedPositions = static_cast<AZ::Vector3*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            AZStd::vector<AZ::Vector3> gotMorphedVertices;
            numVertices = mesh->GetNumVertices();
            for (uint32 vertexNum = 0; vertexNum < numVertices; ++vertexNum)
            {
                gotMorphedVertices.emplace_back(
                    morphedPositions[vertexNum].GetX(),
                    morphedPositions[vertexNum].GetY(),
                    morphedPositions[vertexNum].GetZ()
                );
            }

            // Get the morphed vertices from the scene data
            AZStd::string morphTargetSceneNodeName("testMesh.");
            morphTargetSceneNodeName += selectedMorphTargets[morphTargetIndex].c_str();
            const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IBlendShapeData> morphTargetData = azrtti_cast<const AZ::SceneAPI::DataTypes::IBlendShapeData*>(graph.GetNodeContent(graph.Find(morphTargetSceneNodeName)));
            AZStd::vector<AZ::Vector3> expectedMorphedVertices;
            numVertices = morphTargetData->GetVertexCount();
            for (uint vertexNum = 0; vertexNum < numVertices; ++vertexNum)
            {
                expectedMorphedVertices.emplace_back(morphTargetData->GetPosition(morphTargetData->GetControlPointIndex(vertexNum)));
            }
            EXPECT_EQ(gotMorphedVertices, expectedMorphedVertices);

            // Reset the morph target weight so that the next iteration compares against the unmorphed mesh
            actorInstance->GetMorphSetupInstance()->GetMorphTarget(morphTargetIndex)->SetWeight(0.0f);
            actorInstance->UpdateTransformations(0.0f, true);
            actorInstance->UpdateMeshDeformers(0.0f);
        }
    }

    // Note that these values are instantiated before the SystemAllocator is
    // created, so we can't use AZStd::vector
    INSTANTIATE_TEST_CASE_P(TestMorphTargetCreation, MorphTargetCreationTestFixture,
        ::testing::Values(
            std::vector<std::string> {},
            std::vector<std::string> {"testMorphTarget0"},
            std::vector<std::string> {"testMorphTarget1"},
            std::vector<std::string> {"testMorphTarget0", "testMorphTarget1"}
        )
    );
}
