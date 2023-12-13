/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <ImguiAtomSystemComponent.h>

namespace ImguiAtom
{
    class ImguiAtomModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ImguiAtomModule, "{E3CE5991-30B5-4B04-BF79-516DDBD4D233}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ImguiAtomModule, AZ::SystemAllocator);

        ImguiAtomModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AZ::LYIntegration::ImguiAtomSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AZ::LYIntegration::ImguiAtomSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), ImguiAtom::ImguiAtomModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_ImguiAtom, ImguiAtom::ImguiAtomModule)
#endif
