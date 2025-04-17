/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphVector2Condition.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConditionCommands.h>
#include <MCore/Source/Compare.h>

namespace EMotionFX
{
    const char* AnimGraphVector2Condition::s_operationLength = "Length";
    const char* AnimGraphVector2Condition::s_operationGetX = "Get X";
    const char* AnimGraphVector2Condition::s_operationGetY = "Get Y";

    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphVector2Condition, AnimGraphAllocator)

    AnimGraphVector2Condition::AnimGraphVector2Condition()
        : AnimGraphTransitionCondition()
        , m_parameterIndex(AZ::Failure())
        , m_operation(OPERATION_LENGTH)
        , m_operationFunction(OperationLength)
        , m_function(AnimGraphParameterCondition::FUNCTION_GREATER)
        , m_testFunction(TestGreater)
        , m_testValue(0.0f)
        , m_rangeValue(0.0f)
    {
    }


    AnimGraphVector2Condition::AnimGraphVector2Condition(AnimGraph* animGraph)
        : AnimGraphVector2Condition()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphVector2Condition::~AnimGraphVector2Condition()
    {
    }


    void AnimGraphVector2Condition::Reinit()
    {
        // Update the function pointers.
        SetFunction(m_function);
        SetOperation(m_operation);

        // Find the parameter index for the given parameter name, to prevent string based lookups every frame
        m_parameterIndex = m_animGraph->FindValueParameterIndexByName(m_parameterName);
    }


    bool AnimGraphVector2Condition::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTransitionCondition::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* AnimGraphVector2Condition::GetPaletteName() const
    {
        return "Vector2 Condition";
    }


    // get the type of the selected paramter
    AZ::TypeId AnimGraphVector2Condition::GetParameterType() const
    {
        // if there is no parameter selected yet, return invalid type
        if (m_parameterIndex.IsSuccess())
        {
            // get access to the parameter info and return the type of its default value
            const ValueParameter* valueParameter = m_animGraph->FindValueParameter(m_parameterIndex.GetValue());
            return azrtti_typeid(valueParameter);
        }
        else
        {
            return AZ::TypeId();
        }
    }


    // test the condition
    bool AnimGraphVector2Condition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // allow the transition in case we don't have a valid parameter to test against
        if (!m_parameterIndex.IsSuccess())
        {
            return false; // act like this condition failed
        }

        // make sure we have the right type, otherwise fail
        const AZ::TypeId parameterType = GetParameterType();
        if (parameterType != azrtti_typeid<Vector2Parameter>())
        {
            return false;
        }

        // get the vector value
        AZ::Vector2 vectorValue(0.0f, 0.0f);
        MCore::Attribute* attribute = animGraphInstance->GetParameterValue(static_cast<uint32>(m_parameterIndex.GetValue()));
        MCORE_ASSERT(attribute->GetType() == MCore::AttributeVector2::TYPE_ID);
        vectorValue = static_cast<MCore::AttributeVector2*>(attribute)->GetValue();

        // perform the operation on the vector
        const float operationResult = m_operationFunction(vectorValue);

        // now apply the function
        return m_testFunction(operationResult, m_testValue, m_rangeValue);
    }


    // set the math function to use
    void AnimGraphVector2Condition::SetFunction(AnimGraphParameterCondition::EFunction func)
    {
        m_function = func;

        switch (m_function)
        {
            case AnimGraphParameterCondition::FUNCTION_GREATER:
                m_testFunction = TestGreater;
                return;
            case AnimGraphParameterCondition::FUNCTION_GREATEREQUAL:
                m_testFunction = TestGreaterEqual;
                return;
            case AnimGraphParameterCondition::FUNCTION_LESS:
                m_testFunction = TestLess;
                return;
            case AnimGraphParameterCondition::FUNCTION_LESSEQUAL:
                m_testFunction = TestLessEqual;
                return;
            case AnimGraphParameterCondition::FUNCTION_NOTEQUAL:
                m_testFunction = TestNotEqual;
                return;
            case AnimGraphParameterCondition::FUNCTION_EQUAL:
                m_testFunction = TestEqual;
                return;
            case AnimGraphParameterCondition::FUNCTION_INRANGE:
                m_testFunction = TestInRange;
                return;
            case AnimGraphParameterCondition::FUNCTION_NOTINRANGE:
                m_testFunction = TestNotInRange;
                return;
            default:
                MCORE_ASSERT(false);        // function unknown
            }
        ;
    }


    // set the operation
    void AnimGraphVector2Condition::SetOperation(EOperation operation)
    {
        m_operation = operation;

        switch (m_operation)
        {
            case OPERATION_LENGTH:
                m_operationFunction = OperationLength;
                return;
            case OPERATION_GETX:
                m_operationFunction = OperationGetX;
                return;
            case OPERATION_GETY:
                m_operationFunction = OperationGetY;
                return;
            default:
                MCORE_ASSERT(false);        // function unknown
        }
    }

    
    // get the name of the currently used test function
    const char* AnimGraphVector2Condition::GetTestFunctionString() const
    {
        return AnimGraphParameterCondition::GetTestFunctionString(m_function);
    }


    // get the name of the currently used operation
    const char* AnimGraphVector2Condition::GetOperationString() const
    {
        switch (m_operation)
        {
            case OPERATION_LENGTH:
            {
                return s_operationLength;
            }
            case OPERATION_GETX:
            {
                return s_operationGetX;
            }
            case OPERATION_GETY:
            {
                return s_operationGetY;
            }
            default:
            {
                return "Unknown operation";
            }
        }
    }


    // construct and output the information summary string for this object
    void AnimGraphVector2Condition::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Parameter Name='%s', Test Function='%s', Test Value=%.2f", RTTI_GetTypeName(), m_parameterName.c_str(), GetTestFunctionString(), m_testValue);
    }


    // construct and output the tooltip for this object
    void AnimGraphVector2Condition::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"120\"><b>%s</b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // add the parameter
        columnName = "Parameter Name: ";
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), m_parameterName.c_str());

        // add the operation
        columnName = "Operation: ";
        columnValue = GetOperationString();
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td></tr>", columnName.c_str(), columnValue.c_str());

        // add the test function
        columnName = "Test Function: ";
        columnValue = GetTestFunctionString();
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td></tr>", columnName.c_str(), columnValue.c_str());

        // add the test value
        columnName = "Test Value: ";
        columnValue = AZStd::string::format("%.3f", m_testValue);
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());

        // add the range value
        columnName = "Range Value: ";
        columnValue = AZStd::string::format("%.3f", m_rangeValue);
        *outResult += AZStd::string::format("</tr><tr><td><b><nobr>%s</nobr></b></td><td><nobr>%s</nobr></td>", columnName.c_str(), columnValue.c_str());
    }


    //------------------------------------------------------------------------------------------
    // Test Functions
    //------------------------------------------------------------------------------------------
    bool AnimGraphVector2Condition::TestGreater(float paramValue, float testValue, float rangeValue)           { MCORE_UNUSED(rangeValue); return (paramValue > testValue);    }
    bool AnimGraphVector2Condition::TestGreaterEqual(float paramValue, float testValue, float rangeValue)      { return TestGreater(paramValue, testValue, rangeValue) || TestEqual(paramValue, testValue, rangeValue);   }
    bool AnimGraphVector2Condition::TestLess(float paramValue, float testValue, float rangeValue)              { MCORE_UNUSED(rangeValue); return (paramValue < testValue);    }
    bool AnimGraphVector2Condition::TestLessEqual(float paramValue, float testValue, float rangeValue)         { return TestLess(paramValue, testValue, rangeValue) || TestEqual(paramValue, testValue, rangeValue); }
    bool AnimGraphVector2Condition::TestEqual(float paramValue, float testValue, float rangeValue)             { MCORE_UNUSED(rangeValue); return MCore::Compare<float>::CheckIfIsClose(paramValue, testValue, MCore::Math::epsilon); }
    bool AnimGraphVector2Condition::TestNotEqual(float paramValue, float testValue, float rangeValue)          { MCORE_UNUSED(rangeValue); return !MCore::Compare<float>::CheckIfIsClose(paramValue, testValue, MCore::Math::epsilon); }
    bool AnimGraphVector2Condition::TestInRange(float paramValue, float testValue, float rangeValue)
    {
        if (testValue <= rangeValue)
        {
            return (MCore::InRange<float>(paramValue, testValue, rangeValue));
        }
        else
        {
            return (MCore::InRange<float>(paramValue, rangeValue, testValue));
        }
    }

    bool AnimGraphVector2Condition::TestNotInRange(float paramValue, float testValue, float rangeValue)
    {
        if (testValue <= rangeValue)
        {
            return (MCore::InRange<float>(paramValue, testValue, rangeValue) == false);
        }
        else
        {
            return (MCore::InRange<float>(paramValue, rangeValue, testValue) == false);
        }
    }


    //------------------------------------------------------------------------------------------
    // Operations
    //------------------------------------------------------------------------------------------
    float AnimGraphVector2Condition::OperationLength(const AZ::Vector2& vec)                               { return vec.GetLength();   }
    float AnimGraphVector2Condition::OperationGetX(const AZ::Vector2& vec)                                 { return vec.GetX();    }
    float AnimGraphVector2Condition::OperationGetY(const AZ::Vector2& vec)                                 { return vec.GetY();    }


    AZ::Crc32 AnimGraphVector2Condition::GetRangeValueVisibility() const
    {
        if (m_function == AnimGraphParameterCondition::FUNCTION_INRANGE || m_function == AnimGraphParameterCondition::FUNCTION_NOTINRANGE)
        {
            return AZ::Edit::PropertyVisibility::Show;
        }
        
        return AZ::Edit::PropertyVisibility::Hide;
    }

    void AnimGraphVector2Condition::SetRangeValue(float rangeValue)
    {
        m_rangeValue = rangeValue;
    }

    void AnimGraphVector2Condition::SetTestValue(float testValue)
    {
        m_testValue = testValue;
    }

    AZ::Outcome<size_t> AnimGraphVector2Condition::GetParameterIndex() const
    {
        return m_parameterIndex;
    }

    void AnimGraphVector2Condition::SetParameterName(const AZStd::string& parameterName)
    {
        m_parameterName = parameterName;
    }

    void AnimGraphVector2Condition::ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName)
    {
        if (m_parameterName == oldParameterName)
        {
            SetParameterName(newParameterName);
        }
    }

    void AnimGraphVector2Condition::ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange)
    {
        AZ_UNUSED(beforeChange);
        AZ_UNUSED(afterChange);
        m_parameterIndex = m_animGraph->FindValueParameterIndexByName(m_parameterName);
    }

    void AnimGraphVector2Condition::ParameterRemoved(const AZStd::string& oldParameterName)
    {
        if (oldParameterName == m_parameterName)
        {
            m_parameterName.clear();
            m_parameterIndex = AZ::Failure();
        }
        else
        {
            m_parameterIndex = m_animGraph->FindValueParameterIndexByName(m_parameterName);
        }
    }

    void AnimGraphVector2Condition::BuildParameterRemovedCommands(MCore::CommandGroup& commandGroup, const AZStd::string& parameterNameToBeRemoved)
    {
        // Only handle in case the parameter condition is linked to the to be removed parameter.
        if (!m_parameterName.empty() && m_parameterName == parameterNameToBeRemoved)
        {
            AZ::Outcome<size_t> conditionIndex = m_transition->FindConditionIndex(this);
            if (conditionIndex.IsSuccess())
            {
                CommandSystem::CommandAdjustTransitionCondition* command = aznew CommandSystem::CommandAdjustTransitionCondition(
                    m_transition->GetAnimGraph()->GetID(),
                    m_transition->GetId(),
                    conditionIndex.GetValue(),
                    /*attributesString=*/"-parameterName \"\""); // Clear the linked parameter as it got removed.
                commandGroup.AddCommand(command);
            }
        }
    }

    void AnimGraphVector2Condition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphVector2Condition, AnimGraphTransitionCondition>()
            ->Version(1)
            ->Field("parameterName", &AnimGraphVector2Condition::m_parameterName)
            ->Field("operation", &AnimGraphVector2Condition::m_operation)
            ->Field("testFunction", &AnimGraphVector2Condition::m_function)
            ->Field("testValue", &AnimGraphVector2Condition::m_testValue)
            ->Field("rangeValue", &AnimGraphVector2Condition::m_rangeValue)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphVector2Condition>("Vector2 Condition", "Vector2 condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ_CRC_CE("AnimGraphVector2Parameter"), &AnimGraphVector2Condition::m_parameterName, "Parameter", "The parameter name to apply the condition on.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphVector2Condition::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                ->Attribute(AZ_CRC_CE("AnimGraph"), &AnimGraphVector2Condition::GetAnimGraph)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphVector2Condition::m_operation, "Operation", "The type of operation to perform on the vector.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphVector2Condition::Reinit)
                ->EnumAttribute(OPERATION_LENGTH,   s_operationLength)
                ->EnumAttribute(OPERATION_GETX,     s_operationGetX)
                ->EnumAttribute(OPERATION_GETY,     s_operationGetY)
            ->DataElement(AZ::Edit::UIHandlers::ComboBox, &AnimGraphVector2Condition::m_function)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &AnimGraphVector2Condition::Reinit)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphVector2Condition::m_testValue, "Test Value", "The float value to test against the parameter value.")
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::Default, &AnimGraphVector2Condition::m_rangeValue, "Range Value", "The range high or low bound value, only used when the function is set to 'In Range' or 'Not in Range'.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphVector2Condition::GetRangeValueVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, -std::numeric_limits<float>::max())
                ->Attribute(AZ::Edit::Attributes::Max,  std::numeric_limits<float>::max())
            ;
    }
} // namespace EMotionFX
