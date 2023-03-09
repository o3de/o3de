/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Vector2Parameter.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/AttributeVector2.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Vector2Parameter, AnimGraphAllocator);
    AZ_RTTI_NO_TYPE_INFO_IMPL(Vector2Parameter, ValueParameter);

    void Vector2Parameter::Reflect(AZ::ReflectContext* context)
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

        serializeContext->Class<Vector2Parameter, BaseType>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Vector2Parameter>("Vector2 parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }

    const char* Vector2Parameter::GetTypeDisplayName() const
    {
        return "Vector2";
    }

    MCore::Attribute* Vector2Parameter::ConstructDefaultValueAsAttribute() const
    {
        return MCore::AttributeVector2::Create(m_defaultValue);
    }

    uint32 Vector2Parameter::GetType() const
    {
        return MCore::AttributeVector2::TYPE_ID;
    }

    bool Vector2Parameter::AssignDefaultValueToAttribute(MCore::Attribute* attribute) const
    {
        if (attribute->GetType() == MCore::AttributeVector2::TYPE_ID)
        {
            static_cast<MCore::AttributeVector2*>(attribute)->SetValue(GetDefaultValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Vector2Parameter::SetDefaultValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeVector2::TYPE_ID)
        {
            SetDefaultValue(static_cast<MCore::AttributeVector2*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Vector2Parameter::SetMinValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeVector2::TYPE_ID)
        {
            SetMinValue(static_cast<MCore::AttributeVector2*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Vector2Parameter::SetMaxValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeVector2::TYPE_ID)
        {
            SetMaxValue(static_cast<MCore::AttributeVector2*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    AZ::Vector2 Vector2Parameter::GetUnboundedMinValue()
    {
        return AZ::Vector2(-FLT_MAX, -FLT_MAX);
    }

    AZ::Vector2 Vector2Parameter::GetUnboundedMaxValue()
    {
        return AZ::Vector2(FLT_MAX, FLT_MAX);
    }
}
