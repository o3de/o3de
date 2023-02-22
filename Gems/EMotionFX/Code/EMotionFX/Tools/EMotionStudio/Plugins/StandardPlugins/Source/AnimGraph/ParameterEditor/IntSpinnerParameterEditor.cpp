/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "IntSpinnerParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Parameter/IntParameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <MCore/Source/AttributeInt32.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(IntSpinnerParameterEditor, EMStudio::UIAllocator)

    IntSpinnerParameterEditor::IntSpinnerParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*> attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
    {
        UpdateValue();
    }

    void IntSpinnerParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<IntSpinnerParameterEditor, ValueParameterEditor>()
            ->Version(1)
            ->Field("value", &IntSpinnerParameterEditor::m_currentValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<IntSpinnerParameterEditor>("Int spinner parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &IntSpinnerParameterEditor::m_currentValue, "", "")
                ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
                ->Attribute(AZ::Edit::Attributes::Min, &IntSpinnerParameterEditor::GetMinValue)
                ->Attribute(AZ::Edit::Attributes::Max, &IntSpinnerParameterEditor::GetMaxValue)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &IntSpinnerParameterEditor::OnValueChanged)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
    }

    void IntSpinnerParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeInt32* attribute = static_cast<MCore::AttributeInt32*>(m_attributes[0]);
            m_currentValue = attribute->GetValue();
        }
        else
        {
            const EMotionFX::IntParameter* parameter = static_cast<const EMotionFX::IntParameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
    }

    int IntSpinnerParameterEditor::GetMinValue() const
    {
        const EMotionFX::IntParameter* parameter = static_cast<const EMotionFX::IntParameter*>(m_valueParameter);
        return parameter->GetMinValue();
    }

    int IntSpinnerParameterEditor::GetMaxValue() const
    {
        const EMotionFX::IntParameter* parameter = static_cast<const EMotionFX::IntParameter*>(m_valueParameter);
        return parameter->GetMaxValue();
    }

    void IntSpinnerParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeInt32* typedAttribute = static_cast<MCore::AttributeInt32*>(attribute);
            typedAttribute->SetValue(m_currentValue);
        }
    }
}
