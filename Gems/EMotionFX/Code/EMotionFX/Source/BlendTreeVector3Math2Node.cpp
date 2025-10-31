/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include "BlendTreeVector3Math2Node.h"
#include <MCore/Source/FastMath.h>
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeVector3Math2Node, AnimGraphAllocator)


    BlendTreeVector3Math2Node::BlendTreeVector3Math2Node()
        : AnimGraphNode()
        , m_mathFunction(MATHFUNCTION_DOT)
        , m_defaultValue(AZ::Vector3::CreateZero())
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPortAsVector3("x", INPUTPORT_X, PORTID_INPUT_X);
        SetupInputPortAsVector3("y", INPUTPORT_Y, PORTID_INPUT_Y);

        // setup the output ports
        InitOutputPorts(2);
        SetupOutputPort("Vector3", OUTPUTPORT_RESULT_VECTOR3, MCore::AttributeVector3::TYPE_ID, PORTID_OUTPUT_VECTOR3);
        SetupOutputPort("Float", OUTPUTPORT_RESULT_FLOAT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_FLOAT);

        if (m_animGraph)
        {
            Reinit();
        }
    }

    
    BlendTreeVector3Math2Node::~BlendTreeVector3Math2Node()
    {
    }


    void BlendTreeVector3Math2Node::Reinit()
    {
        switch (m_mathFunction)
        {
        case MATHFUNCTION_DOT:
            m_calculateFunc = CalculateDot;
            SetNodeInfo("dot(x, y)");
            break;
        case MATHFUNCTION_CROSS:
            m_calculateFunc = CalculateCross;
            SetNodeInfo("cross(x, y)");
            break;
        case MATHFUNCTION_ADD:
            m_calculateFunc = CalculateAdd;
            SetNodeInfo("x + y");
            break;
        case MATHFUNCTION_SUBTRACT:
            m_calculateFunc = CalculateSubtract;
            SetNodeInfo("x - y");
            break;
        case MATHFUNCTION_MULTIPLY:
            m_calculateFunc = CalculateMultiply;
            SetNodeInfo("x * y");
            break;
        case MATHFUNCTION_DIVIDE:
            m_calculateFunc = CalculateDivide;
            SetNodeInfo("x / y");
            break;
        case MATHFUNCTION_ANGLEDEGREES:
            m_calculateFunc = CalculateAngleDegrees;
            SetNodeInfo("Angle Degr.");
            break;
        default:
            AZ_Assert(false, "EMotionFX: Math function unknown.");
        }

        AnimGraphNode::Reinit();
    }


    bool BlendTreeVector3Math2Node::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphNode::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    const char* BlendTreeVector3Math2Node::GetPaletteName() const
    {
        return "Vector3 Math2";
    }


    AnimGraphObject::ECategory BlendTreeVector3Math2Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // the update function
    void BlendTreeVector3Math2Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        if (m_connections.empty())
        {
            return;
        }

        // if both x and y inputs have connections
        AZ::Vector3 x, y;
        if (!TryGetInputVector3(animGraphInstance, INPUTPORT_X, x))
        {
            x = m_defaultValue;
        }
        if (!TryGetInputVector3(animGraphInstance, INPUTPORT_Y, y))
        {
            y = m_defaultValue;
        }

        // apply the operation
        AZ::Vector3 vectorResult(0.0f, 0.0f, 0.0f);
        float floatResult = 0.0f;
        m_calculateFunc(x, y, &vectorResult, &floatResult);

        // update the output value
        GetOutputVector3(animGraphInstance, OUTPUTPORT_RESULT_VECTOR3)->SetValue(vectorResult);
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT_FLOAT)->SetValue(floatResult);
    }

    void BlendTreeVector3Math2Node::SetMathFunction(EMathFunction func)
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
    // dot product
    void BlendTreeVector3Math2Node::CalculateDot(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(vectorOutput);
        *floatOutput = inputX.Dot(inputY);
    }


    // cross product
    void BlendTreeVector3Math2Node::CalculateCross(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        *vectorOutput = inputX.Cross(inputY);
    }


    // add
    void BlendTreeVector3Math2Node::CalculateAdd(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        *vectorOutput = inputX + inputY;
    }


    // subtract
    void BlendTreeVector3Math2Node::CalculateSubtract(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        *vectorOutput = inputX - inputY;
    }


    // multiply
    void BlendTreeVector3Math2Node::CalculateMultiply(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        *vectorOutput = inputX * inputY;
    }


    // divide
    void BlendTreeVector3Math2Node::CalculateDivide(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(floatOutput);
        vectorOutput->Set(
            AZ::IsClose(inputY.GetX(), 0.0f, MCore::Math::epsilon) == false ? (static_cast<float>(inputX.GetX()) / static_cast<float>(inputY.GetX())) : 0.0f,
            AZ::IsClose(inputY.GetY(), 0.0f, MCore::Math::epsilon) == false ? (static_cast<float>(inputX.GetY()) / static_cast<float>(inputY.GetY())) : 0.0f,
            AZ::IsClose(inputY.GetZ(), 0.0f, MCore::Math::epsilon) == false ? (static_cast<float>(inputX.GetZ()) / static_cast<float>(inputY.GetZ())) : 0.0f);
    }


    // calculate the angle
    void BlendTreeVector3Math2Node::CalculateAngleDegrees(const AZ::Vector3& inputX, const AZ::Vector3& inputY, AZ::Vector3* vectorOutput, float* floatOutput)
    {
        MCORE_UNUSED(vectorOutput);
        const float radians = MCore::Math::ACos(MCore::SafeNormalize(inputX).Dot(MCore::SafeNormalize(inputY)));
        *floatOutput = MCore::Math::RadiansToDegrees(radians);
    }

    void BlendTreeVector3Math2Node::SetDefaultValue(const AZ::Vector3& value)
    {
        m_defaultValue = value;
    }

    void BlendTreeVector3Math2Node::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeVector3Math2Node, AnimGraphNode>()
            ->Version(1)
            ->Field("mathFunction", &BlendTreeVector3Math2Node::m_mathFunction)
            ->Field("defaultValue", &BlendTreeVector3Math2Node::m_defaultValue)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeVector3Math2Node>("Vector3 Math2", "Vector3 Math2 attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeVector3Math2Node::m_mathFunction, "Math Function", "The math function to use.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeVector3Math2Node::Reinit)
                ->EnumAttribute(MATHFUNCTION_DOT,          "Dot Product")
                ->EnumAttribute(MATHFUNCTION_CROSS,        "Cross Product")
                ->EnumAttribute(MATHFUNCTION_ADD,          "Add")
                ->EnumAttribute(MATHFUNCTION_SUBTRACT,     "Subtract")
                ->EnumAttribute(MATHFUNCTION_MULTIPLY,     "Multiply")
                ->EnumAttribute(MATHFUNCTION_DIVIDE,       "Divide")
                ->EnumAttribute(MATHFUNCTION_ANGLEDEGREES, "AngleDegrees")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeVector3Math2Node::m_defaultValue, "Default Value", "The default value for x or y when one of them has no incomming connection.")
            ;
    }
} // namespace EMotionFX
