/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProfilerImGuiSystemComponent.h>
#include <ProfilerSystemComponent.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace Profiler
{
    class ProfilerImGuiModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ProfilerImGuiModule, "{5946991E-A96C-4E7A-A9B3-605E3C8EC3CB}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ProfilerImGuiModule, AZ::SystemAllocator, 0);

        ProfilerImGuiModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ProfilerSystemComponent::CreateDescriptor(),
                ProfilerImGuiSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ProfilerSystemComponent>(),
                azrtti_typeid<ProfilerImGuiSystemComponent>(),
            };
        }
    };
}// namespace Profiler

AZ_DECLARE_MODULE_CLASS(Gem_ProfilerImGui, Profiler::ProfilerImGuiModule)
