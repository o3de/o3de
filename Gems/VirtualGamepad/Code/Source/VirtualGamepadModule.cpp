/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "VirtualGamepadButtonComponent.h"
#include "VirtualGamepadSystemComponent.h"
#include "VirtualGamepadThumbStickComponent.h"

namespace VirtualGamepad
{
    class VirtualGamepadModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(VirtualGamepadModule, "{0454CF83-A35E-443B-A9BE-858EBE9C908F}", AZ::Module);
        AZ_CLASS_ALLOCATOR(VirtualGamepadModule, AZ::SystemAllocator);

        VirtualGamepadModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                VirtualGamepadSystemComponent::CreateDescriptor(),
                VirtualGamepadButtonComponent::CreateDescriptor(),
                VirtualGamepadThumbStickComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<VirtualGamepadSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), VirtualGamepad::VirtualGamepadModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_VirtualGamepad, VirtualGamepad::VirtualGamepadModule)
#endif
