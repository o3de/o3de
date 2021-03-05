/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "Vector3Parameter.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/AttributeVector3.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Vector3Parameter, AnimGraphAllocator, 0)


    void Vector3Parameter::Reflect(AZ::ReflectContext* context)
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

        serializeContext->Class<Vector3Parameter, BaseType>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<Vector3Parameter>("Vector3 parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }

    const char* Vector3Parameter::GetTypeDisplayName() const
    {
        return "Vector3";
    }

    MCore::Attribute* Vector3Parameter::ConstructDefaultValueAsAttribute() const
    {
        return MCore::AttributeVector3::Create(m_defaultValue.GetX(), m_defaultValue.GetY(), m_defaultValue.GetZ());
    }

    uint32 Vector3Parameter::GetType() const
    {
        return MCore::AttributeVector3::TYPE_ID;
    }

    bool Vector3Parameter::AssignDefaultValueToAttribute(MCore::Attribute* attribute) const
    {
        if (attribute->GetType() == MCore::AttributeVector3::TYPE_ID)
        {
            static_cast<MCore::AttributeVector3*>(attribute)->SetValue(m_defaultValue);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Vector3Parameter::SetDefaultValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeVector3::TYPE_ID)
        {
            SetDefaultValue(static_cast<MCore::AttributeVector3*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Vector3Parameter::SetMinValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeVector3::TYPE_ID)
        {
            SetMinValue(static_cast<MCore::AttributeVector3*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    bool Vector3Parameter::SetMaxValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeVector3::TYPE_ID)
        {
            SetMaxValue(static_cast<MCore::AttributeVector3*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    AZ::Vector3 Vector3Parameter::GetUnboundedMinValue()
    {
        return AZ::Vector3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    AZ::Vector3 Vector3Parameter::GetUnboundedMaxValue()
    {
        return AZ::Vector3(FLT_MAX, FLT_MAX, FLT_MAX);
    }
}
