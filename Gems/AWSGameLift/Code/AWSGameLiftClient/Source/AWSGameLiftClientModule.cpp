/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AWSGameLiftClientSystemComponent.h>

namespace AWSGameLift
{
    //! Provide the entry point for the gem and register the system component.
    class AWSGameLiftClientModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSGameLiftClientModule, "{7b920f3e-2b23-482e-a1b6-16bd278d126c}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AWSGameLiftClientModule, AZ::SystemAllocator, 0);

        AWSGameLiftClientModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AWSGameLiftClientSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<AWSGameLiftClientSystemComponent>(),
            };
        }
    };
}// namespace AWSGameLift

AZ_DECLARE_MODULE_CLASS(Gem_AWSGameLift_Clients, AWSGameLift::AWSGameLiftClientModule)
