/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FloatSpinnerParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Parameter/FloatParameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <MCore/Source/AttributeFloat.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(FloatSpinnerParameterEditor, EMStudio::UIAllocator)

    FloatSpinnerParameterEditor::FloatSpinnerParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*> attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
    {
        UpdateValue();
    }

    void FloatSpinnerParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<FloatSpinnerParameterEditor, ValueParameterEditor>()
            ->Version(1)
            ->Field("value", &FloatSpinnerParameterEditor::m_currentValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<FloatSpinnerParameterEditor>("Float spinner parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &FloatSpinnerParameterEditor::m_currentValue, "", "")
                ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
                ->Attribute(AZ::Edit::Attributes::Min, &FloatSpinnerParameterEditor::GetMinValue)
                ->Attribute(AZ::Edit::Attributes::Max, &FloatSpinnerParameterEditor::GetMaxValue)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &FloatSpinnerParameterEditor::OnValueChanged)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
    }

    void FloatSpinnerParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeFloat* attribute = static_cast<MCore::AttributeFloat*>(m_attributes[0]);
            m_currentValue = attribute->GetValue();
        }
        else
        {
            const EMotionFX::FloatParameter* parameter = static_cast<const EMotionFX::FloatParameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
    }

    float FloatSpinnerParameterEditor::GetMinValue() const
    {
        const EMotionFX::FloatParameter* parameter = static_cast<const EMotionFX::FloatParameter*>(m_valueParameter);
        return parameter->GetMinValue();
    }

    float FloatSpinnerParameterEditor::GetMaxValue() const
    {
        const EMotionFX::FloatParameter* parameter = static_cast<const EMotionFX::FloatParameter*>(m_valueParameter);
        return parameter->GetMaxValue();
    }

    void FloatSpinnerParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeFloat* typedAttribute = static_cast<MCore::AttributeFloat*>(attribute);
            typedAttribute->SetValue(m_currentValue);
        }
    }
}
