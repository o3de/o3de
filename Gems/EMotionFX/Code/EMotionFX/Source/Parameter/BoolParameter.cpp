/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BoolParameter.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/AttributeBool.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeInt32.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BoolParameter, AnimGraphAllocator);
    AZ_RTTI_NO_TYPE_INFO_IMPL(BoolParameter, ValueParameter);

    const char* BoolParameter::GetTypeDisplayName() const
    {
        return "Boolean (checkbox)";
    }

    void BoolParameter::Reflect(AZ::ReflectContext* context)
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

        serializeContext->Class<BoolParameter, BaseType>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<BoolParameter>("Bool parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }

    MCore::Attribute* BoolParameter::ConstructDefaultValueAsAttribute() const
    {
        return MCore::AttributeBool::Create(m_defaultValue);
    }

    uint32 BoolParameter::GetType() const
    {
        return MCore::AttributeBool::TYPE_ID;
    }

    bool BoolParameter::AssignDefaultValueToAttribute(MCore::Attribute* attribute) const
    {
        switch (attribute->GetType())
        {
        case MCore::AttributeFloat::TYPE_ID:
            static_cast<MCore::AttributeFloat*>(attribute)->SetValue(GetDefaultValue());
            return true;
        case MCore::AttributeBool::TYPE_ID:
            static_cast<MCore::AttributeBool*>(attribute)->SetValue(GetDefaultValue());
            return true;
        case MCore::AttributeInt32::TYPE_ID:
            static_cast<MCore::AttributeInt32*>(attribute)->SetValue(GetDefaultValue());
            return true;
        default:
            return false;
        }
    }

    bool BoolParameter::SetDefaultValueFromAttribute(MCore::Attribute* attribute)
    {
        switch (attribute->GetType())
        {
        case MCore::AttributeFloat::TYPE_ID:
            SetDefaultValue(!MCore::Math::IsFloatZero(static_cast<MCore::AttributeFloat*>(attribute)->GetValue()));
            return true;
        case MCore::AttributeBool::TYPE_ID:
            SetDefaultValue(static_cast<MCore::AttributeBool*>(attribute)->GetValue());
            return true;
        case MCore::AttributeInt32::TYPE_ID:
            SetDefaultValue(static_cast<MCore::AttributeInt32*>(attribute)->GetValue() != 0);
            return true;
        default:
            return false;
        }
    }
}
