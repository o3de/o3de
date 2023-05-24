/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <MotionMatchingSystemComponent.h>

namespace EMotionFX::MotionMatching
{
    class MotionMatchingModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(MotionMatchingModuleInterface, "{33e8e826-b143-4008-89f3-9a46ad3de4fe}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MotionMatchingModuleInterface, AZ::SystemAllocator);

        MotionMatchingModuleInterface()
        {
            m_descriptors.insert(m_descriptors.end(),
                {
                    MotionMatchingSystemComponent::CreateDescriptor(),
                });
        }

        /// Add required SystemComponents to the SystemEntity.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList
                {
                    azrtti_typeid<MotionMatchingSystemComponent>(),
                };
        }
    };
}// namespace EMotionFX::MotionMatching
