/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FloatParameter.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeInt32.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(FloatParameter, AnimGraphAllocator);
    AZ_RTTI_NO_TYPE_INFO_IMPL(FloatParameter, ValueParameter);

    void FloatParameter::Reflect(AZ::ReflectContext* context)
    {
        // This method calls Reflect() on it's parent class, which is uncommon
        // in the LY reflection framework.  This is because the parent class is
        // a template, and is unique to each type that subclasses it, as it
        // uses the Curiously Recursive Template Pattern.
        BaseType::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<FloatParameter, BaseType>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<FloatParameter>("Float parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }

    MCore::Attribute* FloatParameter::ConstructDefaultValueAsAttribute() const
    {
        return MCore::AttributeFloat::Create(m_defaultValue);
    }

    uint32 FloatParameter::GetType() const
    {
        return MCore::AttributeFloat::TYPE_ID;
    }

    bool FloatParameter::AssignDefaultValueToAttribute(MCore::Attribute* attribute) const
    {
        switch (attribute->GetType())
        {
        case MCore::AttributeFloat::TYPE_ID:
            static_cast<MCore::AttributeFloat*>(attribute)->SetValue(GetDefaultValue());
            return true;
        case MCore::AttributeBool::TYPE_ID:
            static_cast<MCore::AttributeBool*>(attribute)->SetValue(!MCore::Math::IsFloatZero(GetDefaultValue()));
            return true;
        case MCore::AttributeInt32::TYPE_ID:
            static_cast<MCore::AttributeInt32*>(attribute)->SetValue(static_cast<int32>(GetDefaultValue()));
            return true;
        default:
            return false;
        }
    }

    bool FloatParameter::SetDefaultValueFromAttribute(MCore::Attribute* attribute)
    {
        switch (attribute->GetType())
        {
        case MCore::AttributeFloat::TYPE_ID:
            SetDefaultValue(static_cast<MCore::AttributeFloat*>(attribute)->GetValue());
            return true;
        case MCore::AttributeBool::TYPE_ID:
            SetDefaultValue(static_cast<MCore::AttributeBool*>(attribute)->GetValue());
            return true;
        case MCore::AttributeInt32::TYPE_ID:
            SetDefaultValue(static_cast<float>(static_cast<MCore::AttributeInt32*>(attribute)->GetValue()));
            return true;
        default:
            return false;
        }
    }

    bool FloatParameter::SetMinValueFromAttribute(MCore::Attribute* attribute)
    {
        switch (attribute->GetType())
        {
        case MCore::AttributeFloat::TYPE_ID:
            SetMinValue(static_cast<MCore::AttributeFloat*>(attribute)->GetValue());
            return true;
        case MCore::AttributeBool::TYPE_ID:
            SetMinValue(static_cast<MCore::AttributeBool*>(attribute)->GetValue());
            return true;
        case MCore::AttributeInt32::TYPE_ID:
            SetMinValue(static_cast<float>(static_cast<MCore::AttributeInt32*>(attribute)->GetValue()));
            return true;
        default:
            return false;
        }
    }

    bool FloatParameter::SetMaxValueFromAttribute(MCore::Attribute* attribute)
    {
        switch (attribute->GetType())
        {
        case MCore::AttributeFloat::TYPE_ID:
            SetMaxValue(static_cast<MCore::AttributeFloat*>(attribute)->GetValue());
            return true;
        case MCore::AttributeBool::TYPE_ID:
            SetMaxValue(static_cast<MCore::AttributeBool*>(attribute)->GetValue());
            return true;
        case MCore::AttributeInt32::TYPE_ID:
            SetMaxValue(static_cast<float>(static_cast<MCore::AttributeInt32*>(attribute)->GetValue()));
            return true;
        default:
            return false;
        }
    }

    float FloatParameter::GetUnboundedMinValue()
    {
        return -FLT_MAX;
    }

    float FloatParameter::GetUnboundedMaxValue()
    {
        return FLT_MAX;
    }
}
