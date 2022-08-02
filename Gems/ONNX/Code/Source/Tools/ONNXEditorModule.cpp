/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ONNXModuleInterface.h>
#include "ONNXEditorSystemComponent.h"

namespace ONNX
{
    class ONNXEditorModule
        : public ONNXModuleInterface
    {
    public:
        AZ_RTTI(ONNXEditorModule, "{E006F52B-8EC8-4DFE-AB9D-C5EF7A1A8F32}", ONNXModuleInterface);
        AZ_CLASS_ALLOCATOR(ONNXEditorModule, AZ::SystemAllocator, 0);

        ONNXEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ONNXEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<ONNXEditorSystemComponent>(),
            };
        }
    };
}// namespace ONNX

AZ_DECLARE_MODULE_CLASS(Gem_ONNX, ONNX::ONNXEditorModule)
