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
#include "BlendTreeFloatMath2Node.h"
#include <MCore/Source/Random.h>
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFloatMath2Node, AnimGraphAllocator)

    BlendTreeFloatMath2Node::BlendTreeFloatMath2Node()
        : AnimGraphNode()
        , m_mathFunction(MATHFUNCTION_ADD)
        , m_defaultValue(0.0f)
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X); // accept float/int/bool values
        SetupInputPortAsNumber("y", INPUTPORT_Y, PORTID_INPUT_Y); // accept float/int/bool values

        // setup the output ports
        InitOutputPorts(1);
        SetupOutputPort("Result", OUTPUTPORT_RESULT, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_RESULT);

        if (m_animGraph)
        {
            Reinit();
        }
    }
    

    BlendTreeFloatMath2Node::~BlendTreeFloatMath2Node()
    {
    }


    void BlendTreeFloatMath2Node::Reinit()
    {
        switch (m_mathFunction)
        {
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
        case MATHFUNCTION_AVERAGE:
            m_calculateFunc = CalculateAverage;
            SetNodeInfo("Average");
            break;
        case MATHFUNCTION_RANDOMFLOAT:
            m_calculateFunc = CalculateRandomFloat;
            SetNodeInfo("Random[x..y]");
            break;
        case MATHFUNCTION_MOD:
            m_calculateFunc = CalculateMod;
            SetNodeInfo("x MOD y");
            break;
        case MATHFUNCTION_MIN:
            m_calculateFunc = CalculateMin;
            SetNodeInfo("Min(x, y)");
            break;
        case MATHFUNCTION_MAX:
            m_calculateFunc = CalculateMax;
            SetNodeInfo("Max(x, y)");
            break;
        case MATHFUNCTION_POW:
            m_calculateFunc = CalculatePow;
            SetNodeInfo("Pow(x, y)");
            break;
        default:
            AZ_Assert(false, "EMotionFX: Math function unknown.");
        }

        AnimGraphNode::Reinit();
    }


    bool BlendTreeFloatMath2Node::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeFloatMath2Node::GetPaletteName() const
    {
        return "Float Math2";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFloatMath2Node::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_MATH;
    }


    // the update function
    void BlendTreeFloatMath2Node::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        if (m_connections.empty())
        {
            return;
        }

        // if both x and y inputs have connections
        float x, y;
        if (m_connections.size() == 2)
        {
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
            OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_Y));

            x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);
            y = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Y);
        }
        else // only x or y is connected
        {
            // if only x has something plugged in
            if (m_connections[0]->GetTargetPort() == INPUTPORT_X)
            {
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_X));
                x = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_X);
                y = m_defaultValue;
            }
            else // only y has an input
            {
                MCORE_ASSERT(m_connections[0]->GetTargetPort() == INPUTPORT_Y);
                x = m_defaultValue;
                OutputIncomingNode(animGraphInstance, GetInputNode(INPUTPORT_Y));
                y = GetInputNumberAsFloat(animGraphInstance, INPUTPORT_Y);
            }
        }

        // apply the operation
        const float result = m_calculateFunc(x, y);

        // update the output value
        GetOutputFloat(animGraphInstance, OUTPUTPORT_RESULT)->SetValue(result);

        //if (rand() % 50 == 0)
        //MCore::LogDebug("func(%.4f, %.4f) = %.4f", x, y, result);
    }


    void BlendTreeFloatMath2Node::SetMathFunction(EMathFunction func)
    {
        m_mathFunction = func;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    AZ::Color BlendTreeFloatMath2Node::GetVisualColor() const
    {
        return AZ::Color(0.5f, 1.0f, 0.5f, 1.0f);
    }


    //-----------------------------------------------
    // the math functions
    //-----------------------------------------------
    float BlendTreeFloatMath2Node::CalculateAdd(float x, float y)               { return x + y; }
    float BlendTreeFloatMath2Node::CalculateSubtract(float x, float y)          { return x - y; }
    float BlendTreeFloatMath2Node::CalculateMultiply(float x, float y)          { return x * y; }
    float BlendTreeFloatMath2Node::CalculateDivide(float x, float y)
    {
        if (MCore::Compare<float>::CheckIfIsClose(y, 0.0f, MCore::Math::epsilon) == false)
        {
            return x / y;
        }
        return 0.0f;
    }
    float BlendTreeFloatMath2Node::CalculateAverage(float x, float y)           { return (x + y) * 0.5f; }
    float BlendTreeFloatMath2Node::CalculateRandomFloat(float x, float y)       { return MCore::Random::RandF(x, y); }
    float BlendTreeFloatMath2Node::CalculateMod(float x, float y)
    {
        if (MCore::Compare<float>::CheckIfIsClose(y, 0.0f, MCore::Math::epsilon) == false)
        {
            return MCore::Math::FMod(x, y);
        }
        return 0.0f;
    }
    float BlendTreeFloatMath2Node::CalculateMin(float x, float y)               { return MCore::Min<float>(x, y); }
    float BlendTreeFloatMath2Node::CalculateMax(float x, float y)               { return MCore::Max<float>(x, y); }
    float BlendTreeFloatMath2Node::CalculatePow(float x, float y)
    {
        if (MCore::Compare<float>::CheckIfIsClose(x, 0.0f, MCore::Math::epsilon) && y < 0.0f)
        {
            return 0.0f;
        }
        return MCore::Math::Pow(x, y);
    }

    void BlendTreeFloatMath2Node::SetDefaultValue(float value)
    {
        m_defaultValue = value;
    }

    void BlendTreeFloatMath2Node::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeFloatMath2Node, AnimGraphNode>()
            ->Version(1)
            ->Field("mathFunction", &BlendTreeFloatMath2Node::m_mathFunction)
            ->Field("defaultValue", &BlendTreeFloatMath2Node::m_defaultValue)
            ;


        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeFloatMath2Node>("Float Math2", "Float Math2 attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeFloatMath2Node::m_mathFunction, "Math Function", "The math function to use.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFloatMath2Node::Reinit)
                ->EnumAttribute(MATHFUNCTION_ADD,           "Add")
                ->EnumAttribute(MATHFUNCTION_SUBTRACT,      "Subtract")
                ->EnumAttribute(MATHFUNCTION_MULTIPLY,      "Multiply")
                ->EnumAttribute(MATHFUNCTION_DIVIDE,        "Divide")
                ->EnumAttribute(MATHFUNCTION_AVERAGE,       "Average")
                ->EnumAttribute(MATHFUNCTION_RANDOMFLOAT,   "Random Float")
                ->EnumAttribute(MATHFUNCTION_MOD,           "Mod")
                ->EnumAttribute(MATHFUNCTION_MIN,           "Minimum")
                ->EnumAttribute(MATHFUNCTION_MAX,           "Maximum")
                ->EnumAttribute(MATHFUNCTION_POW,           "Power")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatMath2Node::m_defaultValue, "Default Value", "Value used for x or y when the input port has no connection.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ;
    }
} // namespace EMotionFX
