/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Vector2ParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <MCore/Source/AttributeVector2.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(Vector2ParameterEditor, EMStudio::UIAllocator)

    Vector2ParameterEditor::Vector2ParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
    {
        UpdateValue();
    }

    void Vector2ParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Vector2ParameterEditor, ValueParameterEditor>()
            ->Version(1)
            ->Field("value", &Vector2ParameterEditor::m_currentValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Vector2ParameterEditor>("Vector2 parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Vector2ParameterEditor::m_currentValue, "", "")
                ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
                ->Attribute(AZ::Edit::Attributes::Min, &Vector2ParameterEditor::GetMinValue)
                ->Attribute(AZ::Edit::Attributes::Max, &Vector2ParameterEditor::GetMaxValue)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Vector2ParameterEditor::OnValueChanged)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
    }

    void Vector2ParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeVector2* attribute = static_cast<MCore::AttributeVector2*>(m_attributes[0]);
            m_currentValue = attribute->GetValue();
        }
        else
        {
            const EMotionFX::Vector2Parameter* parameter = static_cast<const EMotionFX::Vector2Parameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
    }

    AZ::Vector2 Vector2ParameterEditor::GetMinValue() const
    {
        const EMotionFX::Vector2Parameter* parameter = static_cast<const EMotionFX::Vector2Parameter*>(m_valueParameter);
        return parameter->GetMinValue();
    }

    AZ::Vector2 Vector2ParameterEditor::GetMaxValue() const
    {
        const EMotionFX::Vector2Parameter* parameter = static_cast<const EMotionFX::Vector2Parameter*>(m_valueParameter);
        return parameter->GetMaxValue();
    }

    void Vector2ParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeVector2* typedAttribute = static_cast<MCore::AttributeVector2*>(attribute);
            typedAttribute->SetValue(m_currentValue);
        }
    }
}
