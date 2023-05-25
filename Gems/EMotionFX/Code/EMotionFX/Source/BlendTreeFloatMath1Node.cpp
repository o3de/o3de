/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "EMotionFXConfig.h"
#include "BlendTreeFloatMath1Node.h"
#include <MCore/Source/Random.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFloatMath1Node, AnimGraphAllocator)

    BlendTreeFloatMath1Node::BlendTreeFloatMath1Node()
        : AnimGraphNode()
        , m_mathFunction(MATHFUNCTION_SIN)
    {
        // setup the input ports
        InitInputPorts(1);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X); // accept float/int/bool values

                                                                  // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);

        if (m_animGraph)
        {
            Reinit();
        }
    }


    BlendTreeFloatMath1Node::~BlendTreeFloatMath1Node()
    {
    }


    void BlendTreeFloatMath1Node::Reinit()
    {
        switch (m_mathFunction)
        {
        case MATHFUNCTION_SIN:
            m_calculateFunc = CalculateSin;
            SetNodeInfo("Sin");
            break;
        case MATHFUNCTION_COS:
            m_calculateFunc = CalculateCos;
            SetNodeInfo("Cos");
            break;
        case MATHFUNCTION_TAN:
            m_calculateFunc = CalculateTan;
            SetNodeInfo("Tan");
            break;
        case MATHFUNCTION_SQR:
            m_calculateFunc = CalculateSqr;
            SetNodeInfo("Square");
            break;
        case MATHFUNCTION_SQRT:
            m_calculateFunc = CalculateSqrt;
            SetNodeInfo("Sqrt");
            break;
        case MATHFUNCTION_ABS:
            m_calculateFunc = CalculateAbs;
            SetNodeInfo("Abs");
            break;
        case MATHFUNCTION_FLOOR:
            m_calculateFunc = CalculateFloor;
            SetNodeInfo("Floor");
            break;
        case MATHFUNCTION_CEIL:
            m_calculateFunc = CalculateCeil;
            SetNodeInfo("Ceil");
            break;
        case MATHFUNCTION_ONEOVERINPUT:
            m_calculateFunc = CalculateOneOverInput;
            SetNodeInfo("1/x");
            break;
        case MATHFUNCTION_INVSQRT:
            m_calculateFunc = CalculateInvSqrt;
            SetNodeInfo("1.0/sqrt(x)");
            break;
        case MATHFUNCTION_LOG:
            m_calculateFunc = CalculateLog;
            SetNodeInfo("Log");
            break;
        case MATHFUNCTION_LOG10:
            m_calculateFunc = CalculateLog10;
            SetNodeInfo("Log10");
            break;
        case MATHFUNCTION_EXP:
            m_calculateFunc = CalculateExp;
            SetNodeInfo("Exponent");
            break;
        case MATHFUNCTION_FRACTION:
            m_calculateFunc = CalculateFraction;
            SetNodeInfo("Fraction");
            break;
        case MATHFUNCTION_SIGN:
            m_calculateFunc = CalculateSign;
            SetNodeInfo("Sign");
            break;
        case MATHFUNCTION_ISPOSITIVE:
            m_calculateFunc = CalculateIsPositive;
            SetNodeInfo("Is Positive");
            break;
        case MATHFUNCTION_ISNEGATIVE:
            m_calculateFunc = CalculateIsNegative;
            SetNodeInfo("Is Negative");
            break;
        case MATHFUNCTION_ISNEARZERO:
            m_calculateFunc = CalculateIsNearZero;
            SetNodeInfo("Is Near Zero");
            break;
        case MATHFUNCTION_RANDOMFLOAT:
            m_calculateFunc = CalculateRandomFloat;
            SetNodeInfo("Random Float");
            break;
        case MATHFUNCTION_RADTODEG:
            m_calculateFunc = CalculateRadToDeg;
            SetNodeInfo("RadToDeg");
            break;
        case MATHFUNCTION_DEGTORAD:
            m_calculateFunc = CalculateDegToRad;
            SetNodeInfo("DegToRad");
            break;
        case MATHFUNCTION_SMOOTHSTEP:
            m_calculateFunc = CalculateSmoothStep;
            SetNodeInfo("SmoothStep");
            break;
        case MATHFUNCTION_ACOS:
            m_calculateFunc = CalculateACos;
            SetNodeInfo("Arc Cos");
            break;
        case MATHFUNCTION_ASIN:
            m_calculateFunc = CalculateASin;
            SetNodeInfo("Arc Sin");
            break;
        case MATHFUNCTION_ATAN:
            m_calculateFunc = CalculateATan;
            SetNodeInfo("Arc Tan");
            break;
        case MATHFUNCTION_NEGATE:
            m_calculateFunc = CalculateNegate;
            SetNodeInfo("Negate");
            break;
        default:
            AZ_Assert(false, "EMotionFX: Math function unknown.");
        }

        AnimGraphNode::Reinit();
    }


    bool BlendTreeFloatMath1Node::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* BlendTreeFloatMath1Node::GetPaletteName() const
    {
        return "Float Math1";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFloatMath1Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // the update function
    void BlendTreeFloatMath1Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // If there are no incoming connections, there is nothing to do.
        if (m_connections.empty())
        {
            return;
        }
        // Pass the input value as output in case we are disabled and have connected inputs.
        else if (m_disabled)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
            GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X));
            return;
        }

        // get the input value as a float, convert if needed
        OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
        const float x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);

        // apply the operation
        const float result = m_calculateFunc(x);

        // update the output value
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(result);
    }


    void BlendTreeFloatMath1Node::SetMathFunction(EMathFunction func)
    {
        m_mathFunction = func;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    AZ::Color BlendTreeFloatMath1Node::GetVisualColor() const
    {
        return AZ::Color(0.5f, 1.0f, 1.0f, 1.0f);
    }


    bool BlendTreeFloatMath1Node::GetSupportsDisable() const
    {
        return true;
    }


    //-----------------------------------------------
    // the math functions
    //-----------------------------------------------
    float BlendTreeFloatMath1Node::CalculateSin(float input)                { return MCore::Math::Sin(input); }
    float BlendTreeFloatMath1Node::CalculateCos(float input)                { return MCore::Math::Cos(input); }
    float BlendTreeFloatMath1Node::CalculateTan(float input)                { return MCore::Math::Tan(input); }
    float BlendTreeFloatMath1Node::CalculateSqr(float input)                { return (input * input); }
    float BlendTreeFloatMath1Node::CalculateSqrt(float input)               { return MCore::Math::SafeSqrt(input); }
    float BlendTreeFloatMath1Node::CalculateAbs(float input)                { return MCore::Math::Abs(input); }
    float BlendTreeFloatMath1Node::CalculateFloor(float input)              { return MCore::Math::Floor(input); }
    float BlendTreeFloatMath1Node::CalculateCeil(float input)               { return MCore::Math::Ceil(input); }
    float BlendTreeFloatMath1Node::CalculateOneOverInput(float input)
    {
        if (input > MCore::Math::epsilon)
        {
            return 1.0f / input;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateInvSqrt(float input)
    {
        if (input > MCore::Math::epsilon)
        {
            return MCore::Math::InvSqrt(input);
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateLog(float input)
    {
        if (input > MCore::Math::epsilon)
        {
            return MCore::Math::Log(input);
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateLog10(float input)
    {
        if (input > MCore::Math::epsilon)
        {
            return log10f(input);
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateExp(float input)                { return MCore::Math::Exp(input); }
    float BlendTreeFloatMath1Node::CalculateFraction(float input)           { return MCore::Math::FMod(input, 1.0f); }
    float BlendTreeFloatMath1Node::CalculateSign(float input)
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
    float BlendTreeFloatMath1Node::CalculateIsPositive(float input)
    {
        if (input >= 0.0f)
        {
            return 1.0f;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateIsNegative(float input)
    {
        if (input < 0.0f)
        {
            return 1.0f;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateIsNearZero(float input)
    {
        if ((input > -MCore::Math::epsilon) && (input < MCore::Math::epsilon))
        {
            return 1.0f;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath1Node::CalculateRandomFloat(float input)        { return MCore::Random::RandF(0.0f, input);  }
    float BlendTreeFloatMath1Node::CalculateRadToDeg(float input)           { return MCore::Math::RadiansToDegrees(input); }
    float BlendTreeFloatMath1Node::CalculateDegToRad(float input)           { return MCore::Math::DegreesToRadians(input); }
    float BlendTreeFloatMath1Node::CalculateSmoothStep(float input)         { const float f = MCore::Clamp<float>(input, 0.0f, 1.0f); return MCore::CosineInterpolate<float>(0.0f, 1.0f, f); }
    float BlendTreeFloatMath1Node::CalculateACos(float input)               { return MCore::Math::ACos(input); }
    float BlendTreeFloatMath1Node::CalculateASin(float input)               { return MCore::Math::ASin(input); }
    float BlendTreeFloatMath1Node::CalculateATan(float input)               { return MCore::Math::ATan(input); }
    float BlendTreeFloatMath1Node::CalculateNegate(float input)             { return -input; }


    void BlendTreeFloatMath1Node::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeFloatMath1Node, AnimGraphNode>()
            ->Version(1)
            ->Field("mathFunction", &BlendTreeFloatMath1Node::m_mathFunction)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeFloatMath1Node>("Float Math1", "Float Math1 attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeFloatMath1Node::m_mathFunction, "Math Function", "The math function to use.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFloatMath1Node::Reinit)
                ->EnumAttribute(MATHFUNCTION_SIN,          "Sine")
                ->EnumAttribute(MATHFUNCTION_COS,          "Cosine")
                ->EnumAttribute(MATHFUNCTION_TAN,          "Tan")
                ->EnumAttribute(MATHFUNCTION_SQR,          "Square")
                ->EnumAttribute(MATHFUNCTION_SQRT,         "Square Root")
                ->EnumAttribute(MATHFUNCTION_ABS,          "Absolute")
                ->EnumAttribute(MATHFUNCTION_FLOOR,        "Floor")
                ->EnumAttribute(MATHFUNCTION_CEIL,         "Ceil")
                ->EnumAttribute(MATHFUNCTION_ONEOVERINPUT, "One Over X")
                ->EnumAttribute(MATHFUNCTION_INVSQRT,      "Inverse Square Root")
                ->EnumAttribute(MATHFUNCTION_LOG,          "Natural Log")
                ->EnumAttribute(MATHFUNCTION_LOG10,        "Log Base 10")
                ->EnumAttribute(MATHFUNCTION_EXP,          "Exponent")
                ->EnumAttribute(MATHFUNCTION_FRACTION,     "Fraction")
                ->EnumAttribute(MATHFUNCTION_SIGN,         "Sign")
                ->EnumAttribute(MATHFUNCTION_ISPOSITIVE,   "Is Positive")
                ->EnumAttribute(MATHFUNCTION_ISNEGATIVE,   "Is Negative")
                ->EnumAttribute(MATHFUNCTION_ISNEARZERO,   "Is Near Zero")
                ->EnumAttribute(MATHFUNCTION_RANDOMFLOAT,  "Random Float")
                ->EnumAttribute(MATHFUNCTION_RADTODEG,     "Radians to Degrees")
                ->EnumAttribute(MATHFUNCTION_DEGTORAD,     "Degrees to Radians")
                ->EnumAttribute(MATHFUNCTION_SMOOTHSTEP,   "Smooth Step [0..1]")
                ->EnumAttribute(MATHFUNCTION_ACOS,         "Arc Cosine")
                ->EnumAttribute(MATHFUNCTION_ASIN,         "Arc Sine")
                ->EnumAttribute(MATHFUNCTION_ATAN,         "Arc Tan")
                ->EnumAttribute(MATHFUNCTION_NEGATE,       "Negate")
            ;
    }
} // namespace EMotionFX
