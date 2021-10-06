/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <ProfilerSystemComponent.h>

namespace Profiler
{
    class ProfilerModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(ProfilerModuleInterface, "{c966e43a-420d-41c9-bd0d-4cb0bca0d3e1}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ProfilerModuleInterface, AZ::SystemAllocator, 0);

        ProfilerModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ProfilerSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ProfilerSystemComponent>(),
            };
        }
    };
}// namespace Profiler
