/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <EMotionStudio/EMStudioSDK/Source/Allocators.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NamedPropertyStringValue.h>

namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(NamedPropertyStringValue, EMStudio::UIAllocator)

    NamedPropertyStringValue::NamedPropertyStringValue(const AZStd::string& name, const AZStd::string& value)
        : m_name(name)
        , m_value(value)
    {
    }

    void NamedPropertyStringValue::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<NamedPropertyStringValue>()
            ->Version(1)
            ->Field("name", &NamedPropertyStringValue::m_name)
            ->Field("value", &NamedPropertyStringValue::m_value)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<NamedPropertyStringValue>("Named property string value", "")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ->DataElement(AZ::Edit::UIHandlers::Default, &NamedPropertyStringValue::m_value, "", "")
                ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &NamedPropertyStringValue::m_name)
                ->Attribute(AZ::Edit::Attributes::ReadOnly, true)
            ;
    }

} // namespace EMStudio
