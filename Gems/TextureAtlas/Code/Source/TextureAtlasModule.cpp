/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>

#include "TextureAtlasSystemComponent.h"

#ifdef TEXTUREATLAS_EDITOR
#include "Editor/AtlasBuilderComponent.h"
#endif

#include <IGem.h>

namespace TextureAtlasNamespace
{
    class TextureAtlasModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(TextureAtlasModule, "{D3997F41-8117-4E0F-9BFE-937C4AE7E71F}", CryHooksModule);
        AZ_CLASS_ALLOCATOR(TextureAtlasModule, AZ::SystemAllocator);

        TextureAtlasModule()
            : CryHooksModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                TextureAtlasSystemComponent::CreateDescriptor(),
#ifdef TEXTUREATLAS_EDITOR
                TextureAtlasBuilder::AtlasBuilderComponent::CreateDescriptor(), //builder component for texture atlas
#endif
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<TextureAtlasSystemComponent>(),
            };
        }
    };
}

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), TextureAtlasNamespace::TextureAtlasModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_TextureAtlas, TextureAtlasNamespace::TextureAtlasModule)
#endif
