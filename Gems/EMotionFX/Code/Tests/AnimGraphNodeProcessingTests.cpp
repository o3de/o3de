/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/AnimGraphFixture.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/AnimGraphPosePool.h>

namespace EMotionFX
{
    struct NodeProcessingTestParam
    {
        size_t m_motionNodeCount;
    };

    class AnimGraphNodeProcessingTestFixture : public AnimGraphFixture
        , public ::testing::WithParamInterface<NodeProcessingTestParam>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            /*
                +----------+
                | Motion 1 +-----------+
                +----------+           |
                                       |
                +----------+           >+---------+               +-------+
                | Motion 2 +----------->| Blend N +-------------->+ Final |
                +----------+     ------>|         |               +-------+
                                 |     >+---------+
                +----------+     |     |
                | Motion N +-----+     |
                +----------+           |
                                       |
                +-------------+        |
                | Const Float +--------+
                +-------------+
            */
            const NodeProcessingTestParam& param = GetParam();

            m_blendNNode = aznew BlendTreeBlendNNode();
            m_blendTree->AddChildNode(m_blendNNode);
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            finalNode->AddConnection(m_blendNNode, BlendTreeBlendNNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            ASSERT_TRUE(param.m_motionNodeCount <= 10) << "The blend N node only has 10 pose inputs.";
            for (size_t i = 0; i < param.m_motionNodeCount; ++i)
            {
                AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
                motionNode->SetName(AZStd::string::format("MotionNode%zu", i).c_str());
                m_blendTree->AddChildNode(motionNode);
                m_blendNNode->AddConnection(motionNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, aznumeric_caster(i));
                m_motionNodes.push_back(motionNode);
            }
            m_blendNNode->UpdateParamWeights();
            m_blendNNode->SetParamWeightsEquallyDistributed(0.0f, 1.0f);
            m_blendNNode->SetSyncMode(AnimGraphObject::SYNCMODE_CLIPBASED);

            m_floatNode = aznew BlendTreeFloatConstantNode();
            m_blendTree->AddChildNode(m_floatNode);
            m_blendNNode->AddConnection(m_floatNode, BlendTreeFloatConstantNode::OUTPUTPORT_RESULT, BlendTreeBlendNNode::INPUTPORT_WEIGHT);

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);

            for (size_t i = 0; i < m_motionNodes.size(); ++i)
            {
                const AZStd::string motionId = AZStd::string::format("testSkeletalMotion%zu", i);

                Motion* motion = aznew Motion(motionId.c_str());
                motion->SetMotionData(aznew NonUniformMotionData());
                motion->GetMotionData()->SetDuration(i + 1.0f);

                MotionSet::MotionEntry * motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
                m_motionSet->AddMotionEntry(motionEntry);

                m_motionNodes[i]->AddMotionId(motionId.c_str());
                m_motionNodes[i]->RecursiveOnChangeMotionSet(m_animGraphInstance, m_motionSet); // Trigger create motion instance.
                m_motionNodes[i]->PickNewActiveMotion(m_animGraphInstance);
            }
        }

    public:
        std::vector<AnimGraphMotionNode*> m_motionNodes;
        BlendTree* m_blendTree = nullptr;
        BlendTreeFloatConstantNode* m_floatNode = nullptr;
        BlendTreeBlendNNode* m_blendNNode = nullptr;
    };

    TEST_P(AnimGraphNodeProcessingTestFixture, NodeProcessingWithBlendNTests)
    {
        // Calling update first to make sure unique data is created.
        GetEMotionFX().Update(0.0f);

        const float deltaTime = 0.1f;
        // Next, we will be mimicing the anim graph instance update in actor instance update function. We just call each steps individually so we can collect the data.
        for (int i = 0; i <= 10; ++i)
        {
            m_floatNode->SetValue(0.1f * i);

            // Check the pose ref data on the anim graph instance, make sure they are zeros before calling update.
            const size_t numUniqueData = m_animGraphInstance->GetNumUniqueObjectDatas();
            for (size_t i2 = 0; i2 < numUniqueData; ++i2)
            {
                const AnimGraphNodeData* uniqueData = static_cast<AnimGraphNodeData*>(m_animGraphInstance->GetUniqueObjectData(i2));
                EXPECT_EQ(uniqueData->GetPoseRefCount(), 0) << "Pose ref count data should be empty";
            }

            // Call update on the instance, make sure the pose ref count is increased.
            m_animGraphInstance->Update(deltaTime);
            for (size_t i2 = 0; i2 < numUniqueData; ++i2)
            {
                const AnimGraphNodeData* uniqueData = static_cast<AnimGraphNodeData*>(m_animGraphInstance->GetUniqueObjectData(i2));
                EXPECT_EQ(uniqueData->GetPoseRefCount(), 1) << "Pose ref count data should be 1";
            }

            // Call output function.
            m_animGraphInstance->Output(m_actorInstance->GetTransformData()->GetCurrentPose());

            // Collect active nodes
            AZStd::vector<AnimGraphNode*> activeNodes;
            m_animGraphInstance->CollectActiveAnimGraphNodes(&activeNodes, AZ::Uuid::CreateNull());
            AZStd::unordered_set<AZStd::string> activeNodeNames;
            for (const AnimGraphNode* node : activeNodes)
            {
                activeNodeNames.emplace(node->GetNameString());
            }

            // See which motion node is activated in the blendNNode.
            float blendWeight;
            AnimGraphNode* nodeA;
            AnimGraphNode* nodeB;
            uint32 poseIndexA;
            uint32 poseIndexB;
            m_blendNNode->FindBlendNodes(m_animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

            // Make sure nodeA and nodeB are active.
            EXPECT_TRUE(activeNodeNames.find(nodeA->GetNameString()) != activeNodeNames.end()) << "%s should be activated", nodeA->GetName();
            if (nodeA != nodeB)
            {
                EXPECT_TRUE(activeNodeNames.find(nodeB->GetNameString()) != activeNodeNames.end()) << "%s should be activated", nodeB->GetName();

                // There will be 5 node always be activated, rootStateMachine, blendTree, floatConst, blendN, finalNode
                // If nodeA is not nodeB, then there will be two more motion node active.
                EXPECT_EQ(activeNodes.size(), 7);
            }
            else
            {
                EXPECT_EQ(activeNodes.size(), 6);
            }

            // Make sure we aren't blowing up the pose pool
            const AnimGraphPosePool& posePool = GetEMotionFX().GetThreadData(m_actorInstance->GetThreadIndex())->GetPosePool();
            EXPECT_EQ(posePool.GetNumUsedPoses(), 0) << "Pose pool should be freed after output called.";
            EXPECT_TRUE(posePool.GetNumMaxUsedPoses() <= 3) << "At most we are using 3 pose at the same time (two motion and a blendN).";
        }
    }

    std::vector<NodeProcessingTestParam> AnimGraphNodeProcessingTestTestData
    {
        {
            2
        },
        {
            5
        },
        {
            10
        }
    };

    INSTANTIATE_TEST_CASE_P(AnimGraphNodeProcessingTests,
        AnimGraphNodeProcessingTestFixture,
        ::testing::ValuesIn(AnimGraphNodeProcessingTestTestData));
} // namespace EMotionFX
