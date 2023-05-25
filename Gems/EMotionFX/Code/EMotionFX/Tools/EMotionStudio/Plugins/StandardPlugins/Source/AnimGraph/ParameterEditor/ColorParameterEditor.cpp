/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColorParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Parameter/ColorParameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <MCore/Source/AttributeColor.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(ColorParameterEditor, EMStudio::UIAllocator)

    ColorParameterEditor::ColorParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
        , m_currentValue(1.0f, 0.0f, 0.0f, 1.0f)
    {
        UpdateValue();
    }

    void ColorParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ColorParameterEditor, ValueParameterEditor>()
            ->Version(1)
            ->Field("value", &ColorParameterEditor::m_currentValue)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<ColorParameterEditor>("Color parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &ColorParameterEditor::m_currentValue, "", "")
                ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
                ->Attribute(AZ::Edit::Attributes::Min, &ColorParameterEditor::GetMinValue)
                ->Attribute(AZ::Edit::Attributes::Max, &ColorParameterEditor::GetMaxValue)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &ColorParameterEditor::OnValueChanged)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
    }

    void ColorParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeColor* attribute = static_cast<MCore::AttributeColor*>(m_attributes[0]);
            m_currentValue = attribute->GetValue();
        }
        else
        {
            const EMotionFX::ColorParameter* parameter = static_cast<const EMotionFX::ColorParameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
    }

    AZ::Color ColorParameterEditor::GetMinValue() const
    {
        const EMotionFX::ColorParameter* parameter = static_cast<const EMotionFX::ColorParameter*>(m_valueParameter);
        return parameter->GetMinValue();
    }

    AZ::Color ColorParameterEditor::GetMaxValue() const
    {
        const EMotionFX::ColorParameter* parameter = static_cast<const EMotionFX::ColorParameter*>(m_valueParameter);
        return parameter->GetMaxValue();
    }

    void ColorParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeColor* typedAttribute = static_cast<MCore::AttributeColor*>(attribute);
            typedAttribute->SetValue(m_currentValue);
        }
    }
}
