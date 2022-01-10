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
#include <MotionMatchingSystemComponent.h>

namespace MotionMatching
{
    class MotionMatchingModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(MotionMatchingModuleInterface, "{33e8e826-b143-4008-89f3-9a46ad3de4fe}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MotionMatchingModuleInterface, AZ::SystemAllocator, 0);

        MotionMatchingModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                MotionMatchingSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<MotionMatchingSystemComponent>(),
            };
        }
    };
}// namespace MotionMatching
