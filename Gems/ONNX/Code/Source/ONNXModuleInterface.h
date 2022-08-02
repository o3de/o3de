/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <Clients/ONNXSystemComponent.h>

namespace ONNX
{
    class ONNXModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(ONNXModuleInterface, "{D5A80703-FF4C-46FD-8EFF-C1D4781B66F2}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ONNXModuleInterface, AZ::SystemAllocator, 0);

        ONNXModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ONNXSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ONNXSystemComponent>(),
            };
        }
    };
}// namespace ONNX
