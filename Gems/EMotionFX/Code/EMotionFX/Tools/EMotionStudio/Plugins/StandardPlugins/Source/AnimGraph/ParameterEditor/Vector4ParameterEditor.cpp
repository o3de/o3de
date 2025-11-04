/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Vector4ParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Parameter/Vector4Parameter.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <MCore/Source/AttributeVector4.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(Vector4ParameterEditor, EMStudio::UIAllocator)

    Vector4ParameterEditor::Vector4ParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
        , m_currentValue(0.0f, 0.0f, 0.0f, 0.0f)
    {
        UpdateValue();
    }

    void Vector4ParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<Vector4ParameterEditor, ValueParameterEditor>()
            ->Version(1)
            ->Field("value", &Vector4ParameterEditor::m_currentValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Vector4ParameterEditor>("Vector4 parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &Vector4ParameterEditor::m_currentValue, "", "")
                ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
                ->Attribute(AZ::Edit::Attributes::Min, &Vector4ParameterEditor::GetMinValue)
                ->Attribute(AZ::Edit::Attributes::Max, &Vector4ParameterEditor::GetMaxValue)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &Vector4ParameterEditor::OnValueChanged)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
    }

    void Vector4ParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeVector4* attribute = static_cast<MCore::AttributeVector4*>(m_attributes[0]);
            m_currentValue = attribute->GetValue();
        }
        else
        {
            const EMotionFX::Vector4Parameter* parameter = static_cast<const EMotionFX::Vector4Parameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
    }

    AZ::Vector4 Vector4ParameterEditor::GetMinValue() const
    {
        const EMotionFX::Vector4Parameter* parameter = static_cast<const EMotionFX::Vector4Parameter*>(m_valueParameter);
        return parameter->GetMinValue();
    }

    AZ::Vector4 Vector4ParameterEditor::GetMaxValue() const
    {
        const EMotionFX::Vector4Parameter* parameter = static_cast<const EMotionFX::Vector4Parameter*>(m_valueParameter);
        return parameter->GetMaxValue();
    }

    void Vector4ParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeVector4* typedAttribute = static_cast<MCore::AttributeVector4*>(attribute);
            typedAttribute->SetValue(m_currentValue);
        }
    }
}
