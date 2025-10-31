/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AtomImGuiToolsSystemComponent.h>

namespace AtomImGuiTools
{
    class AtomImGuiToolsModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AtomImGuiToolsModule, "{1B65F246-7977-4DC4-B5D9-BDAD374388FF}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AtomImGuiToolsModule, AZ::SystemAllocator);

        AtomImGuiToolsModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AtomImGuiToolsSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<AtomImGuiToolsSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AtomImGuiTools::AtomImGuiToolsModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AtomImGuiTools, AtomImGuiTools::AtomImGuiToolsModule)
#endif
