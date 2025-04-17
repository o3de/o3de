
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphFixture.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphMotionNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <EMotionFX/Source/BlendTreeBoolLogicNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <AzCore/std/containers/vector.h>

namespace EMotionFX
{

    class BoolLogicNodeTests : public AnimGraphFixture
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
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();

            m_blendTree->AddChildNode(m_blendNNode);
            m_blendTree->AddChildNode(finalNode);
            finalNode->AddConnection(m_blendNNode, BlendTreeBlendNNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            const uint16 motionNodeCount = 2;
            for (uint16 i = 0; i < motionNodeCount; ++i)
            {
                AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
                m_blendTree->AddChildNode(motionNode);
                m_blendNNode->AddConnection(motionNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, i);
                m_motionNodes->push_back(motionNode);
            }

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

        void AddValueParameter(const AZ::TypeId& typeId, const AZStd::string& name)
        {
            Parameter* parameter = ParameterFactory::Create(typeId);
            parameter->SetName(name);
            m_blendTreeAnimGraph->AddParameter(parameter);
            m_animGraphInstance->AddMissingParameterValues();
        }

        bool CalculateExpectedResult(BlendTreeBoolLogicNode::EFunction functionEnum, bool x, bool y, bool& error)
        {
            bool result = false;
            switch (functionEnum)
            {
            case BlendTreeBoolLogicNode::EFunction::FUNCTION_AND:
                result = x && y;
                break;
            case BlendTreeBoolLogicNode::EFunction::FUNCTION_OR:
                result = x || y;
                break;
            case BlendTreeBoolLogicNode::EFunction::FUNCTION_XOR:
                result = x ^ y;
                break;
            case BlendTreeBoolLogicNode::EFunction::FUNCTION_NAND:
                result = !(x && y);
                break;
            case BlendTreeBoolLogicNode::EFunction::FUNCTION_NOR:
                result = !(x || y);
                break;
            case BlendTreeBoolLogicNode::EFunction::FUNCTION_XNOR:
                result = !(x ^ y);
                break;
            case BlendTreeBoolLogicNode::EFunction::FUNCTION_NOT_X:
                result = !x;
                break;
            case BlendTreeBoolLogicNode::EFunction::FUNCTION_NOT_Y:
                result = !y;
                break;
            default:
                error = true;
                break;
            }

            return result;
        }

        AZStd::vector<AnimGraphMotionNode*>* m_motionNodes = nullptr;
        BlendTreeBlendNNode* m_blendNNode = nullptr;
        BlendTree* m_blendTree = nullptr;
    };

    TEST_F(BoolLogicNodeTests, TestBoolLogic)
    {
        bool success = true;
        const AZStd::string nameBoolX("parameter_bool_x_test");
        const AZStd::string nameBoolY("parameter_bool_y_test");

        AddValueParameter(azrtti_typeid<BoolParameter>(), nameBoolX);
        AddValueParameter(azrtti_typeid<BoolParameter>(), nameBoolY);

        BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
        m_blendTree->AddChildNode(parameterNode);
        parameterNode->InitAfterLoading(m_blendTreeAnimGraph.get());
        parameterNode->InvalidateUniqueData(m_animGraphInstance);

        BlendTreeBoolLogicNode* boolLogicNode = aznew BlendTreeBoolLogicNode();
        m_blendTree->AddChildNode(boolLogicNode);
        boolLogicNode->InitAfterLoading(m_blendTreeAnimGraph.get());
        boolLogicNode->InvalidateUniqueData(m_animGraphInstance);

        const AZ::Outcome<size_t> boolXParamIndexOutcome = m_animGraphInstance->FindParameterIndex(nameBoolX);
        const AZ::Outcome<size_t> boolYParamIndexOutcome = m_animGraphInstance->FindParameterIndex(nameBoolY);
        success = boolXParamIndexOutcome.IsSuccess() && boolYParamIndexOutcome.IsSuccess();

        uint32 boolXOutputPortIndex = InvalidIndex32;
        uint32 boolYOutputPortIndex = InvalidIndex32;
        const int portIndicesTosetCount = 2;
        int portIndicesFound = 0;
        const AZStd::vector<EMotionFX::AnimGraphNode::Port>& parameterNodeOutputPorts = parameterNode->GetOutputPorts();
        for (const EMotionFX::AnimGraphNode::Port& port : parameterNodeOutputPorts)
        {
            uint32 paramIndex = parameterNode->GetParameterIndex(port.m_portId);
            if (paramIndex == boolXParamIndexOutcome.GetValue())
            {
                boolXOutputPortIndex = port.m_portId;
                portIndicesFound++;
            }
            else if (paramIndex == boolYParamIndexOutcome.GetValue())
            {
                boolYOutputPortIndex = port.m_portId;
                portIndicesFound++;
            }
        }
        success = success && (portIndicesFound == portIndicesTosetCount);
        bool expectedResultSuccessful = true;
        if (success)
        {
            boolLogicNode->AddConnection(parameterNode, static_cast<uint16>(boolXOutputPortIndex), BlendTreeBoolLogicNode::INPUTPORT_X);

            boolLogicNode->AddConnection(parameterNode, static_cast<uint16>(boolYOutputPortIndex), BlendTreeBoolLogicNode::INPUTPORT_Y);

            m_blendNNode->AddConnection(boolLogicNode, BlendTreeBoolLogicNode::OUTPUTPORT_BOOL, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
            m_blendTreeAnimGraph->RecursiveReinit();

            MCore::AttributeBool* testBoolXParameter = static_cast<MCore::AttributeBool*>(m_animGraphInstance->FindParameter(nameBoolX));
            testBoolXParameter->SetValue(false);

            MCore::AttributeBool* testBoolYParameter = static_cast<MCore::AttributeBool*>(m_animGraphInstance->FindParameter(nameBoolY));
            testBoolYParameter->SetValue(false);

            Evaluate();

            MCore::Attribute* attribute = m_blendNNode->GetInputAttribute(m_animGraphInstance, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
            if (!attribute)
            {
                success = false;
            }
            else
            {
                // Odds are X even are Y
                const bool tableOfTruthInput[8] = { false, false, false, true, true, false, true, true };
                BlendTreeBoolLogicNode::EFunction functions[8] = {
                    BlendTreeBoolLogicNode::EFunction::FUNCTION_AND, BlendTreeBoolLogicNode::EFunction::FUNCTION_OR,
                    BlendTreeBoolLogicNode::EFunction::FUNCTION_XOR, BlendTreeBoolLogicNode::EFunction::FUNCTION_NAND,
                    BlendTreeBoolLogicNode::EFunction::FUNCTION_NOR, BlendTreeBoolLogicNode::EFunction::FUNCTION_XNOR,
                    BlendTreeBoolLogicNode::EFunction::FUNCTION_NOT_X, BlendTreeBoolLogicNode::EFunction::FUNCTION_NOT_Y
                };
                for (int functionIndex = 0; functionIndex < 8; ++functionIndex)
                {
                    boolLogicNode->SetFunction(functions[functionIndex]);
                    for (int inputIndex = 0; inputIndex < 8; inputIndex += 2)
                    {
                        testBoolXParameter->SetValue(tableOfTruthInput[inputIndex]);
                        testBoolYParameter->SetValue(tableOfTruthInput[inputIndex + 1]);
                        Evaluate();
                        bool error = false;
                        const bool expectedResult = CalculateExpectedResult(boolLogicNode->GetFunction(), testBoolXParameter->GetValue(), testBoolYParameter->GetValue(), error);
                        const bool result = m_blendNNode->GetInputNumberAsBool(m_animGraphInstance, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
                        EXPECT_FALSE(error) << "Boolean logic node: CalculateExpectedResult returned error";
                        EXPECT_EQ(result, expectedResult) << "Boolean logic node: function " << functions[functionIndex] << " did not return the expected result";

                        expectedResultSuccessful = expectedResultSuccessful && !error && (result == expectedResult);
                    }
                }
            }
        }

        ASSERT_TRUE(success && expectedResultSuccessful);
    }

} // end namespace EMotionFX
