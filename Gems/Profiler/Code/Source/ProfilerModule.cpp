/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ProfilerSystemComponent.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace Profiler
{
    class ProfilerModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ProfilerModule, "{4A286414-B387-4D20-9A7E-2F792755B769}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ProfilerModule, AZ::SystemAllocator);

        ProfilerModule()
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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Profiler::ProfilerModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_Profiler, Profiler::ProfilerModule)
#endif
