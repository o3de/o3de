/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeVector3Math1Node.h"
#include <MCore/Source/Random.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeVector3Math1Node, AnimGraphAllocator)


    BlendTreeVector3Math1Node::BlendTreeVector3Math1Node()
        : AnimGraphNode()
        , m_mathFunction(MATHFUNCTION_LENGTH)
    {
        // create the input ports
        InitInputPorts(1);
        SetupInputPortAsVector3("x", INPUTPORT_X, PORTID_INPUT_X);

        // create the output ports
        InitOutputPorts(2);
        SetupOutputPort("Vector3", OUTPUTPORT_RESULT_VECTOR3, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_VECTOR3);
        SetupOutputPort("Float", OUTPUTPORT_RESULT_FLOAT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_FLOAT);

        if (m_animGraph)
        {
            Reinit();
        }
    }
    

    BlendTreeVector3Math1Node::~BlendTreeVector3Math1Node()
    {
    }


    void BlendTreeVector3Math1Node::Reinit()
    {
        switch (m_mathFunction)
        {
        case MATHFUNCTION_LENGTH:
            m_calculateFunc = CalculateLength;
            SetNodeInfo("Length");
            break;
        case MATHFUNCTION_SQUARELENGTH:
            m_calculateFunc = CalculateSquareLength;
            SetNodeInfo("Square Length");
            break;
        case MATHFUNCTION_NORMALIZE:
            m_calculateFunc = CalculateNormalize;
            SetNodeInfo("Normalize");
            break;
        case MATHFUNCTION_ZERO:
            m_calculateFunc = CalculateZero;
            SetNodeInfo("Zero");
            break;
        case MATHFUNCTION_FLOOR:
            m_calculateFunc = CalculateFloor;
            SetNodeInfo("Floor");
            break;
        case MATHFUNCTION_CEIL:
            m_calculateFunc = CalculateCeil;
            SetNodeInfo("Ceil");
            break;
        case MATHFUNCTION_ABS:
            m_calculateFunc = CalculateAbs;
            SetNodeInfo("Abs");
            break;
        case MATHFUNCTION_NEGATE:
            m_calculateFunc = CalculateNegate;
            SetNodeInfo("Negate");
            break;
        case MATHFUNCTION_RANDOM:
            m_calculateFunc = CalculateRandomVector;
            SetNodeInfo("Random[0..1]");
            break;
        case MATHFUNCTION_RANDOMNEG:
            m_calculateFunc = CalculateRandomVectorNeg;
            SetNodeInfo("Random[-1..1]");
            break;
        case MATHFUNCTION_RANDOMDIRVEC:
            m_calculateFunc = CalculateRandomVectorDir;
            SetNodeInfo("RandomDirection");
            break;
        default:
            AZ_Assert(false, "EMotionFX: Math function unknown.");
        }

        AnimGraphNode::Reinit();
    }


    bool BlendTreeVector3Math1Node::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeVector3Math1Node::GetPaletteName() const
    {
        return "Vector3 Math1";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeVector3Math1Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // the update function
    void BlendTreeVector3Math1Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // get the input value as a float, convert if needed
        AZ::Vector3 input;
        if (!TryGetInputVector3(animGraphInstance, INPUTPORT_X, input))
        {
            input = AZ::Vector3::CreateZero();
        }

        // apply the operation
        AZ::Vector3 vectorResult(0.0f, 0.0f, 0.0f);
        float floatResult = 0.0f;
        m_calculateFunc(input, &vectorResult, &floatResult);

        // update the output value
        GetOutputVector3(animGraphInstance, OUTPUTPORT_RESULT_VECTOR3)->SetValue(vectorResult);
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT_FLOAT)->SetValue(floatResult);
    }


    void BlendTreeVector3Math1Node::SetMathFunction(EMathFunction func)
    {
        m_mathFunction = func;
        if (m_animGraph)
        {
            Reinit();
        }
    }

    //-----------------------------------------------
    // the math functions
    //-----------------------------------------------
    void BlendTreeVector3Math1Node::CalculateLength(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)          { MCORE_UNUSED(vectorOutput); *floatOutput = MCore::SafeLength(input); }
    void BlendTreeVector3Math1Node::CalculateSquareLength(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)    { MCORE_UNUSED(vectorOutput); *floatOutput = input.GetLengthSq(); }
    void BlendTreeVector3Math1Node::CalculateNormalize(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)       { MCORE_UNUSED(floatOutput); *vectorOutput = input.GetNormalized(); }
    void BlendTreeVector3Math1Node::CalculateZero(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)            { MCORE_UNUSED(floatOutput); *vectorOutput = input; *vectorOutput = AZ::Vector3::CreateZero(); }
    void BlendTreeVector3Math1Node::CalculateAbs(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)             { MCORE_UNUSED(floatOutput); vectorOutput->Set(MCore::Math::Abs(input.GetX()), MCore::Math::Abs(input.GetY()), MCore::Math::Abs(input.GetZ())); }
    void BlendTreeVector3Math1Node::CalculateFloor(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)           { MCORE_UNUSED(floatOutput); vectorOutput->Set(MCore::Math::Floor(input.GetX()), MCore::Math::Floor(input.GetY()), MCore::Math::Floor(input.GetZ())); }
    void BlendTreeVector3Math1Node::CalculateCeil(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)            { MCORE_UNUSED(floatOutput); vectorOutput->Set(MCore::Math::Ceil(input.GetX()), MCore::Math::Ceil(input.GetY()), MCore::Math::Ceil(input.GetZ())); }
    void BlendTreeVector3Math1Node::CalculateRandomVector(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)    { MCORE_UNUSED(floatOutput); MCORE_UNUSED(input); vectorOutput->Set(MCore::Random::RandF(0.0f, 1.0f), MCore::Random::RandF(0.0f, 1.0f), MCore::Random::RandF(0.0f, 1.0f)); }
    void BlendTreeVector3Math1Node::CalculateRandomVectorNeg(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput) { MCORE_UNUSED(floatOutput); MCORE_UNUSED(input); *vectorOutput = MCore::Random::RandomVecF(); }
    void BlendTreeVector3Math1Node::CalculateRandomVectorDir(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput) { MCORE_UNUSED(floatOutput); MCORE_UNUSED(input); *vectorOutput = MCore::Random::RandDirVecF(); }
    void BlendTreeVector3Math1Node::CalculateNegate(const AZ::Vector3& input, AZ::Vector3* vectorOutput, float* floatOutput)          { MCORE_UNUSED(floatOutput); vectorOutput->Set(-input.GetX(), -input.GetY(), -input.GetZ()); }


    void BlendTreeVector3Math1Node::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeVector3Math1Node, AnimGraphNode>()
            ->Version(1)
            ->Field("mathFunction", &BlendTreeVector3Math1Node::m_mathFunction)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeVector3Math1Node>("Vector3 Math1", "Vector3 Math1 attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeVector3Math1Node::m_mathFunction, "Math Function", "The math function to use.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeVector3Math1Node::Reinit)
                ->EnumAttribute(MATHFUNCTION_LENGTH,       "Length")
                ->EnumAttribute(MATHFUNCTION_SQUARELENGTH, "Square Length")
                ->EnumAttribute(MATHFUNCTION_NORMALIZE,    "Normalize")
                ->EnumAttribute(MATHFUNCTION_ZERO,         "Zero")
                ->EnumAttribute(MATHFUNCTION_FLOOR,        "Floor")
                ->EnumAttribute(MATHFUNCTION_CEIL,         "Ceil")
                ->EnumAttribute(MATHFUNCTION_ABS,          "Abs")
                ->EnumAttribute(MATHFUNCTION_RANDOM,       "Random Vector [0..1]")
                ->EnumAttribute(MATHFUNCTION_RANDOMNEG,    "Random Vector [-1..1]")
                ->EnumAttribute(MATHFUNCTION_RANDOMDIRVEC, "Random Direction")
                ->EnumAttribute(MATHFUNCTION_NEGATE,       "Negate")
            ;
    }
} // namespace EMotionFX
