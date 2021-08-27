/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include "SystemComponentFixture.h"

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeMorphTargetNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/Source/MorphTargetStandard.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Pose.h>
#include <MCore/Source/ReflectionSerializer.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <Tests/Printers.h>
#include <Tests/Matchers.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    class MorphTargetRuntimeFixture
        : public SystemComponentFixture
    {
    public:
        void ScaleMesh(Mesh* mesh)
        {
            const uint32 vertexCount = mesh->GetNumVertices();
            AZ::Vector3* positions = static_cast<AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            for (uint32 vertexNum = 0; vertexNum < vertexCount; ++vertexNum)
            {
                positions[vertexNum] *= m_scaleFactor;
            }
        }

        void AddParam(const char* name, const AZ::TypeId& type, const AZStd::string& defaultValue)
        {
            EMotionFX::Parameter* parameter = EMotionFX::ParameterFactory::Create(type);
            parameter->SetName(name);
            MCore::ReflectionSerializer::DeserializeIntoMember(parameter, "defaultValue", defaultValue);
            m_animGraph->AddParameter(parameter);
        }

        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            m_actor = ActorFactory::CreateAndInit<PlaneActor>("testActor");

            m_morphSetup = MorphSetup::Create();
            m_actor->SetMorphSetup(0, m_morphSetup);

            AZStd::unique_ptr<Actor> morphActor = m_actor->Clone();
            Mesh* morphMesh = morphActor->GetMesh(0, 0);
            ScaleMesh(morphMesh);
            MorphTargetStandard* morphTarget = MorphTargetStandard::Create(
                    /*captureTransforms=*/ false,
                    m_actor.get(),
                    morphActor.get(),
                    "morphTarget"
                    );
            m_morphSetup->AddMorphTarget(morphTarget);

            // Without this call, the bind pose does not know about newly added
            // morph target (m_morphWeights.size() == 0)
            m_actor->ResizeTransformData();
            m_actor->PostCreateInit(/*makeGeomLodsCompatibleWithSkeletalLODs=*/false, /*convertUnitType=*/false);

            m_animGraph = AZStd::make_unique<AnimGraph>();

            AddParam("FloatParam", azrtti_typeid<EMotionFX::FloatSliderParameter>(), "0.0");

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();

            BlendTreeMorphTargetNode* morphTargetNode = aznew BlendTreeMorphTargetNode();
            morphTargetNode->SetMorphTargetNames({"morphTarget"});

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            BlendTree* blendTree = aznew BlendTree();
            blendTree->SetName("testBlendTree");
            blendTree->AddChildNode(parameterNode);
            blendTree->AddChildNode(morphTargetNode);
            blendTree->AddChildNode(finalNode);
            blendTree->SetFinalNodeId(finalNode->GetId());

            m_stateMachine = aznew AnimGraphStateMachine();
            m_stateMachine->SetName("rootStateMachine");
            m_animGraph->SetRootStateMachine(m_stateMachine);
            m_stateMachine->AddChildNode(blendTree);
            m_stateMachine->SetEntryState(blendTree);

            m_stateMachine->InitAfterLoading(m_animGraph.get());

            // Create the connections once the port indices are known. The
            // parameter node's ports are not known until after
            // InitAfterLoading() is called
            morphTargetNode->AddConnection(
                parameterNode,
                aznumeric_caster(parameterNode->FindOutputPortIndex("FloatParam")),
                BlendTreeMorphTargetNode::PORTID_INPUT_WEIGHT
            );
            finalNode->AddConnection(
                morphTargetNode,
                BlendTreeMorphTargetNode::PORTID_OUTPUT_POSE,
                BlendTreeFinalNode::PORTID_INPUT_POSE
            );

            m_motionSet = AZStd::make_unique<MotionSet>();
            m_motionSet->SetName("testMotionSet");

            m_actorInstance = Integration::EMotionFXPtr<ActorInstance>::MakeFromNew(ActorInstance::Create(m_actor.get()));

            m_animGraphInstance = AnimGraphInstance::Create(m_animGraph.get(), m_actorInstance.get(), m_motionSet.get());

            m_actorInstance->SetAnimGraphInstance(m_animGraphInstance);
        }

        void TearDown() override
        {
            m_actor.reset();
            m_actorInstance.reset();
            m_motionSet.reset();
            m_animGraph.reset();

            SystemComponentFixture::TearDown();
        }

        // The members that are EMotionFXPtrs are the ones that are owned by
        // the test fixture. The others are created by the fixture but owned by
        // the EMotionFX runtime.
        AZStd::unique_ptr<Actor> m_actor;
        MorphSetup* m_morphSetup = nullptr;
        AZStd::unique_ptr<AnimGraph> m_animGraph;
        AnimGraphStateMachine* m_stateMachine = nullptr;
        AZStd::unique_ptr<MotionSet> m_motionSet;
        Integration::EMotionFXPtr<ActorInstance> m_actorInstance;
        AnimGraphInstance* m_animGraphInstance = nullptr;
        const float m_scaleFactor = 10.0f;
    };

    TEST_F(MorphTargetRuntimeFixture, DISABLED_TestMorphTargetMeshRuntime)
    {
        const float fps = 30.0f;
        const float updateInterval = 1.0f / fps;

        const Mesh* mesh = m_actor->GetMesh(0, 0);
        const uint32 vertexCount = mesh->GetNumOrgVertices();
        const AZ::Vector3* positions = static_cast<AZ::Vector3*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));

        const AZStd::vector<AZ::Vector3> neutralPoints = [vertexCount, &positions](){
            AZStd::vector<AZ::Vector3> p;
            for (uint32 vertexNum = 0; vertexNum < vertexCount; ++vertexNum)
            {
                p.emplace_back(positions[vertexNum]);
            }
            return p;
        }();
        const AZStd::array<float, 4> weights {
            {0.0f, 0.5f, 1.0f, 0.0f}
        };
        AZStd::vector<AZ::Vector3> gotWeightedPoints;
        AZStd::vector<AZ::Vector3> expectedWeightedPoints;
        for (const float weight : weights)
        {
            gotWeightedPoints.clear();
            expectedWeightedPoints.clear();

            static_cast<MCore::AttributeFloat*>(m_animGraphInstance->FindParameter("FloatParam"))->SetValue(weight);
            GetEMotionFX().Update(updateInterval);
            m_actorInstance->UpdateMeshDeformers(updateInterval);

            for (uint32 vertexNum = 0; vertexNum < vertexCount; ++vertexNum)
            {
                gotWeightedPoints.emplace_back(positions[vertexNum]);
            }

            for (const AZ::Vector3& neutralPoint : neutralPoints)
            {
                const AZ::Vector3 delta = (neutralPoint * m_scaleFactor) - neutralPoint;
                expectedWeightedPoints.emplace_back(neutralPoint + delta * weight);
            }
            EXPECT_THAT(gotWeightedPoints, ::testing::Pointwise(::IsClose(), expectedWeightedPoints));
        }
    }
} // namespace EMotionFX
