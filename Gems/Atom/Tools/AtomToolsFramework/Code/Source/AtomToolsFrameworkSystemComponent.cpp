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

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AtomToolsFrameworkSystemComponent.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>

namespace AtomToolsFramework
{
    void AtomToolsFrameworkSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AtomToolsFramework::DynamicProperty::Reflect(context);
        AtomToolsFramework::DynamicPropertyGroup::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomToolsFrameworkSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomToolsFrameworkSystemComponent>("AtomToolsFrameworkSystemComponent", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomToolsFrameworkSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("AtomToolsFrameworkSystemService"));
    }

    void AtomToolsFrameworkSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("AtomToolsFrameworkSystemService"));
    }

    void AtomToolsFrameworkSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AtomToolsFrameworkSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AtomToolsFrameworkSystemComponent::Init()
    {
    }

    void AtomToolsFrameworkSystemComponent::Activate()
    {
    }

    void AtomToolsFrameworkSystemComponent::Deactivate()
    {
    }
}
