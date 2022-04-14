/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AutomationSystemComponent.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace Automation
{
    class AutomationModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AutomationModule, "{1B94CF12-A1C3-47DC-86DD-CC44A6979F73}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AutomationModule, AZ::SystemAllocator, 0);

        AutomationModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                AutomationSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AutomationSystemComponent>(),
            };
        }
    };
}// namespace Automation

AZ_DECLARE_MODULE_CLASS(AutomationModule, Automation::AutomationModule)
