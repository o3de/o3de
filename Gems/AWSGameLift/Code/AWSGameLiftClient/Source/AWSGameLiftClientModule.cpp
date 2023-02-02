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

#if defined(AWS_GAMELIFT_CLIENT_EDITOR)
#include <AWSGameLiftClientEditorSystemComponent.h>
#endif

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
            m_descriptors.insert(m_descriptors.end(),
                {
#if defined(AWS_GAMELIFT_CLIENT_EDITOR)
                    AWSGameLiftClientEditorSystemComponent::CreateDescriptor()
#else
                    AWSGameLiftClientSystemComponent::CreateDescriptor()
#endif
                }
            );
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList
            {
    #if defined(AWS_GAMELIFT_CLIENT_EDITOR)
                azrtti_typeid<AWSGameLiftClientEditorSystemComponent>()
    #else
                azrtti_typeid<AWSGameLiftClientSystemComponent>()
    #endif
            };
        }
    };
}// namespace AWSGameLift

AZ_DECLARE_MODULE_CLASS(Gem_AWSGameLift_Clients, AWSGameLift::AWSGameLiftClientModule)
