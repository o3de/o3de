/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <AtomBridgeModule.h>

#include "./Editor/AssetCollectionAsyncLoaderTestComponent.h"

namespace AZ
{
    namespace AtomBridge
    {
        class EditorModule
            : public Module
        {
        public:
            AZ_RTTI(EditorModule, "{7B330394-BE9C-4BDA-9345-1A0859815982}", Module);
            AZ_CLASS_ALLOCATOR(EditorModule, AZ::SystemAllocator, 0);

            EditorModule()
                : Module()
            {
                // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
                m_descriptors.insert(m_descriptors.end(), {
                    AssetCollectionAsyncLoaderTestComponent::CreateDescriptor(),
                    });
            }

            /**
             * Add required SystemComponents to the SystemEntity.
             */
            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                AZ::ComponentTypeList components = Module::GetRequiredSystemComponents();
                // components.insert(components.end(), {
                // });
                return components;
            }
        };
    }
} // namespace AZ

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_AtomBridge.Editor, AZ::AtomBridge::EditorModule)
