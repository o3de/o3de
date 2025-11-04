/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptAutomationSystemComponent.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace AZ::ScriptAutomation
{
    class ScriptAutomationModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ScriptAutomationModule, "{1B94CF12-A1C3-47DC-86DD-CC44A6979F73}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ScriptAutomationModule, AZ::SystemAllocator);

        ScriptAutomationModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ScriptAutomationSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ScriptAutomationSystemComponent>(),
            };
        }
    };
}// namespace AZ::ScriptAutomation

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AZ::ScriptAutomation::ScriptAutomationModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ScriptAutomation, AZ::ScriptAutomation::ScriptAutomationModule)
#endif
