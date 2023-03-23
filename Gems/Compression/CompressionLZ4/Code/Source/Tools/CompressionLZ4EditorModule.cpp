/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CompressionLZ4/CompressionLZ4TypeIds.h>
#include <CompressionLZ4ModuleInterface.h>
#include "CompressionLZ4EditorSystemComponent.h"

namespace CompressionLZ4
{
    class CompressionLZ4EditorModule
        : public CompressionLZ4ModuleInterface
    {
    public:
        AZ_RTTI(CompressionLZ4EditorModule, CompressionLZ4EditorModuleTypeId, CompressionLZ4ModuleInterface);
        AZ_CLASS_ALLOCATOR(CompressionLZ4EditorModule, AZ::SystemAllocator);

        CompressionLZ4EditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                CompressionLZ4EditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<CompressionLZ4EditorSystemComponent>(),
            };
        }
    };
}// namespace CompressionLZ4

AZ_DECLARE_MODULE_CLASS(Gem_CompressionLZ4, CompressionLZ4::CompressionLZ4EditorModule)
