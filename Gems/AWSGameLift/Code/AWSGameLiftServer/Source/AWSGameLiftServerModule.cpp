/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

AZ_DECLARE_MODULE_CLASS(Gem_AWSGameLift_Server, AWSGameLift::AWSGameLiftServerModule)
