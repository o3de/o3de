/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <BootstrapSystemComponent.h>

namespace AZ
{
    namespace Render
    {
        namespace Bootstrap
        {
            class BootstrapModule
                : public AZ::Module
            {
            public:
                AZ_RTTI(BootstrapModule, "{ADDE20F4-03E6-4692-A736-E56B87952727}", AZ::Module);
                AZ_CLASS_ALLOCATOR(BootstrapModule, AZ::SystemAllocator, 0);

                BootstrapModule()
                    : AZ::Module()
                {
                    // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                    m_descriptors.insert(m_descriptors.end(), {
                        BootstrapSystemComponent::CreateDescriptor()
                        });
                }

                /**
                 * Add required SystemComponents to the SystemEntity.
                 */
                AZ::ComponentTypeList GetRequiredSystemComponents() const override
                {
                    return AZ::ComponentTypeList{
                        azrtti_typeid<BootstrapSystemComponent>()
                    };
                }
            };
        } // namespace Bootstrap
    } // namespace Render
} // namespace AZ

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_Bootstrap, AZ::Render::Bootstrap::BootstrapModule)
