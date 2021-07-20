/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Atom_RHI_Vulkan_precompiled.h"
#include <RHI/SystemComponent.h>
#include <Atom/RHI.Reflect/Vulkan/ReflectSystemComponent.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>
#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace Vulkan
    {
        class PlatformModule
            : public AZ::Module
        {
        public:
            AZ_RTTI(PlatformModule, "{2B90FAEE-0E76-4A2E-905F-66FC01089DB9}", Module);

            PlatformModule()
            {
                m_descriptors.insert(m_descriptors.end(), {
                    ReflectSystemComponent::CreateDescriptor(),
                    SystemComponent::CreateDescriptor()
                });
            }
            ~PlatformModule() override = default;

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                return
                {
                    azrtti_typeid<Vulkan::SystemComponent>()
                };
            }
        };
    }
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RHI_Vulkan_Private, AZ::Vulkan::PlatformModule)
