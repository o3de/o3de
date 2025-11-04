/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AssetValidationSystemComponent.h>

#ifdef EDITOR_MODULE
#include <Editor/Source/EditorAssetValidationSystemComponent.h>
#endif

namespace AssetValidation
{
    class AssetValidationModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AssetValidationModule, "{66A6C65D-7814-4CFF-AF54-B73925FD1188}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AssetValidationModule, AZ::SystemAllocator);

        AssetValidationModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                AssetValidationSystemComponent::CreateDescriptor(),
#ifdef EDITOR_MODULE
                EditorAssetValidationSystemComponent::CreateDescriptor(),
#endif

            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AssetValidationSystemComponent>(),
#ifdef EDITOR_MODULE
                azrtti_typeid<EditorAssetValidationSystemComponent>(),
#endif
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), AssetValidation::AssetValidationModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_AssetValidation, AssetValidation::AssetValidationModule)
#endif
