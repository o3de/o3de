/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RemoteToolsModuleInterface.h>
#include <RemoteToolsEditorSystemComponent.h>

namespace RemoteTools
{
    class RemoteToolsEditorModule
        : public RemoteToolsModuleInterface
    {
    public:
        AZ_RTTI(RemoteToolsEditorModule, "{86ed333f-1f40-497f-ac31-9de31dee9371}", RemoteToolsModuleInterface);
        AZ_CLASS_ALLOCATOR(RemoteToolsEditorModule, AZ::SystemAllocator, 0);

        RemoteToolsEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                RemoteToolsEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<RemoteToolsEditorSystemComponent>(),
            };
        }
    };
}// namespace RemoteTools

AZ_DECLARE_MODULE_CLASS(Gem_RemoteTools, RemoteTools::RemoteToolsEditorModule)
