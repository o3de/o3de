/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Source/AutoGen/AutoComponentTypes.h>

#include <AutomatedTestingSystemComponent.h>

namespace AutomatedTesting
{
    class AutomatedTestingModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AutomatedTestingModule, "{3D6F59F6-013F-46F8-A840-5C2C43FA6AFB}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AutomatedTestingModule, AZ::SystemAllocator);

        AutomatedTestingModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AutomatedTestingSystemComponent::CreateDescriptor(),
            });

            CreateComponentDescriptors(m_descriptors); //< Register multiplayer components
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AutomatedTestingSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AutomatedTesting::AutomatedTestingModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AutomatedTesting, AutomatedTesting::AutomatedTestingModule)
#endif
