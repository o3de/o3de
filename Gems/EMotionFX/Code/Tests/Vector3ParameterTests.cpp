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
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    class Vector3ParameterFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<AZ::Vector3>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_param = GetParam();
            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            AddParameter<Vector3Parameter, AZ::Vector3>("vec3Test", m_param);

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

            // Using two link IK Node because its GoalPos input port uses Vector3
            m_twoLinkIKNode = aznew BlendTreeTwoLinkIKNode();

            m_blendTree->AddChildNode(finalNode);
            m_blendTree->AddChildNode(m_twoLinkIKNode);
            m_blendTree->AddChildNode(bindPoseNode);
            m_blendTree->AddChildNode(m_paramNode);

            m_twoLinkIKNode->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeTwoLinkIKNode::PORTID_INPUT_POSE);
            finalNode->AddConnection(m_twoLinkIKNode, BlendTreeTwoLinkIKNode::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);
            m_blendTreeAnimGraph->InitAfterLoading();
        }

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
            MCore::Attribute* param = m_animGraphInstance->GetParameterValue(parameterIndex.GetValue());
            paramType* typeParam = static_cast<paramType*>(param);
            typeParam->SetValue(value);
        }

    protected:
        BlendTree* m_blendTree = nullptr;
        BlendTreeParameterNode* m_paramNode = nullptr;
        BlendTreeTwoLinkIKNode* m_twoLinkIKNode = nullptr;
        AZ::Vector3 m_param;

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

    TEST_P(Vector3ParameterFixture, ParameterOutputsCorrectVector3Floats)
    {
        // Parameter node needs to connect to another node, otherwise it will not be updated
        m_twoLinkIKNode->AddConnection(m_paramNode, aznumeric_caster(m_paramNode->FindOutputPortIndex("vec3Test")), BlendTreeTwoLinkIKNode::PORTID_INPUT_GOALPOS);
        GetEMotionFX().Update(1.0f / 60.0f);

        // Check correct output for vector3 parameter.
        const AZ::Vector3& vec3TestParam = m_paramNode->GetOutputVector3(m_animGraphInstance,
            m_paramNode->FindOutputPortIndex("vec3Test"))->GetValue();

        EXPECT_FLOAT_EQ(vec3TestParam.GetX(), m_param.GetX()) << "Vector3 X value should be the same as expected vector3 X value.";
        EXPECT_FLOAT_EQ(vec3TestParam.GetY(), m_param.GetY()) << "Vector3 Y value should be the same as expected vector3 Y value.";
        EXPECT_FLOAT_EQ(vec3TestParam.GetZ(), m_param.GetZ()) << "Vector3 Z value should be the same as expected vector3 Z value.";
    };

    TEST_P(Vector3ParameterFixture, Vec3SetValueOutputsCorrectVector3Floats)
    {
        m_twoLinkIKNode->AddConnection(m_paramNode, aznumeric_caster(m_paramNode->FindOutputPortIndex("vec3Test")), BlendTreeTwoLinkIKNode::PORTID_INPUT_GOALPOS);
        GetEMotionFX().Update(1.0f / 60.0f);

        // Shuffle the vector3 parameter values to check changing vector3 values will be processed correctly.
        ParamSetValue<MCore::AttributeVector3, AZ::Vector3>("vec3Test", AZ::Vector3(m_param.GetY(), m_param.GetZ(), m_param.GetX()));
        GetEMotionFX().Update(1.0f / 60.0f);

        const AZ::Vector3& vec3TestParam = m_paramNode->GetOutputVector3(m_animGraphInstance,
            m_paramNode->FindOutputPortIndex("vec3Test"))->GetValue();
        EXPECT_FLOAT_EQ(vec3TestParam.GetX(), m_param.GetY()) << "Input vector3 X value should be the same as expected vector3 Y value.";
        EXPECT_FLOAT_EQ(vec3TestParam.GetY(), m_param.GetZ()) << "Input vector3 Y value should be the same as expected vector3 Z value.";
        EXPECT_FLOAT_EQ(vec3TestParam.GetZ(), m_param.GetX()) << "Input vector3 Z value should be the same as expected vector3 X value.";
    };

    std::vector<AZ::Vector3> Vector3ParameterTestData
    {
        AZ::Vector3(0.0f, 0.0f, 0.0f),
        AZ::Vector3(1.0f, 0.5f, -0.5f),
        AZ::Vector3(AZ::Constants::FloatMax, -AZ::Constants::FloatMax, AZ::Constants::FloatEpsilon)
    };

    INSTANTIATE_TEST_CASE_P(Vector3Parameter_ValidOutputTests,
        Vector3ParameterFixture,
        ::testing::ValuesIn(Vector3ParameterTestData)
    );
} // end namespace EMotionFX
