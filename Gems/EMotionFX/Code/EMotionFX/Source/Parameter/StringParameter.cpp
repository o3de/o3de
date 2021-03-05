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

#include "StringParameter.h"

#include <EMotionFX/Source/Allocators.h>
#include <MCore/Source/AttributeString.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(StringParameter, AnimGraphAllocator, 0);


    void StringParameter::Reflect(AZ::ReflectContext* context)
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

        serializeContext->Class<StringParameter, BaseType>()
            ->Version(1)
        ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<StringParameter>("String parameter", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        ;
    }

    const char* StringParameter::GetTypeDisplayName() const
    {
        return "String";
    }

    MCore::Attribute* StringParameter::ConstructDefaultValueAsAttribute() const
    {
        return MCore::AttributeString::Create(m_defaultValue);
    }

    uint32 StringParameter::GetType() const
    {
        return MCore::AttributeString::TYPE_ID;
    }

    bool StringParameter::AssignDefaultValueToAttribute(MCore::Attribute* attribute) const
    {
        if (attribute->GetType() == MCore::AttributeString::TYPE_ID)
        {
            static_cast<MCore::AttributeString*>(attribute)->SetValue(GetDefaultValue());
            return true;
        }
        else
        {
            return false;
        }
    }

    bool StringParameter::SetDefaultValueFromAttribute(MCore::Attribute* attribute)
    {
        if (attribute->GetType() == MCore::AttributeString::TYPE_ID)
        {
            SetDefaultValue(static_cast<MCore::AttributeString*>(attribute)->GetValue());
            return true;
        }
        else
        {
            return false;
        }
    }
}
