/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StringParameterEditor.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <MCore/Source/AttributeString.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(StringParameterEditor, EMStudio::UIAllocator)

    StringParameterEditor::StringParameterEditor(EMotionFX::AnimGraph* animGraph, const EMotionFX::ValueParameter* valueParameter, const AZStd::vector<MCore::Attribute*>& attributes)
        : ValueParameterEditor(animGraph, valueParameter, attributes)
    {
        UpdateValue();
    }

    void StringParameterEditor::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<StringParameterEditor, ValueParameterEditor>()
            ->Field("value", &StringParameterEditor::m_currentValue)
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<StringParameterEditor>("String parameter editor", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::Default, &StringParameterEditor::m_currentValue, "", "")
                ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &ValueParameterEditor::GetDescription)
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, &StringParameterEditor::OnValueChanged)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, &ValueParameterEditor::IsReadOnly)
        ;
    }

    void StringParameterEditor::UpdateValue()
    {
        // Use the value from the first attribute, they all should match since they all are the same parameter in different graph instances
        if (!m_attributes.empty())
        {
            MCore::AttributeString* attribute = static_cast<MCore::AttributeString*>(m_attributes[0]);
            m_currentValue = attribute->GetValue();
        }
        else
        {
            const EMotionFX::StringParameter* parameter = static_cast<const EMotionFX::StringParameter*>(m_valueParameter);
            m_currentValue = parameter->GetDefaultValue();
        }
    }

    void StringParameterEditor::OnValueChanged()
    {
        for (MCore::Attribute* attribute : m_attributes)
        {
            MCore::AttributeString* typedAttribute = static_cast<MCore::AttributeString*>(attribute);
            typedAttribute->SetValue(m_currentValue);
        }
    }
}
