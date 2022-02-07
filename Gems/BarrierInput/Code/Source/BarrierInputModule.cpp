/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <BarrierInputSystemComponent.h>

namespace BarrierInput
{
    class BarrierInputModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(BarrierInputModule, "{C338BB3B-EA09-4FC8-AD49-840F8A22837F}", AZ::Module);
        AZ_CLASS_ALLOCATOR(BarrierInputModule, AZ::SystemAllocator, 0);

        BarrierInputModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                BarrierInputSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<BarrierInputSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_BarrierInput, BarrierInput::BarrierInputModule)
