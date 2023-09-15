/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphFixture.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>

namespace EMotionFX
{

    class BlendTreeBlendNNodeTests : public AnimGraphFixture
    {
    public:
        void TearDown() override
        {
            if (m_motionNodes)
            {
                delete m_motionNodes;
            }
            AnimGraphFixture::TearDown();
        }

        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            m_blendNNode = aznew BlendTreeBlendNNode();
            m_blendTree->AddChildNode(m_blendNNode);
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            finalNode->AddConnection(m_blendNNode, BlendTreeBlendNNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            const uint16 motionNodeCount = 3;
            for (uint16 i = 0; i < motionNodeCount; ++i)
            {
                AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
                m_blendTree->AddChildNode(motionNode);
                m_blendNNode->AddConnection(motionNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, i);
                m_motionNodes->push_back(motionNode);
            }
            m_blendNNode->UpdateParamWeights();
            m_blendNNode->SetParamWeightsEquallyDistributed(-1.0f, 1.0f);

            Parameter* parameter = ParameterFactory::Create(azrtti_typeid<FloatSliderParameter>());
            parameter->SetName("parameter_test");
            m_blendTreeAnimGraph->AddParameter(parameter);

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            m_blendTree->AddChildNode(parameterNode);
            m_blendNNode->AddUnitializedConnection(parameterNode, 0, BlendTreeBlendNNode::INPUTPORT_WEIGHT);

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            m_motionNodes = new AZStd::vector<AnimGraphMotionNode*>();
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);

            for (size_t i = 0; i < m_motionNodes->size(); ++i)
            {
                // The motion set keeps track of motions by their name. Each motion
                // within the motion set must have a unique name.
                AZStd::string motionId = AZStd::string::format("testSkeletalMotion%zu", i);

                Motion* motion = aznew Motion(motionId.c_str());
                motion->SetMotionData(aznew NonUniformMotionData());
                motion->GetMotionData()->SetDuration(1.0f);

                MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
                m_motionSet->AddMotionEntry(motionEntry);

                (*m_motionNodes)[i]->AddMotionId(motionId.c_str());
            }
        }

        AZStd::vector<AnimGraphMotionNode*>* m_motionNodes = nullptr;
        BlendTreeBlendNNode* m_blendNNode = nullptr;
        BlendTree* m_blendTree = nullptr;
    };

    TEST_F(BlendTreeBlendNNodeTests, RandomizeMotion)
    {
        bool success = true;

        MCore::AttributeFloat* testParameter = m_animGraphInstance->GetParameterValueChecked<MCore::AttributeFloat>(0);
        testParameter->SetValue(-10.0f);
        Evaluate();
        AnimGraphNode* outNodeA;
        AnimGraphNode* outNodeB;
        uint32 outIndexA;
        uint32 outIndexB;
        float  outWeight;
        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA == outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>(m_motionNodes->front());
        success = success && outWeight <= 0;

        testParameter->SetValue(-1.0f);
        Evaluate();

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA == outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>((*m_motionNodes).front());
        success = success && outWeight <= 0.0f;

        testParameter->SetValue(-0.5f);
        Evaluate();
        const float tolerance = 0.001f;

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA != outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>(m_motionNodes->front());
        success = success && outNodeB == static_cast<AnimGraphNode*>((*m_motionNodes)[1]);
        success = success && outWeight > 0.5f - tolerance && outWeight < 0.5f + tolerance;

        testParameter->SetValue(0.5f);
        Evaluate();

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA != outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>((*m_motionNodes)[1]);
        success = success && outNodeB == static_cast<AnimGraphNode*>((*m_motionNodes)[2]);
        success = success && outWeight > 0.5f - tolerance && outWeight < 0.5f + tolerance;

        testParameter->SetValue(1.0f);
        Evaluate();

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA == outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>((*m_motionNodes).back());
        success = success && outWeight <= 0.0f;

        testParameter->SetValue(10.0f);
        Evaluate();

        m_blendNNode->FindBlendNodes(m_animGraphInstance, &outNodeA, &outNodeB, &outIndexA, &outIndexB, &outWeight);
        success = success && outNodeA == outNodeB;
        success = success && outNodeA == static_cast<AnimGraphNode*>(m_motionNodes->back());
        success = success && outWeight <= 0;

        ASSERT_TRUE(success);
    }

    ///////////////////////////////////////////////////////////////////////////

    struct BlendNSyncTestParam
    {
        AZ::u32 m_motionNodeCount;
        float m_minWeight;
        float m_maxWeight;
        float m_testWeight;
    };

    class BlendTreeBlendNNodeSyncTestFixture : public AnimGraphFixture
        , public ::testing::WithParamInterface<BlendNSyncTestParam>
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
            const BlendNSyncTestParam& param = GetParam();

            m_blendNNode = aznew BlendTreeBlendNNode();
            m_blendTree->AddChildNode(m_blendNNode);
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            finalNode->AddConnection(m_blendNNode, BlendTreeBlendNNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            ASSERT_TRUE(param.m_motionNodeCount <= 10) << "The blend N node only has 10 pose inputs.";
            for (uint16 i = 0; i < param.m_motionNodeCount; ++i)
            {
                AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
                m_blendTree->AddChildNode(motionNode);
                m_blendNNode->AddConnection(motionNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, i);
                m_motionNodes.push_back(motionNode);
            }
            m_blendNNode->UpdateParamWeights();
            m_blendNNode->SetParamWeightsEquallyDistributed(param.m_minWeight, param.m_maxWeight);
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

    // Make sure we don't crash when we have no inputs
    // Also make sure removing connections on BlendN doesn't crash
    TEST_F(BlendTreeBlendNNodeTests, NoInputsNoCrashTest)
    {
        // Remove all input connections of the blendN node.
        while (m_blendNNode->GetNumConnections() > 0)
        {
            const BlendTreeConnection* connection = m_blendNNode->GetConnection(0);
            m_blendNNode->RemoveConnection(
                connection->GetSourceNode(),
                connection->GetSourcePort(),
                connection->GetTargetPort());
        }

        // Update EMFX, which crashed before in the above mentioned Jira bug reports.
        GetEMotionFX().Update(0.1f); 
    }

    TEST_P(BlendTreeBlendNNodeSyncTestFixture, PlaySpeedAndTimeSyncTests)
    {
        const float epsilon = 0.0001f;
        const BlendNSyncTestParam& param = GetParam();
        ASSERT_TRUE(param.m_maxWeight > param.m_minWeight) << "Invalid test weight range. The min weight is bigger than the max weight";
        const float weightRange = param.m_maxWeight - param.m_minWeight;

        m_floatNode->SetValue(param.m_testWeight);
        GetEMotionFX().Update(0.0f);

        const size_t simulationTime = m_motionNodes.size() + 1;
        const size_t sampleRate = 24;
        const float timeDelta = 1.0f / static_cast<float>(sampleRate);
        const size_t numFramesToSimulate = simulationTime * sampleRate;
        for (size_t frame = 0; frame < numFramesToSimulate; ++frame)
        {
            GetEMotionFX().Update(timeDelta);

            float blendWeight;
            AnimGraphNode* nodeA;
            AnimGraphNode* nodeB;
            uint32 poseIndexA;
            uint32 poseIndexB;
            m_blendNNode->FindBlendNodes(m_animGraphInstance, &nodeA, &nodeB, &poseIndexA, &poseIndexB, &blendWeight);

            // Check if the correct motions are picked and blended.
            const float normalizedWeight = (param.m_testWeight - param.m_minWeight) / weightRange;
            const int motionIndexA = static_cast<int>(AZ::Lerp(0.0f, static_cast<float>(m_motionNodes.size() - 1), normalizedWeight));
            EXPECT_TRUE(motionIndexA < m_motionNodes.size());
            EXPECT_TRUE(m_motionNodes[motionIndexA] == nodeA);

            if (nodeA != nodeB)
            {
                const int motionIndexB = motionIndexA + 1;
                EXPECT_TRUE(motionIndexB < m_motionNodes.size());
                EXPECT_TRUE(m_motionNodes[motionIndexB] == nodeB);

                const float playSpeedA = nodeA->GetPlaySpeed(m_animGraphInstance);
                const float playTimeA = nodeA->GetCurrentPlayTime(m_animGraphInstance);
                const float durationA = nodeA->GetDuration(m_animGraphInstance);
                const float playSpeedB = nodeB->GetPlaySpeed(m_animGraphInstance);
                const float playTimeB = nodeB->GetCurrentPlayTime(m_animGraphInstance);
                const float durationB = nodeB->GetDuration(m_animGraphInstance);
                const float playSpeedN = m_blendNNode->GetPlaySpeed(m_animGraphInstance);
                const float playTimeN = m_blendNNode->GetCurrentPlayTime(m_animGraphInstance);
                const float durationN = m_blendNNode->GetDuration(m_animGraphInstance);
                ASSERT_TRUE(durationA > 0.0f && durationB > 0.0f) << "Invalid test data, motion nodes should have a max time bigger than 0.0";

                // Node A is the primary sync node, so the blend N node has to mimic it.
                EXPECT_NEAR(playSpeedA, playSpeedN, epsilon);
                EXPECT_NEAR(playTimeA, playTimeN, epsilon);
                EXPECT_NEAR(durationA, durationN, epsilon);

                // Node B gets synced to the blend N node which got synced to node A.
                const float timeRatio2 = durationB / durationA;
                const float factorB = AZ::Lerp(timeRatio2, 1.0f, blendWeight);
                const float primaryMotionPlaySpeed = m_motionNodes[motionIndexA]->GetDefaultPlaySpeed();
                const float interpolatedSpeed = AZ::Lerp(playSpeedA, primaryMotionPlaySpeed, blendWeight);
                const float expectedSecondaryPlaySpeed = interpolatedSpeed * factorB;
                EXPECT_NEAR(playSpeedB, expectedSecondaryPlaySpeed, epsilon);

                const float normalizedPlayTimeA = playTimeA / durationA;
                const float normalizedPlayTimeB = playTimeB / durationB;
                EXPECT_NEAR(normalizedPlayTimeA, normalizedPlayTimeB, epsilon);
            }
        }
    }

    std::vector<BlendNSyncTestParam> blendNNodeSyncTestData
    {
        {
            2,
            0.0f,
            1.0f,
            0.25f
        },
        {
            2,
            0.0f,
            1.0f,
            0.5f
        },
        {
            2,
            0.0f,
            1.0f,
            0.75f
        },
        {
            3,
            0.0f,
            1.0f,
            0.0f
        },
        {
            3,
            0.0f,
            2.0f,
            1.5f
        },
        {
            5,
            0.0f,
            4.0f,
            2.25f
        },
        {
            10,
            0.0f,
            10.0f,
            7.75f
        },
        {
            3,
            -1.0f,
            1.0f,
            0.25f
        }
    };

    INSTANTIATE_TEST_CASE_P(BlendTreeBlendNNode,
        BlendTreeBlendNNodeSyncTestFixture,
        ::testing::ValuesIn(blendNNodeSyncTestData));
} // namespace EMotionFX
