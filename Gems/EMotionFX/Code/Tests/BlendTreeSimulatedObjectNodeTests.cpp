/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/Pose.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/BlendTreeSimulatedObjectNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <Tests/JackGraphFixture.h>
#include <Tests/Matchers.h>
#include <string>
#include <vector>

namespace EMotionFX
{
    class BlendTreeSimulatedObjectNodeFixture
        : public JackGraphFixture
    {
    public:
        enum
        {
            UpperLeg = 0,
            LowerLeg = 1,
            Foot = 2
        };

        void CreateSimulatedObject(SimulatedObjectSetup* simSetup, const AZStd::string& simObjectName, const AZStd::vector<AZStd::string>& jointNames)
        {
            SimulatedObject* simObject = simSetup->AddSimulatedObject(simObjectName);
            const Skeleton* skeleton = m_actor->GetSkeleton();
            ASSERT_EQ(jointNames.size(), 3);
            for (size_t i= 0; i < 3; ++i)
            {
                AZ::u32 jointIndex = InvalidIndex32;
                const Node* node = skeleton->FindNodeAndIndexByName(jointNames[i].c_str(), jointIndex);
                ASSERT_NE(node, nullptr);
                m_jointIndices[i] = jointIndex;

                SimulatedJoint* simJoint = simObject->AddSimulatedJoint(jointIndex);
                simJoint->SetStiffness(0.0f);
                if (i == UpperLeg)
                {
                    simJoint->SetPinned(true);
                }
            }
        }

        void ConstructGraph() override
        {
            JackGraphFixture::ConstructGraph();

            SimulatedObjectSetup* simSetup = m_actor->GetSimulatedObjectSetup().get();
            ASSERT_NE(simSetup, nullptr);

            CreateSimulatedObject(simSetup, "leftLeg", {"l_upLeg", "l_loLeg", "l_ankle"});
            CreateSimulatedObject(simSetup, "rightLeg", {"r_upLeg", "r_loLeg", "r_ankle"});

            //---------------------------------------------
            // Create a weight parameter.
            m_weightParameter = static_cast<FloatSliderParameter*>(ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>()));
            m_weightParameter->SetName("Weight");
            m_weightParameter->SetDefaultValue(1.0f);
            m_animGraph->AddParameter(m_weightParameter);

            // Create the blend tree.
            BlendTree* blendTree = aznew BlendTree();
            m_animGraph->GetRootStateMachine()->AddChildNode(blendTree);
            m_animGraph->GetRootStateMachine()->SetEntryState(blendTree);

            // Add a final node.
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            blendTree->AddChildNode(finalNode);

            // Add the simulated object node.
            m_simNode = aznew BlendTreeSimulatedObjectNode();
            m_simNode->SetName("SimObjectNode");
            m_simNode->SetSimulatedObjectNames({});
            blendTree->AddChildNode(m_simNode);
            finalNode->AddConnection(m_simNode, BlendTreeSimulatedObjectNode::OUTPUTPORT_POSE, BlendTreeFinalNode::INPUTPORT_POSE);

            // Create the parameter node.
            m_parameterNode = aznew BlendTreeParameterNode();
            blendTree->AddChildNode(m_parameterNode);

            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            blendTree->AddChildNode(bindPoseNode);
            m_simNode->AddConnection(bindPoseNode, AnimGraphBindPoseNode::OUTPUTPORT_RESULT, BlendTreeSimulatedObjectNode::INPUTPORT_POSE);

            // Connect the weight parameter to the weight of the simulated object node.
            m_simNode->AddUnitializedConnection(m_parameterNode, /* Weight parameter port */ 0, BlendTreeSimulatedObjectNode::INPUTPORT_ACTIVE);
        }

        void SetActiveObjects(const AZStd::vector<AZStd::string>& activeObjects)
        {
            m_simNode->SetSimulatedObjectNames(activeObjects);
            m_simNode->InvalidateUniqueData(m_animGraphInstance);
        }

    protected:
        FloatSliderParameter* m_weightParameter = nullptr;
        BlendTreeSimulatedObjectNode* m_simNode = nullptr;
        BlendTreeParameterNode* m_parameterNode = nullptr;
        AZ::u32 m_jointIndices[3] { InvalidIndex32, InvalidIndex32, InvalidIndex32 };
    };

    TEST_F(BlendTreeSimulatedObjectNodeFixture, TransformsCheck)
    {
        SetActiveObjects({"leftLeg"});

        // Get bind pose positions in world space.
        m_actorInstance->GetTransformData()->MakeBindPoseTransformsUnique(); // We do this as otherwise we can't get world transforms.
        const Pose& bindPose = *m_actorInstance->GetTransformData()->GetBindPose();

        // Process 1000 frames at 60 fps.
        const Pose& currentPose = *m_actorInstance->GetTransformData()->GetCurrentPose();
        for (size_t frame = 0; frame < 1000; ++frame)
        {
            GetEMotionFX().Update(1.0f / 60.0f);

            for (size_t joint = 0; joint < 3; ++joint)
            {
                const AZ::Vector3& jointPos = currentPose.GetWorldSpaceTransform(m_jointIndices[joint]).mPosition;
                const AZ::Vector3& jointBindPos = bindPose.GetWorldSpaceTransform(m_jointIndices[joint]).mPosition;
                ASSERT_TRUE((jointPos - jointBindPos).GetLength() <= 0.01f);   // Make sure we didn't move too far from the bind pose.
                ASSERT_TRUE(AZ::IsClose(currentPose.GetWorldSpaceTransform(m_jointIndices[joint]).mRotation.GetLength(), 1.0f, 0.001f));   // Make sure we have a unit quaternion.
            }
        }
    }

    TEST_F(BlendTreeSimulatedObjectNodeFixture, ActiveObjectsZero)
    {
        SetActiveObjects({});

        const BlendTreeSimulatedObjectNode::UniqueData* uniqueData = static_cast<const BlendTreeSimulatedObjectNode::UniqueData*>(m_simNode->FindOrCreateUniqueNodeData(m_animGraphInstance));
        ASSERT_NE(uniqueData, nullptr);
        ASSERT_EQ(uniqueData->m_simulations.size(), 2);

        ASSERT_NE(uniqueData->m_simulations[0], nullptr);
        EXPECT_EQ(uniqueData->m_simulations[0]->m_solver.GetNumSprings(), 2);
        EXPECT_EQ(uniqueData->m_simulations[0]->m_solver.GetNumParticles(), 3);

        ASSERT_NE(uniqueData->m_simulations[1], nullptr);
        EXPECT_EQ(uniqueData->m_simulations[1]->m_solver.GetNumSprings(), 2);
        EXPECT_EQ(uniqueData->m_simulations[1]->m_solver.GetNumParticles(), 3);
    }

    TEST_F(BlendTreeSimulatedObjectNodeFixture, ActiveObjectsOne)
    {
        SetActiveObjects({"leftLeg"});

        const BlendTreeSimulatedObjectNode::UniqueData* uniqueData = static_cast<const BlendTreeSimulatedObjectNode::UniqueData*>(m_simNode->FindOrCreateUniqueNodeData(m_animGraphInstance));
        ASSERT_NE(uniqueData, nullptr);
        ASSERT_EQ(uniqueData->m_simulations.size(), 1);

        ASSERT_NE(uniqueData->m_simulations[0], nullptr);
        EXPECT_EQ(uniqueData->m_simulations[0]->m_solver.GetNumSprings(), 2);
        EXPECT_EQ(uniqueData->m_simulations[0]->m_solver.GetNumParticles(), 3);
    }

    TEST_F(BlendTreeSimulatedObjectNodeFixture, ActiveObjectsTwo)
    {
        SetActiveObjects({"leftLeg", "rightLeg"});

        const BlendTreeSimulatedObjectNode::UniqueData* uniqueData = static_cast<const BlendTreeSimulatedObjectNode::UniqueData*>(m_simNode->FindOrCreateUniqueNodeData(m_animGraphInstance));
        ASSERT_NE(uniqueData, nullptr);
        ASSERT_EQ(uniqueData->m_simulations.size(), 2);

        ASSERT_NE(uniqueData->m_simulations[0], nullptr);
        EXPECT_EQ(uniqueData->m_simulations[0]->m_solver.GetNumSprings(), 2);
        EXPECT_EQ(uniqueData->m_simulations[0]->m_solver.GetNumParticles(), 3);

        ASSERT_NE(uniqueData->m_simulations[1], nullptr);
        EXPECT_EQ(uniqueData->m_simulations[1]->m_solver.GetNumSprings(), 2);
        EXPECT_EQ(uniqueData->m_simulations[1]->m_solver.GetNumParticles(), 3);
    }
} // end namespace EMotionFX
