/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Random.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphBindPoseNode.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeBlend2Node.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendTreeFloatMath1Node.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/IntSliderParameter.h>
#include <Tests/AnimGraphFixture.h>

namespace EMotionFX
{
    struct BlendTreeFloatMath1NodeTestData
    {
        std::vector<float> m_xInputFloat;
        std::vector<int> m_xInputInt;
        std::vector<bool> m_xInputBool;
    };

    std::vector<BlendTreeFloatMath1NodeTestData> blendTreeFloatMath1NodeTestData
    {
        {
            // TODO: MCore RandF function does not handle extreme values properly
            // eg. MCore::Math::RandF(0, FLT_MAX) returns inf
            {1000.3f, -1000.3f, 0.1f, -1.2f, 1.2f},
            {1000, -1000, 0, -1, 1},
            {true, false}
        }
    };

    class BlendTreeFloatMath1NodeFixture
        : public AnimGraphFixture
        , public ::testing::WithParamInterface<BlendTreeFloatMath1NodeTestData>
    {
    public:
        void ConstructGraph() override
        {
            AnimGraphFixture::ConstructGraph();
            m_param = GetParam();

            m_blendTreeAnimGraph = AnimGraphFactory::Create<OneBlendTreeNodeAnimGraph>();
            m_rootStateMachine = m_blendTreeAnimGraph->GetRootStateMachine();
            m_blendTree = m_blendTreeAnimGraph->GetBlendTreeNode();

            AddParameter<FloatSliderParameter>("FloatParam", 0.0f);
            AddParameter<BoolParameter>("BoolParam", false);
            AddParameter<IntSliderParameter>("IntParam", 0);

            /*
                                   +------------------+
                                   |                  |
                                   | bindPoseNode     |
                                   |                  |   +------------------+   +------------------+
                                   +------------------+-->+                  |   |                  |
                                                          | blend2Node       +-->+ finalNode        |
            +------------------+   +------------------+   |                  |   |                  |
            |                  |   |                  +-->+------------------+   +------------------+
            | m_paramNode      +-->+ m_floatMath1Node |
            |                  |   |                  |
            +------------------+   +------------------+
            */
            BlendTreeFinalNode* finalNode = aznew BlendTreeFinalNode();
            m_blendTree->AddChildNode(finalNode);
            AnimGraphBindPoseNode* bindPoseNode = aznew AnimGraphBindPoseNode();
            m_blendTree->AddChildNode(bindPoseNode);
            BlendTreeBlend2Node* blend2Node = aznew BlendTreeBlend2Node();
            m_blendTree->AddChildNode(blend2Node);
            m_floatMath1Node = aznew BlendTreeFloatMath1Node();
            m_blendTree->AddChildNode(m_floatMath1Node);
            m_paramNode = aznew BlendTreeParameterNode();
            m_blendTree->AddChildNode(m_paramNode);

            // Connect the nodes.
            blend2Node->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::INPUTPORT_POSE_A);
            blend2Node->AddConnection(bindPoseNode, AnimGraphBindPoseNode::PORTID_OUTPUT_POSE, BlendTreeBlend2Node::INPUTPORT_POSE_B);
            blend2Node->AddConnection(m_floatMath1Node, BlendTreeFloatMath1Node::OUTPUTPORT_RESULT, BlendTreeBlend2Node::INPUTPORT_WEIGHT);
            finalNode->AddConnection(blend2Node, BlendTreeBlend2Node::PORTID_OUTPUT_POSE, BlendTreeFinalNode::PORTID_INPUT_POSE);

            m_blendTreeAnimGraph->InitAfterLoading();
        }

        template <class paramType, class inputType>
        void TestInput(const AZStd::string& paramName, std::vector<inputType> xInputs)
        {
            BlendTreeConnection* connection = m_floatMath1Node->AddConnection(m_paramNode,
                static_cast<uint16>(m_paramNode->FindOutputPortByName(paramName)->m_portId), BlendTreeFloatMath1Node::PORTID_INPUT_X);

            for (inputType i : xInputs)
            {
                // Get and set parameter value to different test data inputs
                const AZ::Outcome<size_t> parameterIndex = m_animGraphInstance->FindParameterIndex(paramName);
                MCore::Attribute* param = m_animGraphInstance->GetParameterValue(static_cast<AZ::u32>(parameterIndex.GetValue()));
                paramType* typeParam = static_cast<paramType*>(param);
                typeParam->SetValue(i);

                for (AZ::u8 j = 0; j < BlendTreeFloatMath1Node::MATHFUNCTION_NUMFUNCTIONS; j++)
                {
                    // Test input with all 26 math functions
                    const BlendTreeFloatMath1Node::EMathFunction eMathFunc = static_cast<BlendTreeFloatMath1Node::EMathFunction>(j);
                    m_floatMath1Node->SetMathFunction(eMathFunc);
                    GetEMotionFX().Update(1.0f / 60.0f);

                    const float actualOutput = m_floatMath1Node->GetOutputFloat(m_animGraphInstance,
                        BlendTreeFloatMath1Node::OUTPUTPORT_RESULT)->GetValue();
                    const float expectedOutput = CalculateMathFunctionOutput(eMathFunc, static_cast<float>(i));

                    // Special cases for random float where float equal is not suitable
                    // If actual and expected outputs are both NaN, then they should be considered same
                    if (eMathFunc == BlendTreeFloatMath1Node::MATHFUNCTION_RANDOMFLOAT)
                    {
                        EXPECT_TRUE(RandomFloatIsInRange(actualOutput, 0, static_cast<float>(i))) << "Random float is not in range.";
                        continue;
                    }
                    if (AZStd::isnan(actualOutput) && AZStd::isnan(expectedOutput))
                    {
                        continue;
                    }
                    if (AZStd::isinf(actualOutput) && AZStd::isinf(expectedOutput))
                    {
                        continue;
                    }
                    EXPECT_NEAR(actualOutput, expectedOutput, 0.004f) << "Actual and expected outputs does not match.";
                }
            }
            m_floatMath1Node->RemoveConnection(connection);
        }

        void SetUp() override
        {
            AnimGraphFixture::SetUp();
            m_animGraphInstance->Destroy();
            m_animGraphInstance = m_blendTreeAnimGraph->GetAnimGraphInstance(m_actorInstance, m_motionSet);
        }

    protected:
        BlendTree* m_blendTree = nullptr;
        BlendTreeFloatMath1Node* m_floatMath1Node = nullptr;
        BlendTreeFloatMath1NodeTestData m_param;
        BlendTreeParameterNode* m_paramNode = nullptr;
    
    private:
        bool RandomFloatIsInRange(float randomFloat, float bound1, float bound2)
        {
            if (bound1 > bound2)
            {
                return (randomFloat - bound2) <= (bound1 - bound2);
            }
            return (randomFloat - bound1) <= (bound2 - bound1);
        }

        template<class ParameterType, class ValueType>
        void AddParameter(const AZStd::string name, const ValueType& defaultValue)
        {
            ParameterType* parameter = aznew ParameterType();
            parameter->SetName(name);
            parameter->SetDefaultValue(defaultValue);
            m_blendTreeAnimGraph->AddParameter(parameter);
        }

        float CalculateMathFunctionOutput(BlendTreeFloatMath1Node::EMathFunction mathFunction, float input)
        {
            switch (mathFunction)
            {
            case BlendTreeFloatMath1Node::MATHFUNCTION_SIN:
                return CalculateSin(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_COS:
                return CalculateCos(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_TAN:
                return CalculateTan(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_SQR:
                return CalculateSqr(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_SQRT:
                return CalculateSqrt(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_ABS:
                return CalculateAbs(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_FLOOR:
                return CalculateFloor(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_CEIL:
                return CalculateCeil(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_ONEOVERINPUT:
                return CalculateOneOverInput(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_INVSQRT:
                return CalculateInvSqrt(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_LOG:
                return CalculateLog(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_LOG10:
                return CalculateLog10(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_EXP:
                return CalculateExp(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_FRACTION:
                return CalculateFraction(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_SIGN:
                return CalculateSign(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_ISPOSITIVE:
                return CalculateIsPositive(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_ISNEGATIVE:
                return CalculateIsNegative(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_ISNEARZERO:
                return CalculateIsNearZero(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_RANDOMFLOAT:
                return 0.0f;
            case BlendTreeFloatMath1Node::MATHFUNCTION_RADTODEG:
                return CalculateRadToDeg(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_DEGTORAD:
                return CalculateDegToRad(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_SMOOTHSTEP:
                return CalculateSmoothStep(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_ACOS:
                return CalculateACos(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_ASIN:
                return CalculateASin(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_ATAN:
                return CalculateATan(input);
            case BlendTreeFloatMath1Node::MATHFUNCTION_NEGATE:
                return CalculateNegate(input);
            default:
                AZ_Assert(false, "EMotionFX: Math function unknown.");
                return 0.0f;
            }
        }

        //-----------------------------------------------
        // The math functions
        //-----------------------------------------------
        float CalculateSin(float input) { return sin(input); }
        float CalculateCos(float input) { return cos(input); }
        float CalculateTan(float input) { return tan(input); }
        float CalculateSqr(float input) { return (input * input); }
        float CalculateSqrt(float input) 
        { 
            if (input > AZ::Constants::FloatEpsilon)
            {
                return sqrt(input);
            }
            return 0.0f;
        }
        float CalculateAbs(float input) { return abs(input); }
        float CalculateFloor(float input) { return floor(input); }
        float CalculateCeil(float input) { return ceil(input); }
        float CalculateOneOverInput(float input)
        {
            if (input > AZ::Constants::FloatEpsilon)
            {
                return 1.0f / input;
            }
            return 0.0f;
        }
        float CalculateInvSqrt(float input)
        {
            if (input > AZ::Constants::FloatEpsilon)
            {
                return 1.0f / sqrt(input);
            }
            return 0.0f;
        }
        float CalculateLog(float input)
        {
            if (input > AZ::Constants::FloatEpsilon)
            {
                return log(input);
            }
            return 0.0f;
        }
        float CalculateLog10(float input)
        {
            if (input > AZ::Constants::FloatEpsilon)
            {
                return log10f(input);
            }
            return 0.0f;
        }
        float CalculateExp(float input) { return exp(input); }
        float CalculateFraction(float input) { return AZ::GetMod(input, 1.0f); }
        float CalculateSign(float input)
        {
            if (input < 0.0f)
            {
                return -1.0f;
            }
            if (input > 0.0f)
            {
                return 1.0f;
            }
            return 0.0f;
        }
        float CalculateIsPositive(float input)
        {
            if (input >= 0.0f)
            {
                return 1.0f;
            }
            return 0.0f;
        }
        float CalculateIsNegative(float input)
        {
            if (input < 0.0f)
            {
                return 1.0f;
            }
            return 0.0f;
        }
        float CalculateIsNearZero(float input)
        {
            if ((input > -AZ::Constants::FloatEpsilon) && (input < AZ::Constants::FloatEpsilon))
            {
                return 1.0f;
            }
            return 0.0f;
        }
        float CalculateRadToDeg(float input) { return AZ::RadToDeg(input); }
        float CalculateDegToRad(float input) { return AZ::DegToRad(input); }
        float CalculateSmoothStep(float input) 
        { 
            const float f = AZ::GetClamp<float>(input, 0.0f, 1.0f); 
            const float weight = (1.0f - cos(f * AZ::Constants::Pi)) * 0.5f;;
            return 0.0f * (1.0f - weight) + (weight * 1.0f);
        }
        float CalculateACos(float input) { return acos(input); }
        float CalculateASin(float input) { return asin(input); }
        float CalculateATan(float input) { return atan(input); }
        float CalculateNegate(float input) { return -input; }
    };

    TEST_P(BlendTreeFloatMath1NodeFixture, NoInput_OutputsCorrectFloatTest)
    {
        // Testing float math1 node without input node
        for (AZ::u8 i = 0; i < BlendTreeFloatMath1Node::MATHFUNCTION_NUMFUNCTIONS; i++)
        {
            BlendTreeFloatMath1Node::EMathFunction eMathFunc = static_cast<BlendTreeFloatMath1Node::EMathFunction>(i);
            m_floatMath1Node->SetMathFunction(eMathFunc);
            GetEMotionFX().Update(1.0f / 60.0f);

            // Default output should be 0.0f
            EXPECT_FLOAT_EQ(m_floatMath1Node->GetOutputFloat(m_animGraphInstance,
                BlendTreeFloatMath1Node::OUTPUTPORT_RESULT)->GetValue(), 0.0f) << "Expected Output: 0.0f";
        }
    };

#if AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_TESTS
    TEST_P(BlendTreeFloatMath1NodeFixture, DISABLED_FloatInput_OutputsCorrectFloatTest)
#else
    TEST_P(BlendTreeFloatMath1NodeFixture, FloatInput_OutputsCorrectFloatTest)
#endif // AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_TESTS
    {
        TestInput<MCore::AttributeFloat, float>("FloatParam", m_param.m_xInputFloat);
    };

#if AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_TESTS
    TEST_P(BlendTreeFloatMath1NodeFixture, DISABLED_IntInput_OutputsCorrectFloatTest)
#else
    TEST_P(BlendTreeFloatMath1NodeFixture, IntInput_OutputsCorrectFloatTest)
#endif // AZ_TRAIT_DISABLE_FAILED_EMOTION_FX_TESTS
    {
        TestInput<MCore::AttributeInt32, int>("IntParam", m_param.m_xInputInt);
    };

    TEST_P(BlendTreeFloatMath1NodeFixture, BoolInput_OutputsCorrectFloatTest)
    {
        TestInput<MCore::AttributeBool, bool>("BoolParam", m_param.m_xInputBool);
    };

    INSTANTIATE_TEST_CASE_P(BlendTreeFloatMath1Node_ValidOutputTests,
        BlendTreeFloatMath1NodeFixture,
            ::testing::ValuesIn(blendTreeFloatMath1NodeTestData)
    );
} // end namespace EMotionFX
