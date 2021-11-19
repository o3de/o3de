/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreeRagdollNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/RagdollInstance.h>
#include <Tests/ActorFixture.h>
#include <Tests/AnimGraphFixture.h>
#include <Tests/Mocks/PhysicsRagdoll.h>

namespace EMotionFX
{
    // Tests if the activation input port can be controlled with a non-boolean/float values from a const float node.
    class BlendTreeRagdollNode_ConstFloatActivateInputTest
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<float>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            BlendTree* blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            /*
                +-------------+    +---------+    +------------+
                | Const Float |--->| Ragdoll |--->| Final Node |
                +-------------+    +---------+    +------------+
            */
            BlendTreeFloatConstantNode* floatConstNode = aznew BlendTreeFloatConstantNode();
            floatConstNode->SetValue(GetParam());
            blendTree->AddChildNode(floatConstNode);

            m_ragdollNode = aznew BlendTreeRagdollNode();
            blendTree->AddChildNode(m_ragdollNode);

            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            blendTree->AddChildNode(finalNode);

            m_ragdollNode->AddUnitializedConnection(floatConstNode, BlendTreeFloatConstantNode::PORTID_OUTPUT_RESULT, BlendTreeRagdollNode::PORTID_ACTIVATE);
            finalNode->AddUnitializedConnection(m_ragdollNode, BlendTreeRagdollNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

        AZStd::unique_ptr<OneBlendTreeNodeAnimGraph> m_blendTreeAnimGraph;
        BlendTreeRagdollNode* m_ragdollNode = nullptr;
    };

    TEST_P(BlendTreeRagdollNode_ConstFloatActivateInputTest, BlendTreeRagdollNode_ConstFloatActivateInputTest)
    {
        GetEMotionFX().Update(0.0f);

        const float constFloatValue = GetParam();
        const bool isActivated = m_ragdollNode->IsActivated(m_animGraphInstance);

        EXPECT_EQ(!MCore::Math::IsFloatZero(constFloatValue), isActivated)
            << "Activation expected in case const float value is not zero.";
    }

    INSTANTIATE_TEST_CASE_P(BlendTreeRagdollNode_ConstFloatActivateInputTest,
        BlendTreeRagdollNode_ConstFloatActivateInputTest,
        ::testing::ValuesIn({ -1.0f, 0.0f, 0.1f, 1.0f }));

    ///////////////////////////////////////////////////////////////////////////

    struct RagdollRootNodeParam
    {
        std::string m_ragdollRootNode;
        bool m_ragdollRootNodeSimulated;
        std::vector<std::string> m_ragdollConfigNodeNames;
        std::vector<std::string> m_simulatedJointNames;
    };

    class RagdollRootNodeFixture
        : public ActorFixture
        , public ::testing::WithParamInterface<RagdollRootNodeParam>
    {
    public:
        void AddRagdollNodeConfig(AZStd::vector<Physics::RagdollNodeConfiguration>& ragdollNodes, const AZStd::string& jointName)
        {
            Physics::RagdollNodeConfiguration nodeConfig;
            nodeConfig.m_debugName = jointName;
            ragdollNodes.emplace_back(nodeConfig);
        }
    };

    TEST_P(RagdollRootNodeFixture, RagdollRootNodeIsSimulatedTests)
    {
        Physics::RagdollConfiguration& ragdollConfig = GetActor()->GetPhysicsSetup()->GetRagdollConfig();
        AZStd::vector<Physics::RagdollNodeConfiguration>& ragdollNodes = ragdollConfig.m_nodes;
        const RagdollRootNodeParam& param = GetParam();
        const AZStd::string ragdollRootNodeName = param.m_ragdollRootNode.c_str();

        // Create the ragdoll config.
        for (const std::string& jointName : param.m_ragdollConfigNodeNames)
        {
            AddRagdollNodeConfig(ragdollNodes, jointName.c_str());
        }

        // Create the ragdoll instance and check if the ragdoll root node is set correctly.
        TestRagdoll testRagdoll;
        EXPECT_CALL(testRagdoll, GetState(::testing::_)).Times(::testing::AnyNumber());
        EXPECT_CALL(testRagdoll, GetNumNodes()).WillRepeatedly(::testing::Return(1));
        EXPECT_CALL(testRagdoll, IsSimulated()).WillRepeatedly(::testing::Return(true));
        EXPECT_CALL(testRagdoll, GetPosition()).WillRepeatedly(::testing::Return(AZ::Vector3::CreateZero()));
        EXPECT_CALL(testRagdoll, GetOrientation()).WillRepeatedly(::testing::Return(AZ::Quaternion::CreateIdentity()));
        m_actorInstance->SetRagdoll(&testRagdoll);
        RagdollInstance* ragdollInstance = m_actorInstance->GetRagdollInstance();
        const AZ::Outcome<size_t> rootNodeIndex = ragdollInstance->GetRootRagdollNodeIndex();
        ASSERT_TRUE(rootNodeIndex.IsSuccess()) << "No root node for the ragdoll found.";
        EXPECT_EQ(ragdollInstance->GetRagdollRootNode()->GetNameString(), ragdollRootNodeName) << "Wrong ragdoll root node.";

        // Create an anim graph with a ragdoll node.
        AZStd::unique_ptr<MotionSet> motionSet = AZStd::make_unique<MotionSet>("testMotionSet");
        AZStd::unique_ptr<AnimGraph> animGraph = AnimGraphFactory::Create<EmptyAnimGraph>();

        BlendTree* blendTree = aznew BlendTree();
        animGraph->GetRootStateMachine()->AddChildNode(blendTree);
        animGraph->GetRootStateMachine()->SetEntryState(blendTree);

        BlendTreeRagdollNode* ragdollNode = aznew BlendTreeRagdollNode();

        // Set the simulated joints.
        AZStd::vector<AZStd::string> simulatedJointNames;
        for (const std::string& jointName : param.m_simulatedJointNames)
        {
            simulatedJointNames.emplace_back(jointName.c_str());
        }
        ragdollNode->SetSimulatedJointNames(simulatedJointNames);
        blendTree->AddChildNode(ragdollNode);

        EXPECT_TRUE(animGraph->InitAfterLoading());

        AnimGraphInstance* animGraphInstance = AnimGraphInstance::Create(animGraph.get(), m_actorInstance, motionSet.get());
        m_actorInstance->SetAnimGraphInstance(animGraphInstance);

        BlendTreeRagdollNode::UniqueData* uniqueData = static_cast<BlendTreeRagdollNode::UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(ragdollNode));
        ASSERT_TRUE(uniqueData) << "Cannot find unique data for ragdoll node.";

        // Check if the ragdoll root node is simulated or if the ragdoll is partial.
        EXPECT_EQ(uniqueData->m_isRagdollRootNodeSimulated, param.m_ragdollRootNodeSimulated) << "Ragdoll root node should not be simulated.";
    }

    std::vector<RagdollRootNodeParam> ragdollRootNodeIsSimulatedTestValues = 
    {
        // Simulated root node, in skeleton hierarchy order
        {
            "Bip01__pelvis",
            true,
            {
                "Bip01__pelvis",
                "l_upLeg",
                "l_loLeg"
            },
            {
                "Bip01__pelvis",
                "l_upLeg",
                "l_loLeg"
            }
        },
        // Simulated root node, reordered
        {
            "Bip01__pelvis",
            true,
            {
                "l_upLeg",
                "l_loLeg",
                "Bip01__pelvis"
            },
            {
                "l_upLeg",
                "Bip01__pelvis",
                "l_loLeg"
            }
        },
        // Partial ragdoll, root node not simulated, reordered
        {
            "Bip01__pelvis",
            false,
            {
                "l_upLeg",
                "l_loLeg",
                "Bip01__pelvis"
            },
            {
                "l_upLeg",
                "l_loLeg"
            }
        }
    };

    INSTANTIATE_TEST_CASE_P(RagdollRootNodeIsSimulatedTests,
        RagdollRootNodeFixture,
        ::testing::ValuesIn(ragdollRootNodeIsSimulatedTestValues));
} // namespace EMotionFX
