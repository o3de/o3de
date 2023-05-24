/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RotationParameter.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/AttributeQuaternion.h>
#include <MCore/Source/AzCoreConversions.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(RotationParameter, AnimGraphAllocator);
    AZ_RTTI_NO_TYPE_INFO_IMPL(RotationParameter, ValueParameter);

    void RotationParameter::Reflect(AZ::ReflectContext* context)
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

        serializeContext->Class<RotationParameter, BaseType>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<RotationParameter>("Rotation parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }

    const char* RotationParameter::GetTypeDisplayName() const
    {
        return "Rotation";
    }

    MCore::Attribute* RotationParameter::ConstructDefaultValueAsAttribute() const
    {
        return MCore::AttributeQuaternion::Create(m_defaultValue.GetX(), m_defaultValue.GetY(), m_defaultValue.GetZ(), m_defaultValue.GetW());
    }

    uint32 RotationParameter::GetType() const
    {
        return MCore::AttributeQuaternion::TYPE_ID;
    }

    bool RotationParameter::AssignDefaultValueToAttribute(MCore::Attribute* attribute) const
    {
        if (attribute->GetType() == MCore::AttributeQuaternion::TYPE_ID)
        {
            static_cast<MCore::AttributeQuaternion*>(attribute)->SetValue(m_defaultValue);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool RotationParameter::SetDefaultValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeQuaternion::TYPE_ID)
        {
            SetDefaultValue(static_cast<MCore::AttributeQuaternion*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    bool RotationParameter::SetMinValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeQuaternion::TYPE_ID)
        {
            SetMinValue(static_cast<MCore::AttributeQuaternion*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    bool RotationParameter::SetMaxValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeQuaternion::TYPE_ID)
        {
            SetMaxValue(static_cast<MCore::AttributeQuaternion*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    AZ::Quaternion RotationParameter::GetUnboundedMinValue()
    {
        return AZ::Quaternion(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);
    }

    AZ::Quaternion RotationParameter::GetUnboundedMaxValue()
    {
        return AZ::Quaternion(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
    }
}
