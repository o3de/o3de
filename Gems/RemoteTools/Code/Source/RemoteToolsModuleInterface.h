/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <RemoteToolsSystemComponent.h>

namespace RemoteTools
{
    class RemoteToolsModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(RemoteToolsModuleInterface, "{737ac146-f2c5-4f21-bb86-4bb665ca5f65}", AZ::Module);
        AZ_CLASS_ALLOCATOR(RemoteToolsModuleInterface, AZ::SystemAllocator);

        RemoteToolsModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                RemoteToolsSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<RemoteToolsSystemComponent>(),
            };
        }
    };
}// namespace RemoteTools
