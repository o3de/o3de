/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AutomatedLauncherTestingSystemComponent.h>


namespace AutomatedLauncherTesting
{
    class AutomatedLauncherTestingModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AutomatedLauncherTestingModule, "{3FC3E44A-0AC0-47C5-BD02-ADB2BA4338CA}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AutomatedLauncherTestingModule, AZ::SystemAllocator, 0);

        AutomatedLauncherTestingModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AutomatedLauncherTestingSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AutomatedLauncherTestingSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_AutomatedLauncherTesting, AutomatedLauncherTesting::AutomatedLauncherTestingModule)
