
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
#include <EMotionFX/Source/BlendTreeVector3DecomposeNode.h>
#include <EMotionFX/Source/BlendTreeVector2DecomposeNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/MotionData/NonUniformMotionData.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <AzCore/std/containers/vector.h>

namespace EMotionFX
{

    class Vector2ToVector3CompatibilityTests : public AnimGraphFixture
    {
    public:
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
            finalNode->AddUnitializedConnection(m_blendNNode, BlendTreeBlendNNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            const uint16 motionNodeCount = 3;
            for (uint16 i = 0; i < motionNodeCount; ++i)
            {
                AnimGraphMotionNode* motionNode = aznew AnimGraphMotionNode();
                m_blendTree->AddChildNode(motionNode);
                m_blendNNode->AddUnitializedConnection(motionNode, AnimGraphMotionNode::PORTID_OUTPUT_POSE, i);
                
                // The motion set keeps track of motions by their name. Each motion
                // within the motion set must have a unique name.
                const AZStd::string motionId = AZStd::string::format("testSkeletalMotion%i", i);
                Motion* motion = aznew Motion(motionId.c_str());
                motion->SetMotionData(aznew NonUniformMotionData());
                motion->GetMotionData()->SetDuration(1.0f);
                MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(motion->GetName(), motion->GetName(), motion);
                m_motionSet->AddMotionEntry(motionEntry);

                motionNode->AddMotionId(motionId.c_str());
            }

            {
                Parameter* parameter = ParameterFactory::Create(azrtti_typeid<Vector3Parameter>());
                parameter->SetName("parameter_vector3_test");
                m_blendTreeAnimGraph->AddParameter(parameter);
            }
            {
                Parameter* parameter = ParameterFactory::Create(azrtti_typeid<Vector2Parameter>());
                parameter->SetName("parameter_vector2_test");
                m_blendTreeAnimGraph->AddParameter(parameter);
            }

            BlendTreeParameterNode* parameterNode = aznew BlendTreeParameterNode();
            m_blendTree->AddChildNode(parameterNode);

            m_vector2DecomposeNode = aznew BlendTreeVector2DecomposeNode();
            m_blendTree->AddChildNode(m_vector2DecomposeNode);
            m_vector2DecomposeNode->AddUnitializedConnection(parameterNode, 0, BlendTreeVector2DecomposeNode::INPUTPORT_VECTOR);

            m_vector3DecomposeNode = aznew BlendTreeVector3DecomposeNode();
            m_blendTree->AddChildNode(m_vector3DecomposeNode);
            m_vector3DecomposeNode->AddUnitializedConnection(parameterNode, 1, BlendTreeVector3DecomposeNode::INPUTPORT_VECTOR);

            m_blendNNode->AddUnitializedConnection(m_vector3DecomposeNode, BlendTreeVector3DecomposeNode::OUTPUTPORT_X, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
            m_blendTreeAnimGraph->InitAfterLoading();
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

        BlendTreeBlendNNode* m_blendNNode = nullptr;
        BlendTree* m_blendTree = nullptr;
        BlendTreeVector3DecomposeNode* m_vector3DecomposeNode = nullptr;
        BlendTreeVector2DecomposeNode* m_vector2DecomposeNode = nullptr;
    };

    TEST_F(Vector2ToVector3CompatibilityTests, Evaluation)
    {
        AZ::Outcome<size_t> vector2ParamIndexOutcome = m_blendTreeAnimGraph->FindValueParameterIndexByName("parameter_vector2_test");
        ASSERT_TRUE(vector2ParamIndexOutcome.IsSuccess());
        AZ::Outcome<size_t> vector3ParamIndexOutcome = m_blendTreeAnimGraph->FindValueParameterIndexByName("parameter_vector3_test");
        ASSERT_TRUE(vector3ParamIndexOutcome.IsSuccess());
        
        MCore::AttributeVector2* testVector2Parameter = static_cast<MCore::AttributeVector2*>(m_animGraphInstance->GetParameterValue(static_cast<uint32>(vector2ParamIndexOutcome.GetValue())));
        testVector2Parameter->SetValue(AZ::Vector2(-1.0f, 0.5f));

        MCore::AttributeVector3* testVector3Parameter = static_cast<MCore::AttributeVector3*>(m_animGraphInstance->GetParameterValue(static_cast<uint32>(vector3ParamIndexOutcome.GetValue())));
        testVector3Parameter->SetValue(AZ::Vector3(1.0f, 2.5f, 3.5f));

        Evaluate();

        MCore::AttributeFloat* attributeFloatX = m_vector3DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector3DecomposeNode::OUTPUTPORT_X);
        ASSERT_TRUE(attributeFloatX);
        MCore::AttributeFloat* attributeFloatY = m_vector3DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector3DecomposeNode::OUTPUTPORT_Y);
        ASSERT_TRUE(attributeFloatY);
        MCore::AttributeFloat* attributeFloatZ = m_vector3DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector3DecomposeNode::OUTPUTPORT_Z);
        ASSERT_TRUE(attributeFloatZ);

        MCore::AttributeFloat* attributeFloatX1 = m_vector2DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector2DecomposeNode::OUTPUTPORT_X);
        ASSERT_TRUE(attributeFloatX1);
        MCore::AttributeFloat* attributeFloatY1 = m_vector2DecomposeNode->GetOutputFloat(m_animGraphInstance, BlendTreeVector2DecomposeNode::OUTPUTPORT_Y);
        ASSERT_TRUE(attributeFloatY1);

        const AZ::Vector2& testParameterVector2Value = testVector2Parameter->GetValue();
        const AZ::Vector3 testParameterVector3Value(testVector3Parameter->GetValue());
        const float tolerance = 0.001f;
        float outX = attributeFloatX->GetValue();
        float outY = attributeFloatY->GetValue();
        float outZ = attributeFloatZ->GetValue();
        AZ::Vector3 decomposedValuesVector3(outX, outY, outZ);
        AZ::Vector3 expectedDecomposedValuesVector3(testParameterVector2Value.GetX(), testParameterVector2Value.GetY(), 0.0f);
        float differenceLength = static_cast<float>((expectedDecomposedValuesVector3 - decomposedValuesVector3).GetLength());
        ASSERT_TRUE(AZ::IsClose(0, differenceLength, tolerance));

        m_blendNNode->RemoveConnection(m_vector3DecomposeNode, BlendTreeVector3DecomposeNode::OUTPUTPORT_X, BlendTreeBlendNNode::INPUTPORT_WEIGHT);
        m_blendNNode->AddConnection(m_vector2DecomposeNode, BlendTreeVector3DecomposeNode::OUTPUTPORT_X, BlendTreeBlendNNode::INPUTPORT_WEIGHT);

        Evaluate();

        outX = attributeFloatX1->GetValue();
        outY = attributeFloatY1->GetValue();

        AZ::Vector2 decomposedValuesVector2(outX, outY);
        AZ::Vector2 expectedDecomposedValuesVector2(testParameterVector3Value.GetX(), testParameterVector3Value.GetY());
        differenceLength = static_cast<float>((expectedDecomposedValuesVector2 - decomposedValuesVector2).GetLength());
        ASSERT_TRUE(AZ::IsClose(0, differenceLength, tolerance));
    }

} // end namespace EMotionFX
