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
#include <SceneAPI/SceneData/GraphData/RootBoneData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/TransformData.h>

#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Pipeline/RCExt/Actor/ActorBuilder.h>
#include <EMotionFX/Pipeline/RCExt/ExportContexts.h>
#include <EMotionFX/Pipeline/SceneAPIExt/Groups/ActorGroup.h>
#include <Integration/System/SystemCommon.h>

#include <Tests/Matchers.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <AzTest/Utils.h>

namespace EMotionFX
{
    // This fixture is responsible for creating the scene description used by
    // the actor builder pipeline tests
    using ActorBuilderPipelineFixtureBase = InitSceneAPIFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        EMotionFX::Integration::SystemComponent,
        EMotionFX::Pipeline::ActorBuilder
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
        }

        AZ::SceneAPI::Events::ProcessingResult Process(EMotionFX::Pipeline::Group::ActorGroup& actorGroup)
        {
            AZ::SceneAPI::Events::ProcessingResultCombiner result;
            AZStd::string workingDir = this->GetAssetFolder();
            AZStd::vector<AZStd::string> empty;
            EMotionFX::Pipeline::ActorBuilderContext actorBuilderContext(*m_scene, workingDir, actorGroup, m_actor.get(), empty, AZ::RC::Phase::Construction);
            result += AZ::SceneAPI::Events::Process(actorBuilderContext);
            result += AZ::SceneAPI::Events::Process<EMotionFX::Pipeline::ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Filling);
            result += AZ::SceneAPI::Events::Process<EMotionFX::Pipeline::ActorBuilderContext>(actorBuilderContext, AZ::RC::Phase::Finalizing);
            return result.GetResult();
        }

        void TearDown() override
        {
            m_actor.reset();

            delete m_scene;
            m_scene = nullptr;

            ActorBuilderPipelineFixtureBase::TearDown();
        }

        void ProcessScene()
        {
            // Set up the actor group, which controls which parts of the scene graph
            // are used to generate the actor
            EMotionFX::Pipeline::Group::ActorGroup actorGroup;
            actorGroup.SetName("testActor");
            actorGroup.SetSelectedRootBone("root_joint");

            const AZ::SceneAPI::Events::ProcessingResult result = Process(actorGroup);
            ASSERT_EQ(result, AZ::SceneAPI::Events::ProcessingResult::Success) << "Failed to build actor";
        }

        AZStd::unique_ptr<Actor> m_actor;
        AZ::SceneAPI::Containers::Scene* m_scene = nullptr;
    };

    TEST_F(ActorBuilderPipelineFixture, ActorBuilder_Basic_Three_Joint)
    {
        // Set up a scene graph like this for testing
        // root_joint
        //   |____joint_1
        //         |____joint_2

        using SceneGraph = AZ::SceneAPI::Containers::SceneGraph;
        using namespace AZ::SceneData::GraphData;
        SceneGraph& graph = m_scene->GetGraph();

        AZStd::shared_ptr<BoneData> boneData = AZStd::make_shared<BoneData>();
        const SceneGraph::NodeIndex rootJointIndex = graph.AddChild(graph.GetRoot(), "root_joint", boneData);
        const SceneGraph::NodeIndex joint1Index = graph.AddChild(rootJointIndex, "joint_1", boneData);
        graph.AddChild(joint1Index, "joint_2", boneData);

        ProcessScene();

        ASSERT_EQ(m_actor->GetNumNodes(), 3);
        const Node* rootJoint = m_actor->GetSkeleton()->FindNodeByName("root_joint");
        const Node* joint1 = m_actor->GetSkeleton()->FindNodeByName("joint_1");
        const Node* joint2 = m_actor->GetSkeleton()->FindNodeByName("joint_2");
        EXPECT_TRUE(rootJoint);
        EXPECT_TRUE(rootJoint->GetIsRootNode());
        EXPECT_TRUE(joint1);
        EXPECT_TRUE(joint2);
        EXPECT_EQ(joint1->GetParentNode(), rootJoint);
        EXPECT_EQ(joint2->GetParentNode(), joint1);
    }

    TEST_F(ActorBuilderPipelineFixture, ActorBuilder_Basic_Mesh)
    {
        // Set up a scene graph like this for testing
        // root_joint
        //   |____joint_1
        //         |____mesh_1

        using SceneGraph = AZ::SceneAPI::Containers::SceneGraph;
        using namespace AZ::SceneData::GraphData;
        SceneGraph& graph = m_scene->GetGraph();

        AZStd::shared_ptr<BoneData> boneData = AZStd::make_shared<BoneData>();
        const SceneGraph::NodeIndex rootJointIndex = graph.AddChild(graph.GetRoot(), "root_joint", boneData);
        const SceneGraph::NodeIndex joint1Index = graph.AddChild(rootJointIndex, "joint_1", boneData);

        AZStd::shared_ptr<MeshData> meshData = AZStd::make_shared<MeshData>();
        graph.AddChild(joint1Index, "mesh_1", meshData);

        ProcessScene();

        // NOTE: End point mesh node should be skipped in the emfx skeleton structure. 
        ASSERT_EQ(m_actor->GetNumNodes(), 2);
        const Node* rootJoint = m_actor->GetSkeleton()->FindNodeByName("root_joint");
        const Node* joint1 = m_actor->GetSkeleton()->FindNodeByName("joint_1");
        const Node* mesh1 = m_actor->GetSkeleton()->FindNodeByName("mesh_1");
        EXPECT_TRUE(rootJoint);
        EXPECT_TRUE(joint1);
        EXPECT_TRUE(!mesh1);
    }

    TEST_F(ActorBuilderPipelineFixture, ActorBuilder_Basic_Mesh_Chained)
    {
        // Set up a scene graph like this for testing
        // root_joint
        //   |____joint_1
        //         |____mesh_1
        //                |____joint_2

        using SceneGraph = AZ::SceneAPI::Containers::SceneGraph;
        using namespace AZ::SceneData::GraphData;
        SceneGraph& graph = m_scene->GetGraph();

        AZStd::shared_ptr<BoneData> boneData = AZStd::make_shared<BoneData>();
        const SceneGraph::NodeIndex rootJointIndex = graph.AddChild(graph.GetRoot(), "root_joint", boneData);
        const SceneGraph::NodeIndex joint1Index = graph.AddChild(rootJointIndex, "joint_1", boneData);

        AZStd::shared_ptr<MeshData> meshData = AZStd::make_shared<MeshData>();
        SceneGraph::NodeIndex meshIndex = graph.AddChild(joint1Index, "mesh_1", meshData);
        graph.AddChild(meshIndex, "joint_2", boneData);

        ProcessScene();

        // NOTE: Mesh node that's part of the chain should NOT be skipped in the emfx skeleton structure. 
        ASSERT_EQ(m_actor->GetNumNodes(), 4);
        const Node* rootJoint = m_actor->GetSkeleton()->FindNodeByName("root_joint");
        const Node* joint1 = m_actor->GetSkeleton()->FindNodeByName("joint_1");
        const Node* mesh1 = m_actor->GetSkeleton()->FindNodeByName("mesh_1");
        const Node* joint2 = m_actor->GetSkeleton()->FindNodeByName("joint_2");
        EXPECT_TRUE(rootJoint);
        EXPECT_TRUE(joint1);
        EXPECT_TRUE(mesh1);
        EXPECT_TRUE(joint2);
        EXPECT_EQ(mesh1->GetParentNode(), joint1);
        EXPECT_EQ(joint2->GetParentNode(), mesh1);
    }

    class ActorBuilderPipelineTransformTestFixture : public ActorBuilderPipelineFixture
        , public ::testing::WithParamInterface<AZ::Matrix3x4>
    {
    };

    static const AZ::Matrix3x4 Matrix3x4s[] = {
        AZ::Matrix3x4::CreateIdentity(),
        AZ::Matrix3x4::CreateRotationX(-0.6f),
        AZ::Matrix3x4::CreateFromQuaternion(AZ::Quaternion(0.24f, -0.08f, -0.48f, 0.84f)),
        AZ::Matrix3x4::CreateTranslation(AZ::Vector3(7.9f, 2.4f, -4.6f)),
        AZ::Matrix3x4::CreateFromQuaternionAndTranslation(AZ::Quaternion(0.12f, -0.24f, -0.72f, 0.64f), AZ::Vector3(2.3f, -5.2f, 0.7f))
    };

    TEST_P(ActorBuilderPipelineTransformTestFixture, ActorBuilder_Transforms)
    {
        // Set up a scene graph like this for testing
        // root_joint
        //     |____TransformData
        using SceneGraph = AZ::SceneAPI::Containers::SceneGraph;
        using RootBoneData = AZ::SceneData::GraphData::RootBoneData;
        using GraphTransformData = AZ::SceneData::GraphData::TransformData;
        SceneGraph& graph = m_scene->GetGraph();

        const AZ::Matrix3x4 globalTransform = GetParam();

        AZStd::shared_ptr<RootBoneData> rootBoneData = AZStd::make_shared<RootBoneData>();
        const SceneGraph::NodeIndex rootJointIndex = graph.AddChild(graph.GetRoot(), "root_joint", rootBoneData);
        AZStd::shared_ptr<GraphTransformData> transformData = AZStd::make_shared<GraphTransformData>();
        transformData->SetMatrix(globalTransform);
        const SceneGraph::NodeIndex transformIndex = graph.AddChild(rootJointIndex, "transform", transformData);
        graph.MakeEndPoint(transformIndex);

        ProcessScene();

        ASSERT_EQ(m_actor->GetNumNodes(), 1);
        const EMotionFX::Pose* pose = m_actor->GetBindPose();
        AZ::Transform emfxLocal = pose->GetLocalSpaceTransform(0).ToAZTransform();
        AZ::Transform builderLocal = AZ::Transform::CreateFromMatrix3x4(globalTransform);
        EXPECT_TRUE(emfxLocal.IsClose(builderLocal));
    }

    INSTANTIATE_TEST_CASE_P(ActorBuilder_Transforms, ActorBuilderPipelineTransformTestFixture, ::testing::ValuesIn(EMotionFX::Matrix3x4s));
} // namespace EMotionFX
