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
#include "BlendTreeFloatConditionNode.h"
#include <MCore/Source/Compare.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendTreeFloatConditionNode, AnimGraphAllocator)

    BlendTreeFloatConditionNode::BlendTreeFloatConditionNode()
        : AnimGraphNode()
        , m_functionEnum(FUNCTION_EQUAL)
        , m_defaultValue(0.0f)
        , m_trueResult(1.0f)
        , m_falseResult(0.0f)
        , m_trueReturnMode(MODE_VALUE)
        , m_falseReturnMode(MODE_VALUE)
    {
        // setup the input ports
        InitInputPorts(2);
        SetupInputPortAsNumber("x", INPUTPORT_X, PORTID_INPUT_X); // accept float/int/bool values
        SetupInputPortAsNumber("y", INPUTPORT_Y, PORTID_INPUT_Y); // accept float/int/bool values
                                                          // setup the output ports
        InitOutputPorts(2);
        SetupOutputPort("Float", OUTPUTPORT_VALUE, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_VALUE);

        SetupOutputPort("Bool", OUTPUTPORT_BOOL, MCore::AttributeFloat::TYPE_ID, PORTID_OUTPUT_BOOL);    // false on default

        if (m_animGraph)
        {
            Reinit();
        }
    }

        
    BlendTreeFloatConditionNode::~BlendTreeFloatConditionNode()
    {
    }


    void BlendTreeFloatConditionNode::Reinit()
    {
        switch (m_functionEnum)
        {
        case FUNCTION_EQUAL:
            m_function = FloatConditionEqual;
            SetNodeInfo("x == y");
            break;
        case FUNCTION_NOTEQUAL:
            m_function = FloatConditionNotEqual;
            SetNodeInfo("x != y");
            break;
        case FUNCTION_GREATER:
            m_function = FloatConditionGreater;
            SetNodeInfo("x > y");
            break;
        case FUNCTION_LESS:
            m_function = FloatConditionLess;
            SetNodeInfo("x < y");
            break;
        case FUNCTION_GREATEROREQUAL:
            m_function = FloatConditionGreaterOrEqual;
            SetNodeInfo("x >= y");
            break;
        case FUNCTION_LESSOREQUAL:
            m_function = FloatConditionLessOrEqual;
            SetNodeInfo("x <= y");
            break;
        default:
            AZ_Assert(false, "EMotionFX: Function unknown.");
        }

        AnimGraphNode::Reinit();
    }


    bool BlendTreeFloatConditionNode::InitAfterLoading(AnimGraph* animGraph)
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
    const char* BlendTreeFloatConditionNode::GetPaletteName() const
    {
        return "Float Condition";
    }


    // get the category
    AnimGraphObject::ECategory BlendTreeFloatConditionNode::GetPaletteCategory() const
    {
        return AnimGraphObject::CATEGORY_LOGIC;
    }


    // the update function
    void BlendTreeFloatConditionNode::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // update all inputs
        UpdateAllIncomingNodes(animGraphInstance, timePassedInSeconds);

        // if there are no incoming connections, there is nothing to do
        const size_t numConnections = m_connections.size();
        if (numConnections == 0)
        {
            return;
        }

        // if both x and y inputs have connections
        float x, y;
        if (numConnections == 2)
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

        // execute the condition function
        if (m_function(x, y))
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_BOOL)->SetValue(1.0f); // set to true

            switch (m_trueReturnMode)
            {
                case MODE_VALUE:
                    GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(m_trueResult);
                    break;
                case MODE_X:
                    GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(x);
                    break;
                case MODE_Y:
                    GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(y);
                    break;
                default:
                    AZ_Assert(false, "EMotionFX: Function unknown.");
            }
        }
        else
        {
            GetOutputFloat(animGraphInstance, OUTPUTPORT_BOOL)->SetValue(0.0f);

            switch (m_falseReturnMode)
            {
                case MODE_VALUE:
                    GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(m_falseResult);
                    break;
                case MODE_X:
                    GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(x);
                    break;
                case MODE_Y:
                    GetOutputFloat(animGraphInstance, OUTPUTPORT_VALUE)->SetValue(y);
                    break;
                default:
                    AZ_Assert(false, "EMotionFX: Function unknown.");
            }
        }
    }


    void BlendTreeFloatConditionNode::SetFunction(EFunction func)
    {
        m_functionEnum = func;
        if (m_animGraph)
        {
            Reinit();
        }
    }


    AZ::Color BlendTreeFloatConditionNode::GetVisualColor() const
    {
        return AZ::Color(1.0f, 0.39f, 0.2f, 1.0f);
    }


    //-----------------------------------------------
    // the condition functions
    //-----------------------------------------------
    bool BlendTreeFloatConditionNode::FloatConditionEqual(float x, float y)             { return MCore::Compare<float>::CheckIfIsClose(x, y, MCore::Math::epsilon); }
    bool BlendTreeFloatConditionNode::FloatConditionNotEqual(float x, float y)          { return (MCore::Compare<float>::CheckIfIsClose(x, y, MCore::Math::epsilon) == false); }
    bool BlendTreeFloatConditionNode::FloatConditionGreater(float x, float y)           { return (x > y); }
    bool BlendTreeFloatConditionNode::FloatConditionLess(float x, float y)              { return (x < y); }
    bool BlendTreeFloatConditionNode::FloatConditionGreaterOrEqual(float x, float y)    { return (x >= y); }
    bool BlendTreeFloatConditionNode::FloatConditionLessOrEqual(float x, float y)       { return (x <= y); }

    void BlendTreeFloatConditionNode::SetDefaultValue(float defaultValue)
    {
        m_defaultValue = defaultValue;
    }

    void BlendTreeFloatConditionNode::SetTrueResult(float trueResultValue)
    {
        m_trueResult = trueResultValue;
    }

    void BlendTreeFloatConditionNode::SetFalseResult(float falseResultValue)
    {
        m_falseResult = falseResultValue;
    }

    void BlendTreeFloatConditionNode::SetTrueReturnMode(EReturnMode returnMode)
    {
        m_trueReturnMode = returnMode;
    }

    void BlendTreeFloatConditionNode::SetFalseReturnMode(EReturnMode returnMode)
    {
        m_falseReturnMode = returnMode;
    }

    void BlendTreeFloatConditionNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<BlendTreeFloatConditionNode, AnimGraphNode>()
            ->Version(1)
            ->Field("conditionFunction", &BlendTreeFloatConditionNode::m_functionEnum)
            ->Field("defaultValue", &BlendTreeFloatConditionNode::m_defaultValue)
            ->Field("trueResult", &BlendTreeFloatConditionNode::m_trueResult)
            ->Field("falseResult", &BlendTreeFloatConditionNode::m_falseResult)
            ->Field("trueReturnMode", &BlendTreeFloatConditionNode::m_trueReturnMode)
            ->Field("falseReturnMode", &BlendTreeFloatConditionNode::m_falseReturnMode)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BlendTreeFloatConditionNode>("Float Condition", "Float condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeFloatConditionNode::m_functionEnum, "Condition Function", "The condition function to use.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &BlendTreeFloatConditionNode::Reinit)
                ->EnumAttribute(FUNCTION_EQUAL,          "Is Equal")
                ->EnumAttribute(FUNCTION_GREATER,        "Is Greater")
                ->EnumAttribute(FUNCTION_LESS,           "Is Less")
                ->EnumAttribute(FUNCTION_GREATEROREQUAL, "Is Greater Or Equal")
                ->EnumAttribute(FUNCTION_LESSOREQUAL,    "Is Less Or Equal")
                ->EnumAttribute(FUNCTION_NOTEQUAL,       "Is Not Equal")
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatConditionNode::m_defaultValue, "Default Value", "Value used for x or y when the input port has no connection.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatConditionNode::m_trueResult, "Result When True", "The value returned when the expression is true.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &BlendTreeFloatConditionNode::m_falseResult, "Result When False", "The value returned when the expression is false.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeFloatConditionNode::m_trueReturnMode, "True Return Mode", "What to return when the result is true.")
                ->EnumAttribute(MODE_VALUE, "Return True Value")
                ->EnumAttribute(MODE_X,     "Return X")
                ->EnumAttribute(MODE_Y,     "Return Y")
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &BlendTreeFloatConditionNode::m_falseReturnMode, "False Return Mode", "What to return when the result is false.")
                ->EnumAttribute(MODE_VALUE, "Return False Value")
                ->EnumAttribute(MODE_X,     "Return X")
                ->EnumAttribute(MODE_Y,     "Return Y")
            ;
    }
} // namespace EMotionFX
