/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeConnection.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    // Waypoints are the reroutes of a blend tree: editor layout stored on the connection, which the blend tree never evaluates.
    class BlendTreeConnectionWaypointFixture : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            BlendTree* tree = m_blendTreeAnimGraph->GetBlendTreeNode();

            auto* finalNode = aznew BlendTreeFinalNode();
            m_blend = aznew BlendTreeBlend2Node();
            m_weight = aznew BlendTreeFloatConstantNode();
            auto* poseA = aznew AnimGraphBindPoseNode();
            auto* poseB = aznew AnimGraphBindPoseNode();

            finalNode->SetName("Final");
            m_blend->SetName("Blend");
            m_weight->SetName("Weight");
            poseA->SetName("PoseA");
            poseB->SetName("PoseB");
            for (AnimGraphNode* node : AZStd::vector<AnimGraphNode*>{ finalNode, m_blend, m_weight, poseA, poseB })
            {
                tree->AddChildNode(node);
            }

            m_blend->AddConnection(poseA, 0, BlendTreeBlend2Node::INPUTPORT_POSE_A);
            m_blend->AddConnection(poseB, 0, BlendTreeBlend2Node::INPUTPORT_POSE_B);
            m_weightConnection = m_blend->AddConnection(m_weight, 0, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            finalNode->AddConnection(m_blend, 0, 0);
            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

    protected:
        BlendTreeBlend2Node* m_blend = nullptr;
        BlendTreeFloatConstantNode* m_weight = nullptr;
        BlendTreeConnection* m_weightConnection = nullptr;
        const AZStd::vector<AZ::Vector2> m_waypoints{ AZ::Vector2(10.0f, 20.0f), AZ::Vector2(30.0f, 40.0f) };
    };

    TEST_F(BlendTreeConnectionWaypointFixture, WaypointsUndoRedoAndClear)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;

        CommandSystem::AdjustConnectionWaypoints(m_blend, m_weightConnection, m_waypoints);
        ASSERT_EQ(m_weightConnection->GetWaypoints(), m_waypoints);
        EXPECT_TRUE(m_blendTreeAnimGraph->GetDirtyFlag());

        ASSERT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_TRUE(m_weightConnection->GetWaypoints().empty());

        ASSERT_TRUE(commandManager.Redo(result)) << result.c_str();
        EXPECT_EQ(m_weightConnection->GetWaypoints(), m_waypoints);

        CommandSystem::AdjustConnectionWaypoints(m_blend, m_weightConnection, {});
        EXPECT_TRUE(m_weightConnection->GetWaypoints().empty());

        ASSERT_TRUE(commandManager.Undo(result)) << result.c_str();
        EXPECT_EQ(m_weightConnection->GetWaypoints(), m_waypoints);
    }

    TEST_F(BlendTreeConnectionWaypointFixture, AdjustingAMissingConnectionFails)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;

        EXPECT_FALSE(commandManager.ExecuteCommand(AZStd::string::format(
            "AnimGraphAdjustConnection -animGraphID %u -targetNode Blend -id %s",
            m_blendTreeAnimGraph->GetID(), AnimGraphConnectionId::Create().ToString().c_str()), result));
        EXPECT_FALSE(commandManager.ExecuteCommand(AZStd::string::format(
            "AnimGraphAdjustConnection -animGraphID %u -targetNode Missing -id %s",
            m_blendTreeAnimGraph->GetID(), m_weightConnection->GetId().ToString().c_str()), result));
    }

    TEST_F(BlendTreeConnectionWaypointFixture, UndoingAConnectionRemovalRestoresItsWaypoints)
    {
        CommandSystem::CommandManager commandManager;
        AZStd::string result;
        m_weightConnection->SetWaypoints(m_waypoints);
        const AnimGraphConnectionId connectionId = m_weightConnection->GetId();

        ASSERT_TRUE(commandManager.ExecuteCommand(AZStd::string::format(
            "AnimGraphRemoveConnection -animGraphID %u -targetNode Blend -targetPort %d -sourceNode Weight -sourcePort 0",
            m_blendTreeAnimGraph->GetID(), BlendTreeBlend2Node::INPUTPORT_WEIGHT), result)) << result.c_str();
        ASSERT_EQ(m_blend->FindConnectionById(connectionId), nullptr);

        ASSERT_TRUE(commandManager.Undo(result)) << result.c_str();
        const BlendTreeConnection* restored = m_blend->FindConnectionById(connectionId);
        ASSERT_NE(restored, nullptr);
        EXPECT_EQ(restored->GetWaypoints(), m_waypoints);
    }

    TEST_F(BlendTreeConnectionWaypointFixture, WaypointsSurviveSerialization)
    {
        m_weightConnection->SetWaypoints(m_waypoints);
        const AZ::Outcome<AZStd::string> serialized = MCore::ReflectionSerializer::Serialize(m_weightConnection);
        ASSERT_TRUE(serialized.IsSuccess());

        BlendTreeConnection loaded;
        ASSERT_TRUE(MCore::ReflectionSerializer::Deserialize(&loaded, serialized.GetValue()));
        EXPECT_EQ(loaded.GetWaypoints(), m_waypoints);
        EXPECT_EQ(loaded.GetId(), m_weightConnection->GetId());
    }

    TEST_F(BlendTreeConnectionWaypointFixture, WaypointsDoNotChangeWhatTheBlendTreeComputes)
    {
        m_weight->SetValue(0.25f);
        GetEMotionFX().Update(1.0f / 60.0f);
        const float directWeight = m_blend->GetInputNumberAsFloat(m_animGraphInstance, BlendTreeBlend2Node::INPUTPORT_WEIGHT);

        for (AnimGraphNode* node : m_blendTreeAnimGraph->GetBlendTreeNode()->GetChildNodes())
        {
            for (BlendTreeConnection* connection : node->GetConnections())
            {
                connection->SetWaypoints(m_waypoints);
            }
        }
        GetEMotionFX().Update(1.0f / 60.0f);

        EXPECT_FLOAT_EQ(directWeight, 0.25f);
        EXPECT_FLOAT_EQ(m_blend->GetInputNumberAsFloat(m_animGraphInstance, BlendTreeBlend2Node::INPUTPORT_WEIGHT), directWeight);
    }
} // namespace EMotionFX
