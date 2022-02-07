/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AWSGameLiftServerSystemComponent.h>

namespace AWSGameLift
{
    //! Provide the entry point for the gem and register the system component.
    class AWSGameLiftServerModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AWSGameLiftServerModule, "{898416ca-dc11-4731-87de-afe285aedb04}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AWSGameLiftServerModule, AZ::SystemAllocator, 0);

        AWSGameLiftServerModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AWSGameLiftServerSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<AWSGameLiftServerSystemComponent>(),
            };
        }
    };
}// namespace AWSGameLift

AZ_DECLARE_MODULE_CLASS(Gem_AWSGameLift_Servers, AWSGameLift::AWSGameLiftServerModule)
