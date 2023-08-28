/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/BlendTreeTwoLinkIKNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/RotationParameter.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    class QuaternionParameterFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<AZ::Quaternion>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_param = GetParam();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();
            AddParameter<RotationParameter, AZ::Quaternion>("quaternionTest", m_param);

            /*
            +------------+
            |bindPoseNode+---+
            +------------+   |
                             +-->+-------------+     +---------+
             +-----------+       |twoLinkIKNode+---->+finalNode|
             |m_paramNode+------>+-------------+     +---------+
             +-----------+
            */
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            m_paramNode = aznew BlendTreeParameterNode();

            // Using two link IK Node because its GoalRot input port uses quaternion.
            m_twoLinkIKNode = aznew BlendTreeTwoLinkIKNode();

            m_blendTree->AddChildNode(finalNode);
            m_blendTree->AddChildNode(m_twoLinkIKNode);
            m_blendTree->AddChildNode(bindPoseNode);
            m_blendTree->AddChildNode(m_paramNode);

            m_twoLinkIKNode->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeTwoLinkIKNode::PORTID_INPUT_POSE);
            finalNode->AddConnection(m_twoLinkIKNode, BlendTreeTwoLinkIKNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_blendTreeAnimGraph->InitAfterLoading();
        };

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

        template <class paramType, class inputType>
        void ParamSetValue(const AZStd::string& paramName, const inputType& value)
        {
            const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(paramName);
            MCore::Attribute* param = m_animGraphInstance->GetParameterValue(static_cast<AZ::u32>(parameterIndex.GetValue()));
            paramType* typeParam = static_cast<paramType*>(param);
            typeParam->SetValue(value);
        };

    protected:
        AZ::Quaternion m_param;
        BlendTree* m_blendTree = nullptr;
        BlendTreeParameterNode* m_paramNode = nullptr;
        BlendTreeTwoLinkIKNode* m_twoLinkIKNode = nullptr;

    private:
        template<class ParameterType, class ValueType>
        void AddParameter(const AZStd::string& name, const ValueType& defaultValue)
        {
            ParameterType* parameter = aznew ParameterType();
            parameter->SetName(name);
            parameter->SetDefaultValue(defaultValue);
            m_blendTreeAnimGraph->AddParameter(parameter);
        }
    };

    TEST_P(QuaternionParameterFixture, ParameterOutputsCorrectQuaternion)
    {
        // Parameter node needs to connect to another node, otherwise it will not update.
        m_twoLinkIKNode->AddConnection(m_paramNode, aznumeric_caster(m_paramNode->FindOutputPortIndex("quaternionTest")), BlendTreeTwoLinkIKNode::PORTID_INPUT_GOALROT);
        GetEMotionFX().Update(1.0f / 60.0f);

        // Check correct output for quaternion parameter.
        const AZ::Quaternion& quaternionTestParam = m_paramNode->GetOutputQuaternion(m_animGraphInstance,
            m_paramNode->FindOutputPortIndex("quaternionTest"))->GetValue();

        EXPECT_TRUE(quaternionTestParam.GetX() == m_param.GetX()) << "Quaternion X value should be the same as expected Quaternion X value.";
        EXPECT_TRUE(quaternionTestParam.GetY() == m_param.GetY()) << "Quaternion Y value should be the same as expected Quaternion Y value.";
        EXPECT_TRUE(quaternionTestParam.GetZ() == m_param.GetZ()) << "Quaternion Z value should be the same as expected Quaternion Z value.";
        EXPECT_TRUE(quaternionTestParam.GetW() == m_param.GetW()) << "Quaternion W value should be the same as expected Quaternion W value.";
    }

    TEST_P(QuaternionParameterFixture, QuaternionSetValueOutputsCorrectQuaternion)
    {
        m_twoLinkIKNode->AddConnection(m_paramNode, aznumeric_caster(m_paramNode->FindOutputPortIndex("quaternionTest")), BlendTreeTwoLinkIKNode::PORTID_INPUT_GOALROT);
        GetEMotionFX().Update(1.0f / 60.0f);

        // Shuffle the Quaternion parameter values to check changing quaternion values will be processed correctly.
        ParamSetValue<MCore::AttributeQuaternion, AZ::Quaternion>("quaternionTest", AZ::Quaternion(m_param.GetY(), m_param.GetZ(), m_param.GetX(), m_param.GetW()));
        GetEMotionFX().Update(1.0f / 60.0f);

        const AZ::Quaternion& quaternionTestParam = m_paramNode->GetOutputQuaternion(m_animGraphInstance,
            m_paramNode->FindOutputPortIndex("quaternionTest"))->GetValue();
        EXPECT_FLOAT_EQ(quaternionTestParam.GetX(), m_param.GetY()) << "Input Quaternion X value should be the same as expected Quaternion Y value.";
        EXPECT_FLOAT_EQ(quaternionTestParam.GetY(), m_param.GetZ()) << "Input Quaternion Y value should be the same as expected Quaternion Z value.";
        EXPECT_FLOAT_EQ(quaternionTestParam.GetZ(), m_param.GetX()) << "Input Quaternion Z value should be the same as expected Quaternion X value.";
        EXPECT_FLOAT_EQ(quaternionTestParam.GetW(), m_param.GetW()) << "Input Quaternion W value should be the same as expected Quaternion W value.";
    };

    std::vector<AZ::Quaternion> quaternionParameterTestData
    {
        AZ::Quaternion(0.0f, 0.0f, 0.0f, 1.0f),
        AZ::Quaternion(1.0f, 0.5f, -0.5f, 1.0f),
        AZ::Quaternion(AZ::Constants::FloatMax, -AZ::Constants::FloatMax, AZ::Constants::FloatEpsilon, 1.0f)
    };

    INSTANTIATE_TEST_CASE_P(QuaternionParameter_ValidOutputTests,
        QuaternionParameterFixture,
        ::testing::ValuesIn(quaternionParameterTestData)
    );
} // end namespace EMotionFX
